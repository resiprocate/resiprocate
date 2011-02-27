#ifndef RESIP_KEEPALIVE_MANAGER_HXX
#define RESIP_KEEPALIVE_MANAGER_HXX

#include <map>
#include "resip/stack/Tuple.hxx"

namespace resip 
{

class KeepAliveTimeout;
class KeepAlivePongTimeout;
class DialogUsageManager;

class KeepAliveManager
{
   public:
      // Defaults to 10000ms (10s) as specified in RFC5626 section 4.4.1 
      static int mKeepAlivePongTimeoutMs;  // ?slg? move to Profile setting?

      struct NetworkAssociationInfo
      {
            int refCount;
            int keepAliveInterval;  // In seconds
            int id;
            bool supportsOutbound;
            bool pongReceivedForLastPing;
      };

      // .slg.  We track unique Network Associations per transport transport type, transport family, 
      //        target ip addr, target port, and source/transport flow key
      //        This means that if we have two different TCP connections to the same destination, 
      //        each originating from a different NIC, then we will send keepalives on each separately.
      //        For UDP, this is not currently the case, when the transport is bound to any interface
      //        (ie. 0.0.0.0), as the flow key will be same regardless of the source interface used to
      //        send the UDP message - fixing this for UDP remains an outstanding item.
      typedef std::map<Tuple, NetworkAssociationInfo, Tuple::FlowKeyCompare> NetworkAssociationMap;

      KeepAliveManager() : mCurrentId(0) {}
      virtual ~KeepAliveManager() {}
      void setDialogUsageManager(DialogUsageManager* dum) { mDum = dum; }
      virtual void add(const Tuple& target, int keepAliveInterval, bool targetSupportsOutbound);
      virtual void remove(const Tuple& target);
      virtual void process(KeepAliveTimeout& timeout);
      virtual void process(KeepAlivePongTimeout& timeout);
      virtual void receivedPong(const Tuple& flow);

   protected:
      DialogUsageManager* mDum;
      NetworkAssociationMap mNetworkAssociations;
      unsigned int mCurrentId;
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
