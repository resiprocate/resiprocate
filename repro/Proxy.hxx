#if !defined(RESIP_PROXY_HXX)
#define RESIP_PROXY_HXX 

#include <memory>
#include <map>

#include "resip/stack/SipMessage.hxx"
#include "resip/stack/TransactionUser.hxx"
#include "rutil/HashMap.hxx"
#include "rutil/Mutex.hxx"
#include "rutil/ThreadIf.hxx"
#include "rutil/KeyValueStore.hxx"
#include "repro/AccountingCollector.hxx"
#include "repro/RequestContext.hxx"
#include "repro/TimerCMessage.hxx"
#include "repro/ProxyConfig.hxx"

namespace resip
{
class SipStack;
class Transport;
}

namespace repro
{

class UserStore;
class ProcessorChain;

class OptionsHandler
{
public:
    OptionsHandler() {}
    virtual ~OptionsHandler() {}

    // return true if handled and response should be sent, false to route normally through proxy
    virtual bool onOptionsRequest(const resip::SipMessage& request, resip::SipMessage& response) = 0;
};

class RequestContextFactory
{
   public:
      virtual RequestContext* createRequestContext(Proxy& proxy,
                                                   ProcessorChain& requestP,  // monkeys
                                                   ProcessorChain& responseP, // lemurs
                                                   ProcessorChain& targetP);  // baboons   
      virtual ~RequestContextFactory() {}
};

class Proxy : public resip::TransactionUser, public resip::ThreadIf
{
   public:
      Proxy(resip::SipStack&,
            ProxyConfig& config,
            ProcessorChain& requestP, 
            ProcessorChain& responseP,
            ProcessorChain& targetP);
      virtual ~Proxy();

      // Crypto Random Key for Salting Flow Token HMACs
      static resip::Data FlowTokenSalt;
      static resip::KeyValueStore::KeyValueStoreKeyAllocator* getGlobalKeyValueStoreKeyAllocator();
      static resip::KeyValueStore::KeyValueStoreKeyAllocator* getRequestKeyValueStoreKeyAllocator();
      static resip::KeyValueStore::KeyValueStoreKeyAllocator* getTargetKeyValueStoreKeyAllocator();
      static resip::KeyValueStore::Key allocateGlobalKeyValueStoreKey();  // should only be called at static initialization time
      static resip::KeyValueStore::Key allocateRequestKeyValueStoreKey();  // should only be called at static initialization time
      static resip::KeyValueStore::Key allocateTargetKeyValueStoreKey();  // should only be called at static initialization time

      // Note:  These are not thread safe and should be called before run() only
      void setOptionsHandler(OptionsHandler* handler);
      void setRequestContextFactory(std::auto_ptr<RequestContextFactory> requestContextFactory);

      virtual bool isShutDown() const ;
      virtual void thread();
      
      virtual bool isMyUri(const resip::Uri& uri) const;
      void addTransportRecordRoute(unsigned int transportKey, const resip::NameAddr& recordRoute);
      void removeTransportRecordRoute(unsigned int transportKey);
      const resip::NameAddr& getRecordRoute(unsigned int transportKey) const;
      bool getRecordRouteForced() const { return mRecordRouteForced; }
      void setRecordRouteForced(bool forced) { mRecordRouteForced = forced; }

      void setAssumePath(bool f) { mAssumePath = f; }
      bool getAssumePath() const { return mAssumePath; }

      bool isPAssertedIdentityProcessingEnabled() { return mPAssertedIdentityProcessing; }
      bool isNeverStripProxyAuthorizationHeadersEnabled() { return mNeverStripProxyAuthorizationHeaders; }
      
      UserStore& getUserStore();
      resip::SipStack& getStack(){return mStack;}
      ProxyConfig& getConfig(){return mConfig;}
      void send(const resip::SipMessage& msg);
      void addClientTransaction(const resip::Data& transactionId, RequestContext* rc);

      void postTimerC(std::auto_ptr<TimerCMessage> tc);

      void postMS(std::auto_ptr<resip::ApplicationMessage> msg, int msec);

      bool compressionEnabled() const;

      void addSupportedOption(const resip::Data& option);
      void removeSupportedOption(const resip::Data& option);

      void setServerText(const resip::Data& text) { mServerText = text; }
      const resip::Data& getServerText() const { return mServerText; }

      // Accessor for global extensible state storage for monkeys
      resip::KeyValueStore& getKeyValueStore() { return mKeyValueStore; }

      void doSessionAccounting(const resip::SipMessage& sip, bool received, RequestContext& context);
      void doRegistrationAccounting(repro::AccountingCollector::RegistrationEvent regEvent, const resip::SipMessage& sip);

   protected:
      virtual const resip::Data& name() const;

   private:
      resip::SipStack& mStack;
      ProxyConfig& mConfig;
      resip::NameAddr mRecordRoute;
      typedef std::map<unsigned int, resip::NameAddr> TransportRecordRouteMap;
      TransportRecordRouteMap mTransportRecordRoutes;
      mutable resip::Mutex mTransportRecordRouteMutex;

      bool mRecordRouteForced;
      bool mAssumePath;
      bool mPAssertedIdentityProcessing;
      bool mNeverStripProxyAuthorizationHeaders;
      resip::Data mServerText;
      int mTimerC;
      resip::KeyValueStore mKeyValueStore;
      
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
      std::set<resip::Data> mSupportedOptions;
      OptionsHandler* mOptionsHandler;
      std::auto_ptr<RequestContextFactory> mRequestContextFactory;

      bool mSessionAccountingEnabled;
      bool mRegistrationAccountingEnabled;
      AccountingCollector* mAccountingCollector;

      // disabled
      Proxy();
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
