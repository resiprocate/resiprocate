#if !defined(RESIP_ACCOUNTINGCOLLECTOR_HXX)
#define RESIP_ACCOUNTINGCOLLECTOR_HXX 

#include <memory>
#include "rutil/ThreadIf.hxx"
#include "rutil/TimeLimitFifo.hxx"
#include "resip/stack/SipMessage.hxx"

namespace json
{
   class Object;
}

namespace repro
{
class RequestContext;
class PersistentMessageEnqueue;
class ProxyConfig;

class AccountingCollector : public resip::ThreadIf
{
public:
   typedef enum
   {
      RegistrationAdded = 1,
      RegistrationRefreshed = 2,
      RegistrationRemoved = 3,
      RegistrationRemovedAll = 4
   } RegistrationEvent;

   typedef enum
   {
      SessionCreated = 1,
      SessionRouted = 2,
      SessionRedirected = 3,
      SessionEstablished = 4,
      SessionCancelled = 5,
      SessionEnded = 6,
      SessionError = 7
   } SessionEvent;

   AccountingCollector(ProxyConfig& config);
   virtual ~AccountingCollector();

   virtual void doSessionAccounting(const resip::SipMessage& sip, bool received, RequestContext& context);
   virtual void doRegistrationAccounting(RegistrationEvent regevent, const resip::SipMessage& sip);

private:
   resip::Data mDbBaseDir;
   PersistentMessageEnqueue* mSessionEventQueue;
   PersistentMessageEnqueue* mRegistrationEventQueue;
   bool mSessionAccountingAddRoutingHeaders;
   bool mSessionAccountingAddViaHeaders;
   bool mRegistrationAccountingAddRoutingHeaders;
   bool mRegistrationAccountingAddViaHeaders;
   bool mRegistrationAccountingLogRefreshes;

   virtual void thread();

   enum FifoEventType
   {
      SessionEventType,
      RegistrationEventType
   };
   class FifoEvent
   {
   public:
      FifoEventType mType;
      resip::Data mData;
   };
   resip::TimeLimitFifo<FifoEvent> mFifo;
   PersistentMessageEnqueue* initializeEventQueue(FifoEventType type, bool destroyFirst=false);
   void pushEventObjectToQueue(json::Object& object, FifoEventType type);
   void internalProcess(std::auto_ptr<FifoEvent> eventData);
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
