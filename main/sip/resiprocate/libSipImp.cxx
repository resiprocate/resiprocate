#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "libSipImp.h"

#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/TuIM.hxx"
#include "resiprocate/Security.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

static  SipStack* sipStack;
static  TuIM* tuIM;


class TestCallback: public TuIM::Callback
{
   public:
      virtual void presenseUpdate(const Uri& dest, bool open, const Data& status );
      virtual void receivedPage( const Data& msg, const Uri& from ,
                                 const Data& signedBy,  Security::SignatureStatus sigStatus,
                                 bool wasEncryped  );
      virtual void sendPageFailed( const Uri& dest,int respNumber );
      virtual void registrationFailed(const resip::Uri&, int respNumber); 
      virtual void registrationWorked(const Uri& dest );
      virtual void receivePageFailed(const Uri& sender);
};
  

void 
TestCallback::presenseUpdate(const Uri& from, bool open, const Data& status )
{
}

void 
TestCallback::receivedPage( const Data& msg, const Uri& from,
                            const Data& signedBy,  Security::SignatureStatus sigStatus,
                            bool wasEncryped  )
{  
}


void 
TestCallback::sendPageFailed( const Uri& target, int respNum )
{
}


void 
TestCallback::receivePageFailed( const Uri& target )
{
}


void 
TestCallback::registrationFailed(const resip::Uri& target, int respNum )
{
   Data num(respNum);
}
  
                              
void 
TestCallback::registrationWorked(const resip::Uri& target)
{
}
  
      

void 
libSipImp_Init()
{  
   Log::initialize(Log::COUT, Log::ERR, "libSimpImp");
   Log::setLevel(Log::ERR);

   InfoLog(<<"Test Driver for IM Starting");
    
   int port = 5060;

#ifdef USE_SSL
   int tlsPort = 0;
   bool tlsServer=false;
   bool useTls = true;
#endif
   
   Uri aor;
   bool haveAor=false;
   Uri  dest = Uri("sip:nobody@example.com");
   Data aorPassword;
   Uri contact("sip:user@localhost");
   bool haveContact=false;
   Uri outbound;
   bool noRegister = false;
   
   int numAdd=0;
   Data addList[100];

   Data key("password");
   //InfoLog( << "Using port " << port );

#ifdef USE_SSL
   Security*  security = new Security( tlsServer, useTls ); // !cj! mem leak 
   SipStack* sipStack = new SipStack( false /*multihtread*/, security );  

   assert( sipStack->security );
   bool ok = sipStack->security->loadAllCerts( key , Data::Empty );
   if ( !ok )
   {
      InfoLog( << "Could not load the certificates" );
   }
#else 
   SipStack* sipStack = new SipStack( false /*multihtread*/ );  
#endif
   
   if (port!=0)
   {
      sipStack->addTransport(Transport::UDP, port);
      sipStack->addTransport(Transport::TCP, port);
   }
   
#if USE_SSL
   if ( port == 5060 )
   {
      if ( tlsPort == 0 )
      {
         tlsPort = 5061;
      }
   }
   if ( tlsPort != 0 )
   {
      sipStack->addTransport(Transport::TLS, tlsPort);
   }
#endif

   if (!haveContact)
   {
      contact.port() = port;
      contact.host() = sipStack->getHostname();
   }
   
   if ( haveAor )
   {
      if (!haveContact)
      {
         contact.user() = aor.user();
#if USE_SSL
         if ( aor.scheme() == "sips" )
         {
            contact.scheme() = aor.scheme();
            contact.port() = tlsPort;
         }
#endif
      }
   }
   else
   {
      aor = contact;
   }

   InfoLog( << "aor is " << aor );
   InfoLog( << "contact is " << contact );
   TestCallback callback;
   TuIM*  tuIM = new TuIM(sipStack,aor,contact,&callback);

   Data name("SIPimp.org/0.2.2 (fire)");
   tuIM->setUAName( name );
      
   if ( !outbound.host().empty() )
   {
      tuIM->setOutboundProxy( outbound );
   }
   
   if ( haveAor )
   {
      if ( !noRegister )
      {
         tuIM->registerAor( aor, aorPassword );
      }
   }
   
  
   for ( int i=0; i<numAdd; i++ )
   { 
      Uri uri(addList[i]);
      tuIM->addBuddy( uri, Data::Empty );
   }
}


void 
libSipImp_Process()
{  
   assert( sipStack );
   assert( tuIM );
   
   FdSet fdset; 
   sipStack->buildFdSet(fdset);
   int time = sipStack->getTimeTillNextProcessMS();
   
   time = 0;
   int  err = fdset.selectMilliSeconds( time );
   
   if ( err == -1 )
   {
      int e = errno;
      switch (e)
      {
         case 0:
            break;
         default:
            //InfoLog(<< "Error " << e << " " << strerror(e) << " in select");
            break;
      }
   }
   if ( err == 0 )
   {
      //cerr << "select timed out" << endl;
   }
   if ( err > 0 )
   {
      //cerr << "select has " << err << " fd ready" << endl;
   }
   
   sipStack->process(fdset);
   
   tuIM->process();       
}



void 
libSipImp_SendMessage( char* destStr , char* msgStr )
{
   assert( tuIM );
   
   Uri uri;
   uri = Uri(Data(destStr));
   
   Data msg(msgStr);
   Data encFor = Data::Empty;
   
   tuIM->sendPage(msg,uri,false/*sign*/,encFor/*encrypt for*/);
}

