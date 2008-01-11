#ifdef WIN32
#  define usleep(t) Sleep(t)
#endif

#if defined (HAVE_POPT_H)
#include <popt.h>
#endif

#include <signal.h>

#include "resip/stack/NameAddr.hxx"
#include "resip/stack/Pkcs8Contents.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Symbols.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/X509Contents.hxx"
#include "resip/dum/AppDialogSet.hxx"
#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/DumShutdownHandler.hxx"
#include "resip/dum/OutOfDialogHandler.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/PublicationHandler.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/ServerPublication.hxx"
#include "resip/dum/ServerSubscription.hxx"
#include "resip/dum/SubscriptionHandler.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"
#include "rutil/Subsystem.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

using namespace std;
using namespace resip;

static bool finished=false;

void
signalHandler(int signo)
{
   std::cerr << "Shutting down" << endl;
   finished = true;
}

// When a publish comes in, we should let any outstanding subscriptions know
// about it. 

class CertSubscriptionHandler;
class PrivateKeySubscriptionHandler;

class CertPublicationHandler : public ServerPublicationHandler
{
   public:
      CertPublicationHandler(Security& security) : mSecurity(security)
      {
      }

      virtual void onInitial(ServerPublicationHandle h, 
                             const Data& etag, 
                             const SipMessage& pub, 
                             const Contents* contents,
                             const SecurityAttributes* attrs, 
                             int expires)
      {
         add(h, contents);
      }

      virtual void onExpired(ServerPublicationHandle h, const Data& etag)
      {
         mSecurity.removeUserCert(h->getPublisher());
      }

      virtual void onRefresh(ServerPublicationHandle, 
                             const Data& etag, 
                             const SipMessage& pub, 
                             const Contents* contents,
                             const SecurityAttributes* attrs,
                             int expires)
      {
      }

      virtual void onUpdate(ServerPublicationHandle h, 
                            const Data& etag, 
                            const SipMessage& pub, 
                            const Contents* contents,
                            const SecurityAttributes* attrs,
                            int expires)
      {
         add(h, contents);
      }

      virtual void onRemoved(ServerPublicationHandle h, const Data& etag, const SipMessage& pub, int expires)
      {
         mSecurity.removeUserCert(h->getPublisher());
      }
   private:
      void add(ServerPublicationHandle h, const Contents* contents)
      {
         if (h->getDocumentKey() == h->getPublisher())
         {
            const X509Contents* x509 = dynamic_cast<const X509Contents*>(contents);
            assert(x509);
            mSecurity.addUserCertDER(h->getPublisher(), x509->getBodyData());
            h->send(h->accept(200));
         }
         else
         {
            h->send(h->accept(403)); // !jf! is this the correct code? 
         }
      }

      Security& mSecurity;
};

class PrivateKeyPublicationHandler : public ServerPublicationHandler
{
   public:
      PrivateKeyPublicationHandler(Security& security) : mSecurity(security)
      {
      }

      virtual void onInitial(ServerPublicationHandle h, 
                             const Data& etag, 
                             const SipMessage& pub, 
                             const Contents* contents,
                             const SecurityAttributes* attrs, 
                             int expires)
      {
         add(h, contents);
      }

      virtual void onExpired(ServerPublicationHandle h, const Data& etag)
      {
         mSecurity.removeUserPrivateKey(h->getPublisher());
      }

      virtual void onRefresh(ServerPublicationHandle, 
                             const Data& etag, 
                             const SipMessage& pub, 
                             const Contents* contents,
                             const SecurityAttributes* attrs,
                             int expires)
      {
      }

      virtual void onUpdate(ServerPublicationHandle h, 
                            const Data& etag, 
                            const SipMessage& pub, 
                            const Contents* contents,
                            const SecurityAttributes* attrs,
                            int expires)
      {
         add(h, contents);
      }

      virtual void onRemoved(ServerPublicationHandle h, const Data& etag, const SipMessage& pub, int expires)
      {
         mSecurity.removeUserPrivateKey(h->getPublisher());
      }

