
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
#include "sip2/sipstack/Helper.hxx"
#include "sip2/sipstack/TuIM.hxx"

#include "testIM.hxx"


using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

class TestPageCallback: public TuIM::PageCallback
{
   public:
      virtual void receivedPage( Data& msg, Uri& from );

      Uri* mDest;
};
    
class TestErrCallback: public  TuIM::ErrCallback
{
   public:
      virtual void sendPageFailed( Uri& dest );
};


void 
TestPageCallback::receivedPage( Data& msg, Uri& from )
{  
   InfoLog(<< "In TestPageCallback");

   if ( mDest && ( *mDest != from) )
   {
      Data f("sip:");
      f += from.getAor();
      f += Data(":");
      f += Data( from.port() );
      
      *mDest = Uri(f);
      cerr << "Set destination to <" << *mDest << ">" << endl;
   }
   
   cerr << from << " says:\n";
   cerr << msg << endl;
}


void 
TestErrCallback::sendPageFailed( Uri& dest )
{
   InfoLog(<< "In TestErrCallback");  
   cerr << "Message to " << dest << " failed" << endl;
}


bool
processStdin(  TuIM& tuIM, Uri* dest )
{
   char buf[1024];

   //DebugLog( << "start read " << sizeof(buf) << " charaters from stdin" );
   int num = read(fileno(stdin),buf,sizeof(buf));
   //DebugLog( << "Read " << num << " charaters from stdin" );
   
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
         
         InfoLog( << "Send to <" << *dest << ">" );
         
         tuIM.sendPage( text , *dest );
      }
   }

   return true;
}


int
main(int argc, char* argv[])
{  
   Log::initialize(Log::COUT, Log::CRIT, argv[0]);
   CritLog(<<"Test Driver for IM Starting");
    
   CritLog( << "\nType a line like\nto:sip:fluffy@localhost:5060\n"
            "to control the destination of your messages. "
            "A line with a singe period on it ends the program\n" );
      
   int port = 5060;
   Uri aor("sip:aor@localhost" );
   Uri dest("sip:you@localhost:5060");
      
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

   sipStack.addTransport(Transport::UDP, port);

   TestPageCallback pageCallback;
   pageCallback.mDest = &dest;
   
   TestErrCallback errCallback;
    
   aor.port() = port;
   
   Uri contact("sip:me-contact@localhost");
   contact.port() = port;
   
   TuIM tuIM(&sipStack,aor,contact,&pageCallback,&errCallback);
    
   while (1)
   {
      FdSet fdset; 
      sipStack.buildFdSet(fdset);

      fdset.setRead( fileno(stdin) );
       
      int  err = fdset.select( sipStack.getTimeTillNextProcess());
      if ( err == -1 )
      {
         int e = errno;
         InfoLog(<< "Error " << e << " " << strerror(e) << " in select");
      }
      //InfoLog(<< "Select returned");
       
      if ( fdset.readyToRead( fileno(stdin) ) )
      {
         bool keepGoing = processStdin(tuIM,&dest);
         if (!keepGoing) break;
      }
       
      //DebugLog ( << "Try TO PROCESS " );
      sipStack.process(fdset);
       
      tuIM.process();
       
   }
}

