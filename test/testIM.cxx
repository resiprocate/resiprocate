
#include <iostream>

#ifndef WIN32
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include "sip2/util/Socket.hxx"
#include "sip2/util/Logger.hxx"

#include "sip2/sipstack/SipStack.hxx"
#include "sip2/sipstack/Uri.hxx"
#include "sip2/sipstack/TuIM.hxx"
#include "sip2/sipstack/Security.hxx"

#include "testIM.hxx"


using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

class TestPresCallback: public TuIM::PresCallback
      {
         public:
            virtual void presenseUpdate(const Uri& dest, bool open, const Data& status );
      };
  

void 
TestPresCallback::presenseUpdate(const Uri& dest, bool open, const Data& status )
{
	assert(0);
}


class TestPageCallback: public TuIM::PageCallback
{
   public:
      virtual void receivedPage( const Data& msg, const Uri& from ,
                                 const Data& signedBy,  Security::SignatureStatus sigStatus,
                                 bool wasEncryped  );

      Uri* mDest;
};
    
class TestErrCallback: public  TuIM::ErrCallback
{
   public:
      virtual void sendPageFailed( const Uri& dest );
};


void 
TestPageCallback::receivedPage( const Data& msg, const Uri& from,
                                const Data& signedBy,  Security::SignatureStatus sigStatus,
                                bool wasEncryped  )
{  
   DebugLog(<< "In TestPageCallback");

   if ( mDest && ( *mDest != from) )
   {
      *mDest = from;
      cerr << "Set destination to <" << *mDest << ">" << endl;
   }
   
   cout << from;
   if ( !wasEncryped )
   {
      cout << " -NOT SECURE- ";
   }
   switch ( sigStatus )
   {
      case  Security::isBad:
         cout << " -bad signature- ";
         break;
      case  Security::none:
         cout << " -no signature- ";
         break;
      case  Security::trusted:
         cout << " <signed  " << signedBy << " > ";
         break;
      case  Security::caTrusted:
         cout << " <ca signed  " << signedBy << " > ";
         break;
      case  Security::notTrusted:
         cout << " <signed  " << signedBy << " NOT TRUSTED > ";
         break;
   }
   
   cout << " says:" << endl;
   cout << msg.escaped() << endl;
}


void 
TestErrCallback::sendPageFailed( const Uri& dest )
{
   InfoLog(<< "In TestErrCallback");  
   cerr << "Message to " << dest << " failed" << endl;
}


bool
processStdin(  TuIM& tuIM, Uri* dest )
{
   char buf[1024];

#if 0
   DebugLog( << "eof = " << eof(fileno(stdin)) );
	if ( eof(fileno(stdin)) )
	{
		return true;
	}
#endif

   DebugLog( << "start read " << sizeof(buf) << " charaters from stdin" );
   int num = read(fileno(stdin),buf,sizeof(buf));
   DebugLog( << "Read " << num << " charaters from stdin" );
   
   if ( (num>3) && (!strncmp("to:",buf,3)) )
   {
      buf[num-1] = 0;
      *dest = Uri(Data(buf+3));
      cerr << "Set destination to <" << *dest << ">";
   }
   else if ( (num==2) && (!strncmp(".",buf,1)) )
   {
      DebugLog( << "Got a period - end program" );
      return false;
   }
   else
   { 
      if ( num >= 1 )
      {
         buf[num-1] = 0;
         Data text(buf);
         
         DebugLog( << "Read <" << text << ">" );
         
         cout << "Send to <" << *dest << ">";
         
         tuIM.sendPage( text , *dest, false /*sign*/, Data::Empty /*encryptFor*/ );
         //tuIM.sendPage( text , *dest, false /*sign*/, dest->getAorNoPort() /*encryptFor*/ );
         //tuIM.sendPage( text , *dest, true /*sign*/,  Data::Empty /*encryptFor*/ );
         //tuIM.sendPage( text , *dest, true /*sign*/, dest->getAorNoPort() /*encryptFor*/ );
      }
   }

   return true;
}


int
main(int argc, char* argv[])
{  
   Log::initialize(Log::COUT, Log::ERR, argv[0]);
   
   //Log::setLevel(Log::DEBUG_STACK);

   InfoLog(<<"Test Driver for IM Starting");
    
   InfoLog( << "\nType a line like\nto:sip:fluffy@localhost:5060\n"
            "to control the destination of your messages. "
            "A line with a singe period on it ends the program\n" );
   
   int port = 5060;
   Uri aor("sip:aor@localhost:5060" );
   Uri dest("sip:you@localhost:5070");
      
   for ( int i=1; i<argc; i++)
   {
      if (!strcmp(argv[i],"-vv"))
      {
         Log::setLevel(Log::DEBUG_STACK);
      }
      else if (!strcmp(argv[i],"-v"))
      {
         Log::setLevel(Log::INFO);
      }
      else if (!strcmp(argv[i],"-port"))
      {
         i++;
         assert( i<argc );
         port = atoi( argv[i] );
      } 
      else if (!strcmp(argv[i],"-aor"))
      {
         i++;
         assert( i<argc );
         aor = Uri(Data(argv[i]));
      } 
      else if (!strcmp(argv[i],"-to"))
      {
         i++;
         assert( i<argc );
         dest = Uri(Data(argv[i]));

         dest.param(p_transport) = "tls";
         
         cout << "Destination is " << dest << endl;
         
      } 
      else
      { 
         ErrLog(<<"Bad command line opion: " << argv[i] );
         ErrLog(<<"options are: [-v] [-vv] [-port 1234] [-aor sip:flffuy@flouf.com] [-to sip:1@foo.com]" << argv[i] );
         assert(0);
      }
   }
   
   InfoLog( << "Using port " << port );
   
   SipStack sipStack;  

#ifdef USE_SSL
   assert( sipStack.security );
   bool ok = sipStack.security->loadAllCerts( Data("password") );
   if ( !ok )
   {
      ErrLog( << "Could not load the certificates" );
	  assert( ok );
   }
#endif
   
//   sipStack.addTransport(Transport::UDP, port);
//   sipStack.addTransport(Transport::TCP, port);
   sipStack.addTransport(Transport::TLS, port);

   TestPageCallback pageCallback;
   pageCallback.mDest = &dest;
   
   TestErrCallback errCallback;
    
   TestPresCallback presCallback;

   aor.port() = port;
   
   Uri contact("sip:me-contact@localhost");
   contact.port() = port;
   contact.param(p_transport) = "tls";
   dest.param(p_transport) = "tls";
   aor.param(p_transport) = "tls";

   TuIM tuIM(&sipStack,aor,contact,&pageCallback,&errCallback,&presCallback);
    
   //Vocal2::makeSocketNonBlocking( fileno(stdin) );

   while (1)
   {
      FdSet fdset; 
      sipStack.buildFdSet(fdset);

      fdset.setRead( fileno(stdin) );
       
      int  err = fdset.select( sipStack.getTimeTillNextProcess());
      if ( err == -1 )
      {
         int e = errno;
		 switch (e)
		 {
		 case 0:
			 break;
		 default:
			InfoLog(<< "Error " << e << " " << strerror(e) << " in select");
			break;
			}
      }
      //InfoLog(<< "Select returned");
       
      if ( fdset.readyToRead( fileno(stdin) ) )
	  //if ( !eof( fileno(stdin) ) )
      {
         bool keepGoing = processStdin(tuIM,&dest);
         if (!keepGoing) break;
      }
       
      // DebugLog ( << "Try TO PROCESS " );
      sipStack.process(fdset);
       
      tuIM.process();       
   }
}

