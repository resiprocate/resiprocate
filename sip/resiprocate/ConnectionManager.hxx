#if !defined(RESIP_ConnectionMgr_hxx)
#define RESIP_ConnectionMgr_hxx 

#include <memory>
#include <list>

#include "resiprocate/os/HashMap.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Connection.hxx"

namespace resip
{

class ConnectionManager
{
      friend class Connection;
   public:
      // smallest time to reuse
      static const UInt64 MinLastUsed;
      // largest unused time to reclaim
      static const UInt64 MaxLastUsed;
 
      ConnectionManager();
      ~ConnectionManager();

      // !jf! May want to add an interface that returns a Connection* given a Tuple
      Connection* findConnection(const Tuple& tuple);

      // Return 0 if nothing to do
      Connection* getNextRead();
      Connection* getNextWrite();
      void buildFdSet(FdSet& fdset);
      
   private:
      void addToWritable(Connection* conn); // add the specified conn to end
      void removeFromWritable(); // remove the current mWriteMark

      // release excessively old connections (free up file descriptors)
      void gc(UInt64 threshhold = ConnectionManager::MaxLastUsed);

      typedef HashMap<Tuple, Connection*> AddrMap;
      typedef HashMap<ConnectionId, Connection*> IdMap;

      void addConnection(Connection* connection);
      void removeConnection(Connection* connection);

      // move to youngest 
      // !jf! I think it would be reasonable to look for oldest when time comes
      // to delete rather than touch each time
      void touch(Connection* connection);
      
      AddrMap mAddrMap;
      IdMap mIdMap;
      std::list<Connection*> mWriteSet;
      std::list<Connection*>::iterator mWriteMark;

      std::list<Connection*> mReadSet;
      std::list<Connection*>::iterator mReadMark;

      // least recently used
      Connection mPreYoungest;
      Connection mPostOldest;
      ConnectionId mConnectionIdGenerator;

      friend class TcpBaseTransport;
};

}

#endif

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
