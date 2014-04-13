#if !defined(RESIP_REQUEST_CONTEXT_HXX)
#define RESIP_REQUEST_CONTEXT_HXX 

#include <vector>
#include <iosfwd>
#include "resip/stack/Uri.hxx"
#include "repro/ProcessorChain.hxx"
#include "repro/ResponseContext.hxx"
#include "resip/stack/NameAddr.hxx"
#include "repro/ResponseContext.hxx"
#include "repro/TimerCMessage.hxx"
#include "rutil/resipfaststreams.hxx"
#include "rutil/KeyValueStore.hxx"

namespace resip
{
class SipMessage;
class TransactionTerminated;
}


namespace repro
{
class Proxy;

class RequestContext
{
   public:
      RequestContext(Proxy& proxy,
                     ProcessorChain& requestP, // monkeys
                     ProcessorChain& responseP, // lemurs
                     ProcessorChain& targetP); // baboons
      virtual ~RequestContext();

      virtual void process(resip::TransactionTerminated& msg);
      virtual void process(std::auto_ptr<resip::SipMessage> sip);
      virtual void process(std::auto_ptr<resip::ApplicationMessage> app);
      
      virtual void handleSelfAimedStrayAck(resip::SipMessage* sip);
      virtual void cancelClientTransaction(const resip::Data& tid);

      /// Returns the SipMessage associated with the server transaction
      resip::SipMessage& getOriginalRequest();
      const resip::SipMessage& getOriginalRequest() const;
      resip::Data getTransactionId() const;
      
      /** Returns the event that we are currently working on. Use a pointer
          since users need to check for null */
      resip::Message* getCurrentEvent();
      const resip::Message* getCurrentEvent() const;
      
      void setDigestIdentity (const resip::Data&);
      const resip::Data& getDigestIdentity() const;

      Proxy& getProxy();
      ResponseContext& getResponseContext();
      
      resip::NameAddr& getTopRoute();
      const resip::Data& getDigestRealm();
            
      virtual void send(resip::SipMessage& msg);
      void sendResponse(resip::SipMessage& response);

      virtual void forwardAck200(const resip::SipMessage& ack);
      void postAck200Done();
      
      void updateTimerC();
      bool mInitialTimerCSet;

      void setSessionCreatedEventSent() { mSessionCreatedEventSent = true; }
      void setSessionEstablishedEventSent() { mSessionEstablishedEventSent = true; }

      void postTimedMessage(std::auto_ptr<resip::ApplicationMessage> msg,int seconds);

      // Accessor for per-requset extensible state storage for monkeys
      resip::KeyValueStore& getKeyValueStore() { return mKeyValueStore; }
      
      bool mHaveSentFinalResponse;
   protected:
      resip::SipMessage*  mOriginalRequest;
      resip::Message*  mCurrentEvent;
      resip::SipMessage* mAck200ToRetransmit;
      ProcessorChain& mRequestProcessorChain; // monkeys
      ProcessorChain& mResponseProcessorChain; // lemurs
      ProcessorChain& mTargetProcessorChain; // baboons

      bool processRequestInviteTransaction(resip::SipMessage* msg,bool original);
      bool processRequestNonInviteTransaction(resip::SipMessage* msg,bool original);
      void processRequestAckTransaction(resip::SipMessage* msg,bool original);
      void doPostRequestProcessing(resip::SipMessage* msg, bool original);
      bool processResponseInviteTransaction(resip::SipMessage* msg);
      bool processResponseNonInviteTransaction(resip::SipMessage* msg);
      void processResponseAckTransaction(resip::SipMessage* msg);
      void doPostResponseProcessing(resip::SipMessage* msg);
      
      resip::Data mDigestIdentity;
      int mTransactionCount;
      Proxy& mProxy;
      resip::NameAddr mTopRoute;
      ResponseContext mResponseContext;
      int mTCSerial;
      bool mSessionCreatedEventSent;
      bool mSessionEstablishedEventSent;
      resip::KeyValueStore mKeyValueStore;

      typedef std::vector<ProcessorChain::Chain::iterator>

      /** Stack of iterators used to keep track of where
          we are in the request processor chain(s) for
          async processing */
      ChainIteratorStack;
      ChainIteratorStack mChainIteratorStack;
      
      virtual void fixStrictRouterDamage();
      virtual void removeTopRouteIfSelf();
      
      friend class ResponseContext;
      friend EncodeStream& operator<<(EncodeStream& strm, const repro::RequestContext& rc);
};

EncodeStream&
operator<<(EncodeStream& strm, const repro::RequestContext& rc);


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
