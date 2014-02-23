#if !defined(CommandServer_hxx)
#define CommandServer_hxx 

#include <rutil/Data.hxx>
#include <rutil/dns/DnsStub.hxx>
#include <rutil/TransportType.hxx>
#include <rutil/XMLCursor.hxx>
#include <resip/stack/StatisticsMessage.hxx>
#include <resip/dum/InMemorySyncRegDb.hxx>
#include "repro/XmlRpcServerBase.hxx"
#include "repro/Proxy.hxx"

namespace resip
{
class SipStack;
}

namespace repro
{
class ReproRunner;

class CommandServer: public XmlRpcServerBase,
                     public resip::GetDnsCacheDumpHandler
{
public:
   CommandServer(ReproRunner& reproRunner,
                 resip::Data ipAddr,
                 int port, 
                 resip::IpVersion version);
   virtual ~CommandServer();

   // thread safe
   virtual void sendResponse(unsigned int connectionId, 
                             unsigned int requestId, 
                             const resip::Data& responseData, 
                             unsigned int resultCode, 
                             const resip::Data& resultText);

   virtual void handleStatisticsMessage(resip::StatisticsMessage &statsMessage);

protected:
   virtual void handleRequest(unsigned int connectionId, 
                              unsigned int requestId, 
                              const resip::Data& request); 

   // Handlers
   virtual void onDnsCacheDumpRetrieved(std::pair<unsigned long, unsigned long> key, const resip::Data& dnsEntryStrings);

private: 
   void handleGetStackInfoRequest(unsigned int connectionId, unsigned int requestId, resip::XMLCursor& xml);
   void handleGetStackStatsRequest(unsigned int connectionId, unsigned int requestId, resip::XMLCursor& xml);
   void handleResetStackStatsRequest(unsigned int connectionId, unsigned int requestId, resip::XMLCursor& xml);
   void handleLogDnsCacheRequest(unsigned int connectionId, unsigned int requestId, resip::XMLCursor& xml);
   void handleClearDnsCacheRequest(unsigned int connectionId, unsigned int requestId, resip::XMLCursor& xml);
   void handleGetDnsCacheRequest(unsigned int connectionId, unsigned int requestId, resip::XMLCursor& xml);
   void handleGetCongestionStatsRequest(unsigned int connectionId, unsigned int requestId, resip::XMLCursor& xml);
   void handleSetCongestionToleranceRequest(unsigned int connectionId, unsigned int requestId, resip::XMLCursor& xml);
   void handleShutdownRequest(unsigned int connectionId, unsigned int requestId, resip::XMLCursor& xml);
   void handleGetProxyConfigRequest(unsigned int connectionId, unsigned int requestId, resip::XMLCursor& xml);
   void handleRestartRequest(unsigned int connectionId, unsigned int requestId, resip::XMLCursor& xml);
   void handleAddTransportRequest(unsigned int connectionId, unsigned int requestId, resip::XMLCursor& xml);
   void handleRemoveTransportRequest(unsigned int connectionId, unsigned int requestId, resip::XMLCursor& xml);

   ReproRunner& mReproRunner;
   resip::Mutex mStatisticsWaitersMutex;
   typedef std::list<std::pair<unsigned int, unsigned int> > StatisticsWaitersList;
   StatisticsWaitersList mStatisticsWaiters;
};

}

#endif  

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * Copyright (c) 2010 SIP Spectrum, Inc.  All rights reserved.
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
