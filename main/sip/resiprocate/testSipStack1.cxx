#include <iostream>

#ifndef WIN32
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <sipstack/SipStack.hxx>
#include <util/Logger.hxx>

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

int
main()
{
	initNetwork();
	
  SipStack sipStack;
  SipMessage* msg=NULL;

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
	  
	  sipStack.process(&fdSet);

	  msg = sipStack.receive();
	  if ( msg )
	  {
		  DebugLog ( << "got message: " << *msg);
	   
		  msg->encode(cerr);	  
	  }
	  
  }
}
