#ifdef WIN32
#  define usleep(t) Sleep(t)
#endif

#if defined (HAVE_POPT_H)
#include <popt.h>
#endif

#include <signal.h>

#include "resiprocate/NameAddr.hxx"
#include "resiprocate/Pkcs8Contents.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Symbols.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/X509Contents.hxx"
#include "resiprocate/dum/AppDialogSet.hxx"
#include "resiprocate/dum/ClientAuthManager.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/DumShutdownHandler.hxx"
#include "resiprocate/dum/OutOfDialogHandler.hxx"
#include "resiprocate/dum/MasterProfile.hxx"
#include "resiprocate/dum/PublicationHandler.hxx"
#include "resiprocate/dum/RegistrationHandler.hxx"
#include "resiprocate/dum/ServerPublication.hxx"
#include "resiprocate/dum/ServerSubscription.hxx"
#include "resiprocate/dum/SubscriptionHandler.hxx"
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
      CertServer(const resip::NameAddr& me) : 
         DialogUsageManager(),
         mCertServer(getSecurity()),
         mPrivateKeyServer(getSecurity()),
         mCertUpdater(getSecurity()),
         mPrivateKeyUpdater(getSecurity()),
         mDone(false)
      {
         addTransport(UDP, 5100);
         addTransport(TCP, 5100);
         addTransport(TLS, 5101, V4, Data::Empty, me.uri().host(), Data::Empty);
         
         mProfile.clearSupportedMethods();
         mProfile.addSupportedMethod(PUBLISH);
         mProfile.addSupportedMethod(SUBSCRIBE);
         mProfile.validateAcceptEnabled() = true;
         mProfile.validateContentEnabled() = true;
         mProfile.addSupportedMimeType(Pkcs8Contents::getStaticType());
         mProfile.addSupportedMimeType(X509Contents::getStaticType());
         
         mProfile.setDefaultFrom(me);
         setMasterProfile(&mProfile);

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
      MasterProfile mProfile;
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
   CertServer server(domain);
   server.run();
   return 0;
}

