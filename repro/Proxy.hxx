#if !defined(RESIP_PROXY_HXX)
#define RESIP_PROXY_HXX 

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/TransactionUser.hxx"
#include "rutil/HashMap.hxx"
#include "rutil/ThreadIf.hxx"
#include "repro/RequestContext.hxx"
#include "repro/TimerCMessage.hxx"

namespace resip
{
class SipStack;
}

namespace repro
{

class UserStore;
class ProcessorChain;

class Proxy : public resip::TransactionUser, public resip::ThreadIf
{
   public:
      Proxy(resip::SipStack&,
            const resip::Uri& recordRoute, 
            ProcessorChain& requestP, 
            ProcessorChain& responseP,
            ProcessorChain& targetP,
            UserStore& ,
            int timerC);
      virtual ~Proxy();

      virtual bool isShutDown() const ;
      virtual void thread();
      
      bool isMyUri(const resip::Uri& uri);      
      const resip::NameAddr& getRecordRoute() const;
      
      UserStore& getUserStore();
      resip::SipStack& getStack(){return mStack;}
      void send(const resip::SipMessage& msg);
      void addClientTransaction(const resip::Data& transactionId, RequestContext* rc);

      void postTimerC(std::auto_ptr<TimerCMessage> tc);

      void postMS(std::auto_ptr<resip::ApplicationMessage> msg, int msec);

      int mTimerC;
      
      
   protected:
      virtual const resip::Data& name() const;

   private:
      resip::SipStack& mStack;
      resip::NameAddr mRecordRoute;
      
      // needs to be a reference since parent owns it
      ProcessorChain& mRequestProcessorChain;
      ProcessorChain& mResponseProcessorChain;
      ProcessorChain& mTargetProcessorChain;
      
      /** a map from transaction id to RequestContext. Store the server
          transaction and client transactions in this map. The
          TransactionTerminated events from the stack will be passed to the
          RequestContext
      */
      HashMap<resip::Data, RequestContext*> mClientRequestContexts;
      HashMap<resip::Data, RequestContext*> mServerRequestContexts;
      
      UserStore &mUserStore;
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
