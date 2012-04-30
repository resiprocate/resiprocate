#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "resip/stack/ExtensionHeader.hxx"
#include "resip/stack/HeaderTypes.hxx"
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
#include "RegConfig.hxx"
#include "UserRegistrationClient.hxx"
#include "KeyedFile.hxx"

#define RESIPROCATE_SUBSYSTEM AppSubsystem::REGISTRATIONCLIENT

#define DEFAULT_CONFIG_FILE "registrationClient.config"

using namespace registrationclient;
using namespace resip;
using namespace std;

volatile bool mustReload = false;

static void
signalHandler(int signo)
{
#ifndef WIN32
   if(signo == SIGHUP)
   {
      InfoLog(<<"Received HUP signal, logger reset");
      Log::reset();
      mustReload = true;
      return;
   }
#endif
   WarningLog(<<"Unexpected signal, ignoring it: " << signo);
}

class MyClientRegistrationAgent : public ServerProcess
{
   public:
      MyClientRegistrationAgent() {};
      ~MyClientRegistrationAgent() {};

      void run(int argc, char **argv)
      {
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

         Data loggingType = cfg.getConfigData("LoggingType", "cout", true);
         Data logLevel = cfg.getConfigData("LogLevel", "INFO", true);
         Data logFilename = cfg.getConfigData("LogFilename", "registrationClient.log", true);
         Log::initialize(loggingType, logLevel, argv[0], logFilename.c_str(), 0);
#ifndef WIN32
         if ( signal( SIGHUP, signalHandler ) == SIG_ERR )
         {
            ErrLog(<<"Couldn't install signal handler for SIGHUP");
            exit(-1);
         }
#endif

         InfoLog(<<"Starting client registration agent");

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
         SipStack stack(security);
#else
         SipStack stack;
#endif

         DialogUsageManager clientDum(stack);
         SharedPtr<MasterProfile> profile(new MasterProfile);
         auto_ptr<ClientAuthManager> clientAuth(new ClientAuthManager);

         stack.addTransport(UDP, 0, V4);
         // stack.addTransport(UDP, 0, V6);
         stack.addTransport(TCP, 0, V4);
         // stack.addTransport(TCP, 0, V6);
#ifdef USE_SSL
         stack.addTransport(TLS, 0, V4);
         // stack.addTransport(TLS, 0, V6);
#endif
         clientDum.setMasterProfile(profile);
         clientDum.setClientAuthManager(clientAuth);
         clientDum.getMasterProfile()->setDefaultRegistrationTime(cfg.getConfigInt("RegistrationExpiry", 3600));
         // Retry every 60 seconds after a hard failure:
         clientDum.getMasterProfile()->setDefaultRegistrationRetryTime(60);

         // keep alive test.
         auto_ptr<KeepAliveManager> keepAlive(new KeepAliveManager);
         clientDum.setKeepAliveManager(keepAlive);

         profile->addSupportedOptionTag(Token(Symbols::Outbound));
         profile->addSupportedOptionTag(Token(Symbols::Path));

         Data outboundProxy(cfg.getConfigData("OutboundProxy", "", true));
         if(!outboundProxy.empty())
         {
            const Uri _outboundProxy(outboundProxy);
            profile->setOutboundProxy(_outboundProxy);
         }

         SharedPtr<UserAccountFileRowHandler> rowHandler(new UserAccountFileRowHandler(clientDum));
         SharedPtr<KeyedFile> kf(new KeyedFile(cfg.getConfigData("UserAccountFile", "users.txt", false), SharedPtr<KeyedFileRowHandler>(rowHandler, dynamic_cast_tag())));
         kf->setSharedPtr(kf);
         UserRegistrationClient clientHandler(kf);
         clientDum.setClientRegistrationHandler(&clientHandler);
         rowHandler->setUserRegistrationClient(&clientHandler);
         kf->doReload();

         int n = 0;
         while ( true )
         {
            stack.process(100);
            while(clientDum.process());
            if(mustReload)
            {
               kf->doReload();
               mustReload = false;
            }
         }
       }
};

int
main(int argc, char** argv)
{
   MyClientRegistrationAgent agent;
   agent.run(argc, argv);
}

/* ====================================================================
 *
 * Copyright 2012 Daniel Pocock http://danielpocock.com  All rights reserved.
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
