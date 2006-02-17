#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Transport.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/TlsConnection.hxx"
#include "resip/stack/TlsTransport.hxx"
#include "resip/stack/ConnectionManager.hxx"
#include "repro/monkeys/IsTrustedNode.hxx"
#include "repro/RequestContext.hxx"
#include "repro/Proxy.hxx"
#include "repro/AclStore.hxx"
#include <ostream>

#include "rutil/Logger.hxx"
#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;

IsTrustedNode::IsTrustedNode(AclStore& store) :
   mAclStore(store)
{}

IsTrustedNode::~IsTrustedNode()
{}

Processor::processor_action_t
IsTrustedNode::process(RequestContext& context)
{
   DebugLog(<< "Monkey handling request: " << *this 
            << "; reqcontext = " << context);

   resip::SipMessage& request = context.getOriginalRequest();
   Tuple source = request.getSource();

   // check the sender of the message via source IP address or identity from TLS 
   
   // check if the request came over a secure channel and sucessfully authenticated 
   // (ex: TLS or DTLS)
#ifdef USE_SSL
   if(request.getReceivedTransport()->transport() == resip::TLS
#ifdef USE_DTLS
      || request.getReceivedTransport()->transport() == resip::DTLS
#endif
      )
   {
      TcpBaseTransport *transport = const_cast<TcpBaseTransport *>(dynamic_cast<const TcpBaseTransport *>(request.getReceivedTransport()));
      assert(transport);
      ConnectionManager &connectionManager = transport->getConnectionManager();
      TlsConnection* conn = dynamic_cast<TlsConnection *>(connectionManager.findConnection(source));
      assert(conn);

      // !slg! TODO need mechanism in resip to check client cert chain - and we should verify cert is valid here.
      if(mAclStore.isTlsPeerNameTrusted(conn->getPeerName()))
      {
         InfoLog (<< "IsTrustedNode Monkey - Tls peer name IS trusted: " << conn->getPeerName());
         context.setFromTrustedNode();
      }
      else
      {
         InfoLog (<< "IsTrustedNode Monkey - Tls peer name NOT trusted: " << conn->getPeerName());
      }
   }
#endif

   // check the source address against the TrustedNode list
   if(!context.fromTrustedNode())
   {
      if(mAclStore.isAddressTrusted(source))
      {
         InfoLog (<< "IsTrustedNode Monkey - source address IS trusted: " << source.presentationFormat() << ":" << source.getPort() << " " << Tuple::toData(source.getType()));
         context.setFromTrustedNode();
      }
      else
      {
         InfoLog (<< "IsTrustedNode Monkey - source address NOT trusted: " << source.presentationFormat() << ":" << source.getPort() << " " << Tuple::toData(source.getType()));
      }
   }      
   
   // strip PAI headers that we don't trust
   if ((!context.fromTrustedNode()) && request.exists(h_PAssertedIdentities))
   {
      request.remove(h_PAssertedIdentities);
   }
   
   return Processor::Continue;
}

void
IsTrustedNode::dump(std::ostream &os) const
{
  os << "IsTrustedNode monkey" << std::endl;
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
