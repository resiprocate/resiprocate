#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/ConnectionManager.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Inserter.hxx"

#include <vector>

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT


// smallest time to reuse
const UInt64 
ConnectionManager::MinLastUsed = 1;

// largest unused time to reclaim
const UInt64 
ConnectionManager::MaxLastUsed = 1000;

ConnectionManager::ConnectionManager() : 
   mHead(),
   mWriteHead(ConnectionWriteList::makeList(&mHead)),
   mWriteIter(mWriteHead->begin()),
   mReadHead(ConnectionReadList::makeList(&mHead)),
   mReadIter(mReadHead->begin()),
   mLRUHead(ConnectionLruList::makeList(&mHead)),
   mConnectionIdGenerator(1) 
{
}

ConnectionManager::~ConnectionManager()
{
   while (!mAddrMap.empty())
   {
      delete mAddrMap.begin()->second;
   }
   assert(mReadHead->empty());
   assert(mWriteHead->empty());
   assert(mLRUHead->empty());
}

Connection*
ConnectionManager::findConnection(const Tuple& addr)
{
   if (addr.mConnectionId == 0)
   {
      AddrMap::const_iterator i = mAddrMap.find(addr);
      if (i != mAddrMap.end())
      {
         return i->second;
      }
   }
   else
   {
      IdMap::const_iterator i = mIdMap.find(addr.mConnectionId);
      if (i != mIdMap.end())
      {
         return i->second;
      }
   }
   
   return 0;
}

Connection*
ConnectionManager::getNextRead(FdSet &fdset)
{
#ifdef _WIN32
    if (mReadHead->empty() || fdset.read.fd_count == 0)
#else
   if (mReadHead->empty())
#endif
   {
      return 0;
   }
   else 
   {
      ConnectionReadList::iterator startIter = ++mReadIter;

      // loop through all connections looking for signalled ones only
      do
      {
         if (mReadIter == mReadHead->end())
         {
            mReadIter = mReadHead->begin();
            if(mReadIter == startIter) break;
         }

         if(fdset.readyToRead((*mReadIter)->getSocket()))
         {
            Connection* ret = *mReadIter;
            return ret;
         }
      } while(++mReadIter != startIter);

      return 0;
   }
}

Connection*
ConnectionManager::getNextWrite()
{
   if (mWriteHead->empty())
   {
      return 0;
   }
   else 
   {
      if (++mWriteIter == mWriteHead->end())
      {
         mWriteIter = mWriteHead->begin();
      }

      Connection* ret = *mWriteIter;
      return ret;
   }
}

void
ConnectionManager::buildFdSet(FdSet& fdset) const
{
   //
   // !kh!
   // The two const_cast hacks used here are not beautiful, but safe.
   // Since setting fdset doesn't alter the state of the descriptor of
   // a connection and this allows ConnectionManager::buildFdSet() to
   // be a const memeber, which is more semantically correct.
   // Connection::getSocket() isn't const, that's understandable, one
   // might use it to alter the state of the descriptor, such as close().
   // Anyone knows why STD containers was not used for read/write
   // connections? I am guessing this is a legacy, should rewrite using
   // STD containers.
   // 
   for (ConnectionReadList::iterator i = mReadHead->begin(); 
        i != mReadHead->end(); ++i)
   {
      Connection* conn(const_cast<Connection*>(*i));
      Socket sock(conn->getSocket());
      fdset.setRead(sock);
      fdset.setExcept(sock);
   }

   for (ConnectionWriteList::iterator i = mWriteHead->begin(); 
        i != mWriteHead->end(); ++i)
   {
      Connection* conn(const_cast<Connection*>(*i));
      Socket sock(conn->getSocket());
      fdset.setWrite(sock);
      fdset.setExcept(sock);
   }
}

void
ConnectionManager::addToWritable(Connection* conn)
{
   mWriteHead->push_back(conn);
}

void
ConnectionManager::removeFromWritable()
{
   assert(!mWriteHead->empty());
   Connection* current = *mWriteIter;
   ++mWriteIter;
   current->ConnectionWriteList::remove();

   if (mWriteIter == mWriteHead->end())
   {
      mWriteIter = mWriteHead->begin();
   }
}

void
ConnectionManager::addConnection(Connection* connection)
{
   connection->who().mConnectionId = ++mConnectionIdGenerator;
   //DebugLog (<< "ConnectionManager::addConnection() " << connection->mWho.connectionId  << ":" << connection->mSocket);
   
   mAddrMap[connection->who()] = connection;
   mIdMap[connection->who().mConnectionId] = connection;

   mReadHead->push_back(connection);
   mLRUHead->push_back(connection);

   //DebugLog (<< "count=" << mAddrMap.count(connection->who()) << "who=" << connection->who() << " mAddrMap=" << Inserter(mAddrMap));
   //assert(mAddrMap.begin()->first == connection->who());
   assert(mAddrMap.count(connection->who()) == 1);
}

void
ConnectionManager::removeConnection(Connection* connection)
{
   //DebugLog (<< "ConnectionManager::removeConnection()");

   assert(!mReadHead->empty());

   mIdMap.erase(connection->mWho.mConnectionId);
   mAddrMap.erase(connection->mWho);

   connection->ConnectionReadList::remove();
   connection->ConnectionWriteList::remove();
   connection->ConnectionLruList::remove();

   // keep the iterators valid
   mReadIter = mReadHead->begin();
   mWriteIter = mWriteHead->begin();
}

// release excessively old connections (free up file descriptors)
void
ConnectionManager::gc(UInt64 relThreshhold)
{
   UInt64 threshhold = Timer::getTimeMs() - relThreshhold;
   InfoLog(<< "recycling connections older than " << relThreshhold/1000.0 << " seconds");

   for (ConnectionLruList::iterator i = mLRUHead->begin();
        i != mLRUHead->end();)
   {
      if ((*i)->whenLastUsed() < threshhold)
      {
         Connection* discard = *i;
         InfoLog(<< "recycling connection: " << discard << " " << discard->getSocket());
         // iterate before removing
         ++i;
         delete discard;
      }
      else
      {
         break;
      }
   }
}

// move to youngest
void
ConnectionManager::touch(Connection* connection)
{
   connection->ConnectionLruList::remove();
   mLRUHead->push_back(connection);
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
