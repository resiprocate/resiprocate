#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "resip/stack/ConnectionManager.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Inserter.hxx"

#include <vector>

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

const UInt64 ConnectionManager::MinimumGcAge = 1;
ConnectionId ConnectionManager::theConnectionIdGenerator=1;
resip::Mutex ConnectionManager::theCidMutex;

ConnectionManager::ConnectionManager() : 
   mHead(),
   mWriteHead(ConnectionWriteList::makeList(&mHead)),
   mWriteIter(mWriteHead->begin()),
   mReadHead(ConnectionReadList::makeList(&mHead)),
   mReadIter(mReadHead->begin()),
   mLRUHead(ConnectionLruList::makeList(&mHead))
{
   DebugLog(<<"ConnectionManager::ConnectionManager() called ");
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
   if (addr.connectionId != 0)
   {
      IdMap::iterator i = mIdMap.find(addr.connectionId);
      if (i != mIdMap.end())
      {
         if(i->second->who()==addr)
         {
            DebugLog(<<"Found connection id " << addr.connectionId);
            return i->second;
         }
         else
         {
            DebugLog(<<"connection id " << addr.connectionId 
                     << " exists, but does not match the destination. Cid -> "
                     << i->second->who() << ", tuple -> " << addr);
         }
      }
      else
      {
         DebugLog(<<"connection id " << addr.connectionId << " does not exist.");
      }
   }
   
   AddrMap::iterator i = mAddrMap.find(addr);
   if (i != mAddrMap.end())
   {
      DebugLog(<<"Found connection for tuple "<< addr );
      return i->second;
   }

   
   DebugLog(<<"Could not find connection " << addr.connectionId);
   return 0;
}

const Connection* 
ConnectionManager::findConnection(const Tuple& addr) const
{
   if (addr.connectionId != 0)
   {
      IdMap::const_iterator i = mIdMap.find(addr.connectionId);
      if (i != mIdMap.end() && i->second->who()==addr)
      {
         DebugLog(<<"Found connection id " << addr.connectionId);
         return i->second;
      }
   }
   
   AddrMap::const_iterator i = mAddrMap.find(addr);
   if (i != mAddrMap.end())
   {
      DebugLog(<<"Found connection for tuple "<< addr );
      return i->second;
   }

   
   DebugLog(<<"Could not find connection " << addr.connectionId);
   return 0;
}

Connection*
ConnectionManager::getNextRead(FdSet &fdset)
{
#ifdef WIN32
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
ConnectionManager::buildFdSet(FdSet& fdset)
{
   for (ConnectionReadList::iterator i = mReadHead->begin(); 
        i != mReadHead->end(); ++i)
   {
      fdset.setRead((*i)->getSocket());
      fdset.setExcept((*i)->getSocket());
   }

   for (ConnectionWriteList::iterator i = mWriteHead->begin(); 
        i != mWriteHead->end(); ++i)
   {
      fdset.setWrite((*i)->getSocket());
      fdset.setExcept((*i)->getSocket());
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
   assert(mAddrMap.find(connection->who())==mAddrMap.end());

   {
      resip::Lock g(theCidMutex);
      connection->who().connectionId = ++theConnectionIdGenerator;
   }
   //DebugLog (<< "ConnectionManager::addConnection() " << connection->mWho.connectionId  << ":" << connection->mSocket);
   
   
   mAddrMap[connection->who()] = connection;
   mIdMap[connection->who().connectionId] = connection;

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


   mIdMap.erase(connection->mWho.connectionId);
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
 * Copyright (c)
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