   private:
      void add(ServerPublicationHandle h, const Contents* contents)
      {
         if (h->getDocumentKey() == h->getPublisher())
         {
            const Pkcs8Contents* pkcs8 = dynamic_cast<const Pkcs8Contents*>(contents);
            assert(pkcs8);
            mSecurity.addUserPrivateKeyDER(h->getPublisher(), pkcs8->getBodyData());
         }
         else
         {
            h->send(h->accept(403)); // !jf! is this the correct code? 
         }
      }
      
      Security& mSecurity;
};

class CertSubscriptionHandler : public ServerSubscriptionHandler
{
   public:
      CertSubscriptionHandler(Security& security) : mSecurity(security)
      {
      }

      virtual void onNewSubscription(ServerSubscriptionHandle h, const SipMessage& sub)
      {
         if (!mSecurity.hasUserCert(h->getDocumentKey()))
         {
            // !jf! really need to do this async. send neutral state in the meantime,
            // blah blah blah
            mSecurity.generateUserCert(h->getDocumentKey());
         }

         if (mSecurity.hasUserCert(h->getDocumentKey()))
         {
            X509Contents x509(mSecurity.getUserCertDER(h->getDocumentKey()));
            h->send(h->update(&x509));
         }
         else
         {
            h->reject(404);
         }
      }

      virtual void onPublished(ServerSubscriptionHandle associated, 
                               ServerPublicationHandle publication, 
                               const Contents* contents,
                               const SecurityAttributes* attrs)
      {
         associated->send(associated->update(contents));
      }
      

      virtual void onTerminated(ServerSubscriptionHandle)
      {
      }

      virtual void onError(ServerSubscriptionHandle, const SipMessage& msg)
      {
      }

   private:
      Security& mSecurity;
};

class PrivateKeySubscriptionHandler : public ServerSubscriptionHandler
{
   public:
      PrivateKeySubscriptionHandler(Security& security) : mSecurity(security)
      {
      }

      virtual void onNewSubscription(ServerSubscriptionHandle h, const SipMessage& sub)
      {
         if (h->getDocumentKey() != h->getSubscriber())
         {
            h->send(h->accept(403)); // !jf! is this the correct code? 
         }
         else if (mSecurity.hasUserCert(h->getDocumentKey()))
         {
            Pkcs8Contents pkcs(mSecurity.getUserPrivateKeyDER(h->getDocumentKey()));
            h->send(h->update(&pkcs));
         }
         else
         {
            h->reject(404);
         }
      }

      virtual void onPublished(ServerSubscriptionHandle associated, 
                               ServerPublicationHandle publication, 
                               const Contents* contents,
                               const SecurityAttributes* attrs)
      {
         associated->send(associated->update(contents));
      }

      virtual void onTerminated(ServerSubscriptionHandle)
      {
      }

      virtual void onError(ServerSubscriptionHandle, const SipMessage& msg)
      {
      }

   private:
      Security& mSecurity;
};
   
   

