#ifdef WIN32
#  define usleep(t) Sleep(t)
#endif

#if defined (HAVE_POPT_H)
#include <popt.h>
#endif

#include <signal.h>

#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Symbols.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/NameAddr.hxx"
#include "resiprocate/Pkcs8Contents.hxx"
#include "resiprocate/X509Contents.hxx"
#include "resiprocate/dum/AppDialogSet.hxx"
#include "resiprocate/dum/ServerSubscription.hxx"
#include "resiprocate/dum/ServerPublication.hxx"
#include "resiprocate/dum/ClientAuthManager.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/OutOfDialogHandler.hxx"
#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/dum/RegistrationHandler.hxx"
#include "resiprocate/dum/SubscriptionHandler.hxx"
#include "resiprocate/dum/PublicationHandler.hxx"
#include "resiprocate/dum/DumShutdownHandler.hxx"
#include "resiprocate/os/Log.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Random.hxx"
#include "resiprocate/os/Subsystem.hxx"

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

class PublicationHandler : public ServerPublicationHandler
{
   public:
      PublicationHandler(Security& security) : mSecurity(security)
      {
      }

      virtual void onInitial(ServerPublicationHandle h, const Data& etag, const SipMessage& pub, int expires)
      {
         Contents* contents=0;
         add(h, contents);
      }

      virtual void onExpired(ServerPublicationHandle h, const Data& etag)
      {
         removeUserCertDER(h->getPublisher());
      }

      virtual void onRefresh(ServerPublicationHandle, const Data& etag, const SipMessage& pub, int expires)
      {
      }

      virtual void onUpdate(ServerPublicationHandle h, const Data& etag, const SipMessage& pub, int expires)
      {
         Contents* contents=0;
         add(h, contents);
      }

      virtual void onRemoved(ServerPublicationHandle, const Data& etag, const SipMessage& pub, int expires)
      {
         removeUserCertDER(h->getPublisher());
      }
   private:
      void add(ServerPublicationHandle h, Contents* contents)
      {
         X509Contents* x509 = dynamic_cast<X509Contents*>(contents);
         assert(x509);
         addUserCertDER(h->getPublisher(), x509->getBodyData());
      }

      Security& mSecurity;
};

class PrivateKeyPublicationHandler : public ServerPublicationHandler
{
   public:
      PrivateKeyPublicationHandler(Security& security) : mSecurity(security)
      {
      }

      virtual void onInitial(ServerPublicationHandle h, const Data& etag, const SipMessage& pub, int expires)
      {
         Contents* contents=0;
         add(h, contents);
      }

      virtual void onExpired(ServerPublicationHandle h, const Data& etag)
      {
         removeUserPrivateKeyDER(h->getPublisher());
      }

      virtual void onRefresh(ServerPublicationHandle, const Data& etag, const SipMessage& pub, int expires)
      {
      }

      virtual void onUpdate(ServerPublicationHandle, const Data& etag, const SipMessage& pub, int expires)
      {
         Contents* contents=0;
         add(h, contents);
      }

      virtual void onRemoved(ServerPublicationHandle, const Data& etag, const SipMessage& pub, int expires)
      {
         removeUserPrivateKeyDER(h->getPublisher());
      }

   private:
      void add(ServerPublicationHandle h, Contents* contents)
      {
         Pkcs8Contents* pkcs8 = dynamic_cast<Pkcs8Contents*>(contents);
         assert(pkcs8);
         addUserPrivateKeyDER(h->getPublisher(), pkcs8->getBodyData())
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
         if (mSecurity.hasUserCert(h->getPublisher()))
         {
            X509Contents x509(mSecurity.getUserCertDER(h->getPublisher()));
            h->send(h->update(&x509));
         }
         else
         {
            h->reject(404);
         }
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

      virtual void onNewSubscription(ServerSubscriptionHandle, const SipMessage& sub)
      {
         if (mSecurity.hasUserCert(h->getPublisher()))
         {
            Pkcs8Contents pkcs(mSecurity.getUserPrivateKeyDER(h->getPublisher()));
            h->send(h->update(&pkcs));
         }
         else
         {
            h->reject(404);
         }
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
      CertServer(const resip::NameAddr& me) : 
         DialogUsageManager(),
         mCertUpdater(getSecurity()),
         mPrivateKeyUpdater(getSecurity()),
         mCertServer(getSecurity()),
         mPrivateKeyServer(getSecurity())
      {
         addTransport(UDP, 5100);
         addTransport(TCP, 5100);
         // addTlsTransport
         
         mProfile.clearSupportedMethods();
         mProfile.addSupportedMethod(PUBLISH);
         mProfile.addSupportedMethod(SUBSCRIBE);
         mProfile.validateAcceptEnabled() = true;
         mProfile.validateContentEnabled() = true;
         mProfile.addSupportedMimeType(Pkcs8Contents::getStaticType());
         mProfile.addSupportedMimeType(X509Contents::getStaticType());
         
         mProfile.setDefaultFrom(me);
         setProfile(&mProfile);

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
      Profile mProfile;
      CertPublicationHandler mCertUpdater;
      PrivateKeyPublicationHandler mPrivateKeyUpdater;
      CertSubscriptionHandler mCertServer;
      PrivateKeySubscriptionHandler mPrivateKeyServer;
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
   CertServer server(domain);
   server.run();
   return 0;
}

