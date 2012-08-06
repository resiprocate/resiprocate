#if !defined(RESIP_REPRORUNNER_HXX)
#define RESIP_REPRORUNNER_HXX 

#include "rutil/Data.hxx"
#include "rutil/ServerProcess.hxx"
#include "resip/dum/TlsPeerAuthManager.hxx"
#include <memory>

namespace resip
{
   class TransactionUser;
   class SipStack;
   class RegistrationPersistenceManager;
   class FdPollGrp;
   class AsyncProcessHandler;
   class ThreadIf;
   class DialogUsageManager;
   class CongestionManager;
}

namespace repro
{
class ProxyConfig;
class ProcessorChain;
class Dispatcher;
class AbstractDb;
class ProcessorChain;
class Proxy;
class WebAdmin;
class WebAdminThread;
class Registrar;
class CertServer;
class RegSyncClient;
class RegSyncServer;
class RegSyncServerThread;
class CommandServer;
class CommandServerThread;
class Processor;

class ReproRunner : public resip::ServerProcess
{
public:
   ReproRunner();
   virtual ~ReproRunner();

   virtual bool run(int argc, char** argv);
   virtual void shutdown();
   virtual void restart();  // brings everydown and then backup again - leaves InMemoryRegistrationDb intact

   virtual Proxy* getProxy() { return mProxy; }

protected:
   virtual void cleanupObjects();

   virtual bool createSipStack();
   virtual bool createDatastore();
   virtual bool createProxy();
   virtual void populateRegistrations();
   virtual bool createWebAdmin();
   virtual void createDialogUsageManager();
   virtual void createRegSync();
   virtual void createCommandServer();

   virtual resip::Data addDomains(resip::TransactionUser& tu, bool log);
   virtual bool addTransports(bool& allTransportsSpecifyRecordRoute);
   // Override this and examine the processor name to selectively add custom processors before or after the standard ones
   virtual void addProcessor(repro::ProcessorChain& chain, std::auto_ptr<repro::Processor> processor);
   virtual void makeRequestProcessorChain(repro::ProcessorChain& chain);
   virtual void makeResponseProcessorChain(repro::ProcessorChain& chain);
   virtual void makeTargetProcessorChain(repro::ProcessorChain& chain);

   virtual void loadCommonNameMappings();

   bool mRunning;
   bool mRestarting;
   int mArgc;
   char** mArgv;
   bool mThreadedStack;
   resip::Data mHttpRealm;
   bool mSipAuthDisabled;
   bool mUseV4;
   bool mUseV6;
   int mRegSyncPort;
   ProxyConfig* mProxyConfig;
   resip::FdPollGrp* mFdPollGrp;
   resip::AsyncProcessHandler* mAsyncProcessHandler;
   resip::SipStack* mSipStack;
   resip::ThreadIf* mStackThread;
   AbstractDb* mAbstractDb;
   AbstractDb* mRuntimeAbstractDb;
   resip::RegistrationPersistenceManager* mRegistrationPersistenceManager;
   Dispatcher* mAuthRequestDispatcher;
   Dispatcher* mAsyncProcessorDispatcher;
   ProcessorChain* mMonkeys;
   ProcessorChain* mLemurs;
   ProcessorChain* mBaboons;
   Proxy* mProxy;
   WebAdmin* mWebAdmin;
   WebAdminThread* mWebAdminThread;
   Registrar* mRegistrar;
   resip::DialogUsageManager* mDum;
   resip::ThreadIf* mDumThread;
   CertServer* mCertServer;
   RegSyncClient* mRegSyncClient;
   RegSyncServer* mRegSyncServerV4;
   RegSyncServer* mRegSyncServerV6;
   RegSyncServerThread* mRegSyncServerThread;
   CommandServer* mCommandServerV4;
   CommandServer* mCommandServerV6;
   CommandServerThread* mCommandServerThread;
   resip::CongestionManager* mCongestionManager;
   resip::CommonNameMappings mCommonNameMappings;
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
