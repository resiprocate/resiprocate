#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif


#include <cassert>

#include "resiprocate/os/Logger.hxx"
#include "resiprocate/ConnectionMap.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT


using namespace std;

using namespace resip;


const UInt64 ConnectionMap::MinLastUsed = 10*1000;
const UInt64 ConnectionMap::MaxLastUsed = 10*60*60*1000;


ConnectionMap::ConnectionMap()
{
   mPreYoungest.mOlder = &mPostOldest;
   mPostOldest.mYounger = &mPreYoungest;
}

ConnectionMap::~ConnectionMap()
{
   for (Map::iterator it = mConnections.begin();
        it != mConnections.end(); it++)
   {
      delete it->second;
   }
   mConnections.clear();
}

void
ConnectionMap::touch(Connection* connection)
{
   connection->mLastUsed = Timer::getTimeMs();

   connection->remove();
   connection->mOlder = mPreYoungest.mOlder;
   mPreYoungest.mOlder->mYounger = connection;
   connection->mYounger = &mPreYoungest;
   mPreYoungest.mOlder = connection;
}

Connection*
ConnectionMap::add(Tuple& who, Socket socket)
{
   DebugLog (<< "Adding " << who);
   assert(mConnections.count(who) == 0);
   
   Connection* connection = new Connection(who, socket);
   assert(socket > 2);
   
   who.connection = connection;
   mConnections.insert(std::make_pair(who,connection));
   touch(connection);

   DebugLog(<< "ConnectionMap::add: " << who << " fd: " << int(socket) );
      
   return connection;
}

Connection*
ConnectionMap::get(const Tuple& who)
{
   Map::const_iterator i = mConnections.find(who);
   if (i != mConnections.end())
   {
      return i->second;
   }
   return 0;
}

void
ConnectionMap::close(const Tuple& who)
{
   DebugLog (<< "Closing " << who);
   
   Map::iterator i = mConnections.find(who);
   if (i != mConnections.end())
   {
      i->second->remove();
      mConnections.erase(i);
      //delete i->second;
   }
}

void
ConnectionMap::gc(UInt64 relThreshhold)
{
   UInt64 threshhold = Timer::getTimeMs() - relThreshhold;

   // start with the oldest
   Connection* i = mPostOldest.mYounger;
   while ((i != &mPreYoungest) && (i->mLastUsed < threshhold))
   {
      DebugLog( << "ConnectionMap::gc: " << *i);
      Connection* old = i;
      i->remove();
      i = mPostOldest.mYounger;
      mConnections.erase(old->mWho);
      delete old;
   }
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
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