class CertServer : public OutOfDialogHandler,  public DialogUsageManager
{
   public:
      CertServer(const resip::NameAddr& me, SipStack& stack) : 
         DialogUsageManager(stack),
         mCertServer(*getSecurity()),
         mPrivateKeyServer(*getSecurity()),
         mCertUpdater(*getSecurity()),
         mPrivateKeyUpdater(*getSecurity()),
         mDone(false)
      {
         addTransport(UDP, 5100);
         addTransport(TCP, 5100);
         addTransport(TLS, 5101, V4, Data::Empty, me.uri().host(), Data::Empty);
         
         mProfile = new MasterProfile;
         mProfile->clearSupportedMethods();
         mProfile->addSupportedMethod(PUBLISH);
         mProfile->addSupportedMethod(SUBSCRIBE);
         mProfile->validateAcceptEnabled() = true;
         mProfile->validateContentEnabled() = true;
         mProfile->addSupportedMimeType(PUBLISH, Pkcs8Contents::getStaticType());
         mProfile->addSupportedMimeType(SUBSCRIBE, Pkcs8Contents::getStaticType());
         mProfile->addSupportedMimeType(PUBLISH, X509Contents::getStaticType());
         mProfile->addSupportedMimeType(SUBSCRIBE, X509Contents::getStaticType());
         
         mProfile.setDefaultFrom(me);
         setMasterProfile(mProfile);

         addServerSubscriptionHandler(Symbols::Credential, &mPrivateKeyServer);
         addServerSubscriptionHandler(Symbols::Certificate, &mCertServer);
         addServerPublicationHandler(Symbols::Credential, &mPrivateKeyUpdater);
         addServerPublicationHandler(Symbols::Certificate, &mCertUpdater);
         addOutOfDialogHandler(OPTIONS, this);
         
         //setServerAuthManager(std::auto_ptr<ServerAuthManager>(new ServerAuthManager(mProfile)));

         DialogUsageManager::run();
      }

      ~CertServer()
      {
      }
      
      void run()
      {
         while ( !mDone )
         {
            while (process());
            usleep(5);

            if (finished)
            {
               // graceful shutdown
               exit(0);
            }
         }
      }

      virtual void onSuccess(ClientOutOfDialogReqHandle, const SipMessage& successResponse)
      {
      }
      
      virtual void onFailure(ClientOutOfDialogReqHandle, const SipMessage& errorResponse)
      {
      }

      virtual void onReceivedRequest(ServerOutOfDialogReqHandle, const SipMessage& request)
      {
      }

   private:
      SharedPtr<MasterProfile> mProfile;
      CertSubscriptionHandler mCertServer;
      PrivateKeySubscriptionHandler mPrivateKeyServer;
      CertPublicationHandler mCertUpdater;
      PrivateKeyPublicationHandler mPrivateKeyUpdater;
      bool mDone;
};

int 
main (int argc, char** argv)
{
   char* logType = "COUT";
   char* logLevel = "DEBUG";
   char* myUrl = "sip:localhost:7001";
   char* bindAddr = 0;
   int v6 = 0;

#if defined(HAVE_POPT_H)
   struct poptOption table[] = {
      {"log-type",    'l', POPT_ARG_STRING, &logType,   0, "where to send logging messages", "syslog|cerr|cout"},
      {"log-level",   'v', POPT_ARG_STRING, &logLevel,  0, "specify the default log level", "DEBUG|INFO|WARNING|ALERT"},
      {"name" ,       'n', POPT_ARG_STRING, &myUrl,     0, "my url", 0},
      {"bind",        'b', POPT_ARG_STRING, &bindAddr,  0, "interface address to bind to",0},
      {"v6",          '6', POPT_ARG_NONE,   &v6     ,   0, "ipv6", 0},
      POPT_AUTOHELP
      { NULL, 0, 0, NULL, 0 }
   };
   poptContext context = poptGetContext(NULL, argc, const_cast<const char**>(argv), table, 0);
   poptGetNextOpt(context);
#endif
   Log::initialize(logType, logLevel, argv[0]);

#ifndef _WIN32
   if ( signal( SIGPIPE, SIG_IGN) == SIG_ERR)
   {
      cerr << "Couldn't install signal handler for SIGPIPE" << endl;
      exit(-1);
   }

   if ( signal( SIGINT, signalHandler ) == SIG_ERR )
   {
      cerr << "Couldn't install signal handler for SIGINT" << endl;
      exit( -1 );
   }

   if ( signal( SIGTERM, signalHandler ) == SIG_ERR )
   {
      cerr << "Couldn't install signal handler for SIGTERM" << endl;
      exit( -1 );
   }
#endif

   NameAddr domain(myUrl);
   SipStack stack;
   CertServer server(domain, stack);
   server.run();
   return 0;
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
