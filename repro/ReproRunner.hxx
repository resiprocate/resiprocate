#if !defined(RESIP_REPRORUNNER_HXX)
#define RESIP_REPRORUNNER_HXX 

#include "rutil/Data.hxx"

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

class ReproRunner
{
public:
   ReproRunner();
   virtual ~ReproRunner();

   virtual bool run(int argc, char** argv);
   virtual void shutdown();

protected:
   virtual void cleanupObjects();

   virtual bool createSipStack();
   virtual bool createProxy();
   virtual void populateRegistrations();
   virtual bool createWebAdmin();
   virtual void createDialogUsageManager();
   virtual void createRegSync();
   virtual void createCommandServer();

   virtual resip::Data addDomains(resip::TransactionUser& tu, bool log);
   virtual bool addTransports(bool& allTransportsSpecifyRecordRoute);
   virtual void makeRequestProcessorChain(repro::ProcessorChain& chain);
   virtual void makeResponseProcessorChain(repro::ProcessorChain& chain);
   virtual void makeTargetProcessorChain(repro::ProcessorChain& chain);

   bool mRunning;
   bool mThreadedStack;
   resip::Data mHttpRealm;
   bool mSipAuthDisabled;
   bool mUseV4;
   bool mUseV6;
   int mRegSyncPort;
   repro::ProxyConfig* mProxyConfig;
   resip::FdPollGrp* mFdPollGrp;
   resip::AsyncProcessHandler* mAsyncProcessHandler;
   resip::SipStack* mSipStack;
   resip::ThreadIf* mStackThread;
   repro::AbstractDb* mAbstractDb;
   resip::RegistrationPersistenceManager* mRegistrationPersistenceManager;
   repro::Dispatcher* mAuthRequestDispatcher;
   repro::Dispatcher* mAsyncProcessorDispatcher;
   repro::ProcessorChain* mMonkeys;
   repro::ProcessorChain* mLemurs;
   repro::ProcessorChain* mBaboons;
   repro::Proxy* mProxy;
   repro::WebAdmin* mWebAdmin;
   repro::WebAdminThread* mWebAdminThread;
   repro::Registrar* mRegistrar;
   resip::DialogUsageManager* mDum;
   resip::ThreadIf* mDumThread;
   repro::CertServer* mCertServer;
   repro::RegSyncClient* mRegSyncClient;
   repro::RegSyncServer* mRegSyncServerV4;
   repro::RegSyncServer* mRegSyncServerV6;
   repro::RegSyncServerThread* mRegSyncServerThread;
   repro::CommandServer* mCommandServerV4;
   repro::CommandServer* mCommandServerV6;
   repro::CommandServerThread* mCommandServerThread;
   resip::CongestionManager* mCongestionManager;
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
