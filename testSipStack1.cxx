#include <iostream>

#ifndef WIN32
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <sipstack/SipStack.hxx>
#include <sipstack/Uri.hxx>
#include <util/Logger.hxx>
#include <sipstack/Helper.hxx>

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

int
main(int argc, char* argv[])
{
   Log::initialize(Log::COUT, Log::DEBUG, argv[0]);
   DebugLog (<< "hi there");
   
   initNetwork();
	
   SipStack sipStack;
   SipMessage* msg=NULL;

      int count=0;
  while (1)
   {
      struct timeval tv;
      fd_set fdSet; int fdSetSize;
      FD_ZERO(&fdSet); fdSetSize=0;
       
      sipStack.buildFdSet(&fdSet,&fdSetSize);
	  
      tv.tv_sec=0;
      tv.tv_usec= 1000 * sipStack.getTimeTillNextProcess();
	  
      int  err = select(fdSetSize, &fdSet, NULL, NULL, &tv);
      int e = errno;
      if ( err == -1 )
      {
         // error occured
         cerr << "Error " << e << " " << strerror(e) << " in select" << endl;
      }
      
      //DebugLog ( << "Try TO PROCESS " );
      sipStack.process(&fdSet);

      //DebugLog ( << "Try TO receive " );
      msg = sipStack.receive();
      if ( msg )
      {
         DebugLog ( << "got message: " << *msg);
	   
         msg->encode(cerr);	  
      }

#if 0
      if ( count++ >= 10 )
      {   
	      DebugLog ( << "Try to send a message" );
	      count = 0;
	      
	      NameAddr dest;
	      NameAddr from;
	      NameAddr contact;
	      from.uri().scheme() = Data("sip");
	      from.uri().host() = Data("foo.com");
	      from.uri().port() = 5060;
	      from.uri().user() = Data("fluffy");
	      from.uri().param(p_transport) == "udp";
	      
	      dest = from;
	      contact = from;
	            
	      SipMessage message = Helper::makeInvite( dest ,
						       from,
						       contact);
	      
	      sipStack.send( message );
	      
	       DebugLog ( << "Sent Msg" << message );
      }
#endif
   }
}
