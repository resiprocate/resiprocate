#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "resip/stack/ExtensionHeader.hxx"
#include "resip/stack/HeaderTypes.hxx"
#include "resip/stack/InteropHelper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ServerProcess.hxx"
#include "rutil/Subsystem.hxx"
#include "resip/dum/KeepAliveManager.hxx"

#if defined (USE_SSL)
#if defined(WIN32) 
#include "resip/stack/ssl/WinSecurity.hxx"
#else
#include "resip/stack/ssl/Security.hxx"
#endif
#endif

#ifndef WIN32
#include <signal.h>
#endif

#include "AppSubsystem.hxx"
#include "CommandThread.hxx"
#include "RegConfig.hxx"
#include "SNMPThread.hxx"
#include "UserRegistrationClient.hxx"
#include "KeyedFile.hxx"

#include <utility>

#define RESIPROCATE_SUBSYSTEM AppSubsystem::REGISTRATIONAGENT

#define DEFAULT_CONFIG_FILE "registrationAgent.config"

using namespace registrationagent;
using namespace resip;
using namespace std;

class MyClientRegistrationAgent : public ServerProcess
{
   public:
      MyClientRegistrationAgent() = default;
      ~MyClientRegistrationAgent() = default;

      void run(int argc, char **argv)
      {
         installSignalHandler();
         Data defaultConfigFile(DEFAULT_CONFIG_FILE);
         RegConfig cfg;
         try
         {
            cfg.parseConfig(argc, argv, defaultConfigFile);
         }
         catch(BaseException& ex)
         {
            std::cerr << "Error parsing configuration: " << ex << std::endl;
            syslog(LOG_DAEMON | LOG_CRIT, "%s", ex.getMessage().c_str());
            exit(1);
         }

         setPidFile(cfg.getConfigData("PidFile", "", true));
         if(cfg.getConfigBool("Daemonize", false))
         {
            daemonize();
         }

         Log::initialize(cfg, argv[0]);

         InfoLog(<<"Starting client registration agent");

         bool rport = cfg.getConfigBool("AddViaRport", true);
         InteropHelper::setRportEnabled(rport);

#ifdef USE_SSL
         Data certPath = cfg.getConfigData("CertificatePath", Data::Empty);
         Security* security;
#ifdef WIN32
         if(certPath.empty())
         {
            security = new WinSecurity;
         }
         else
         {
            security = new WinSecurity(certPath);
         }
#else
         if(certPath.empty())
         {
            security = new Security;
         }
         else
         {
            security = new Security(certPath);
         }
#endif
         Data caDir, caFile;
         cfg.getConfigValue("CADirectory", caDir);
         if(!caDir.empty())
         {
            security->addCADirectory(caDir);
         }
         cfg.getConfigValue("CAFile", caFile);
         if(!caFile.empty())
         {
            security->addCAFile(caFile);
         }
         mStack = std::make_shared<SipStack>(security);
#else
         mStack = std::make_shared<SipStack>();
#endif

         mClientDum = std::make_shared<DialogUsageManager>(*mStack);
         const auto profile = std::make_shared<MasterProfile>();
         std::unique_ptr<ClientAuthManager> clientAuth(new ClientAuthManager);

         mStack->addTransport(UDP, 0, V4);
         // mStack->addTransport(UDP, 0, V6);
         mStack->addTransport(TCP, 0, V4);
         // mStack->addTransport(TCP, 0, V6);
#ifdef USE_SSL
         mStack->addTransport(TLS, 0, V4);
         // mStack->addTransport(TLS, 0, V6);
#endif

         // Drop privileges (can do this now that sockets are bound)
         Data runAsUser = cfg.getConfigData("RunAsUser", Data::Empty, true);
         Data runAsGroup = cfg.getConfigData("RunAsGroup", Data::Empty, true);
         if(!runAsUser.empty())
         {
            InfoLog( << "Trying to drop privileges, configured uid = " << runAsUser << " gid = " << runAsGroup);
            dropPrivileges(runAsUser, runAsGroup);
         }

         mClientDum->setMasterProfile(profile);
         mClientDum->setClientAuthManager(std::move(clientAuth));
         mClientDum->getMasterProfile()->setDefaultRegistrationTime(cfg.getConfigInt("RegistrationExpiry", 3600));
         // Retry every 60 seconds after a hard failure:
         mClientDum->getMasterProfile()->setDefaultRegistrationRetryTime(60);
         mClientDum->getMasterProfile()->setUserAgent("reSIProcate registrationAgent");

         // keep alive test.
         std::unique_ptr<KeepAliveManager> keepAlive(new KeepAliveManager);
         mClientDum->setKeepAliveManager(std::move(keepAlive));

         profile->setRportEnabled(rport);

         profile->addSupportedOptionTag(Token(Symbols::Outbound));
         profile->addSupportedOptionTag(Token(Symbols::Path));

         Data outboundProxy(cfg.getConfigData("OutboundProxy", "", true));
         if(!outboundProxy.empty())
         {
            const Uri _outboundProxy(outboundProxy);
            profile->setOutboundProxy(_outboundProxy);
         }

         const auto rowHandler = std::make_shared<UserAccountFileRowHandler>(*mClientDum);
         mKeyedFile = std::make_shared<KeyedFile>(cfg.getConfigData("UserAccountFile", "users.txt", false), rowHandler);
         mKeyedFile->setSharedPtr(mKeyedFile);
         mClientHandler = std::make_shared<UserRegistrationClient>(mKeyedFile);
         mClientDum->setClientRegistrationHandler(mClientHandler.get());
         rowHandler->setUserRegistrationClient(mClientHandler);
         mKeyedFile->doReload();

         Data brokerURL(cfg.getConfigData("BrokerURL", "", true));
         if(!brokerURL.empty())
         {
            mProton.reset(new ProtonThreadBase());
            mCmd.reset(new CommandThread(brokerURL.c_str()));
            mProton->addReceiver(mCmd);
            mProton->run();
         }

         // FIXME - conditional start
         Data snmpSocket(cfg.getConfigData("SNMPMasterSocket", "", true));
         if(!snmpSocket.empty())
         {
            mSnmp.reset(new SnmpThread(snmpSocket.c_str()));
            mSnmp->run();
         }

         mainLoop();

         if(mCmd.get())
         {
            mProton->shutdown();
            mProton->join();
         }

         if(mSnmp.get())
         {
            mSnmp->shutdown();
            mSnmp->join();
         }
      }

      void doWait()
      {
         mStack->process(100);
      }

      void onLoop()
      {
         while(mClientDum->process());
         if(mCmd.get())
         {
            mCmd->processQueue(*mClientHandler);
         }
         if(mSnmp.get())
         {
            mSnmp->setAccountsTotal(mClientHandler->getAccountsTotal());
            mSnmp->setAccountsFailed(mClientHandler->getAccountsFailed());
         }
      }

      void onReload()
      {
         mKeyedFile->doReload();
      }

   private:
      std::shared_ptr<SipStack> mStack;
      std::shared_ptr<DialogUsageManager> mClientDum;
      std::shared_ptr<KeyedFile> mKeyedFile;
      std::shared_ptr<UserRegistrationClient> mClientHandler;
      std::shared_ptr<ProtonThreadBase> mProton;
      std::shared_ptr<CommandThread> mCmd;
      std::shared_ptr<SnmpThread> mSnmp;

};

int
main(int argc, char** argv)
{
   MyClientRegistrationAgent agent;
   agent.run(argc, argv);
}

/* ====================================================================
 *
 * Copyright (C) 2012-2022 Daniel Pocock https://danielpocock.com
 * Copyright (C) 2022 Software Freedom Institute SA https://softwarefreedom.institute
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */
