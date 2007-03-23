#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include <list>
#include <errno.h>
#include <sstream>

#ifndef WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#endif

#include "rutil/Socket.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/TuIM.hxx"
#include "resip/stack/Security.hxx"
#include "resip/stack/ShutdownMessage.hxx"
//#include "resip/stack/ApiCheckList.hxx"

#include "contrib/getopt/getopt.h"

// GAG headers
#include "GagMessage.hxx"
#include "GagConduit.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

void shutdown (SipStack *stack)
{
  if (!stack) return;
  // Wait for all transactions to complete
  bool done = false;
  stack->shutdown();
  while (!done)
  {
      FdSet fdset; 
      stack->buildFdSet(fdset);
      fdset.selectMilliSeconds(1000); 
      stack->process(fdset);
      Message* msg = stack->receiveAny();
 
      if (msg) 
      {
        DebugLog(<<"SHUTDOWN ATE: " << msg->brief() );
      }
      else
      {
        DebugLog(<<"SHUTDOWN Waiting");
      }
      done = dynamic_cast<ShutdownMessage*>(msg) != 0;
  }
  return;
  
}

void init_loopback()
{
  Socket s;
  int status;
  struct sockaddr_in them;

  DebugLog (<<"Using loopback (127.0.0.1:48879) for communications");

  memset((void *)&them, 0, sizeof(them));
  them.sin_family = AF_INET;
  them.sin_port = 0xBEEF;
#ifdef WIN32
  them.sin_addr.s_addr = htonl(0x100007f);
#else
  inet_aton("127.0.0.1", &(them.sin_addr));
#endif

  s = socket(PF_INET, SOCK_STREAM, 0);
  assert(s > 0);

  status = connect(s, (struct sockaddr *)&them, sizeof(them));
  if (status < 0)
  {
    ErrLog( << "Could not connect to loopback interface. Exiting.");
    exit (-1);
  }

  // Hijack stdin and stdout  
  close (0);
  close (1);
  status = dup2( static_cast<int>(s), 0);
  assert(status >= 0);
  status = dup2( static_cast<int>(s), 1);
  assert(status >= 0);
}

int
main (int argc, char **argv)
{
  int c;
  bool useLoopback = false;
  bool getoptError = false;

  // Defaults (override with commandline options)
#ifdef WIN32
  int tcpPort = 6000;
  int udpPort = 6000;
  int tlsTcpPort = 6001;
#else
  int tcpPort = 6000 + getuid() * 2;
  int udpPort = 6000 + getuid() * 2;
  int tlsTcpPort = 6001 + getuid() * 2;
  pid_t parent = getppid();
#endif

  bool tlsServer = false;

  Log::initialize(Log::File, Log::Debug, argv[0]);

  // Read commandline options

  while	((c = getopt(argc, argv, "l")) != -1)
  {
    switch (c) {
      case 'l':
        useLoopback = true;
      case '?':
        getoptError = true;
    }
  }

  if (useLoopback)
  {
    init_loopback();
  }

  if (getoptError)
  {
    InfoLog ( << "Invalid command line option provided");
  }

#ifdef USE_SSL
  // Get the SIP stack up and running
  Security security (tlsServer, true);
  SipStack sipStack (false, &security);
#else
  SipStack sipStack (false);
#endif

  sipStack.addTransport(UDP, udpPort );
  sipStack.addTransport(TCP, tcpPort );
/*
  sipStack.addTransport(UDP, udpPort+2,V6 );
  sipStack.addTransport(TCP, tcpPort+2,V6 );
*/
#ifdef USE_SSL
  sipStack.addTlsTransport(tlsTcpPort);
#endif

  GagConduit conduit(sipStack, udpPort);

  // Say hello to GAIM (eventually, we should
  // make certain things work, and base the
  // value of "ok" on this).
  GagHelloMessage(true).serialize(cout);


  // Main processing loop
  int time;
  int err;

#if 0
  // candidate for deletion
#ifndef WIN32
  // Make stdin nonblocking
  fcntl(0, F_SETFL, O_NONBLOCK);
#else
    unsigned long noBlock = 1;
	int errNoBlock = ioctlsocket( 0, FIONBIO , &noBlock );
	assert( errNoBlock == 0 );
#endif
#endif

  while (1)
  {
    FdSet fdset;

    sipStack.buildFdSet(fdset);
    time = sipStack.getTimeTillNextProcessMS();

    fdset.setRead(fileno(stdin));

    err = fdset.selectMilliSeconds(time);
    if (err < 0)
    {
      // send error message to GAIM
      Data error;
      error = "Error in select(): [";
      error += errno;
      error += "] ";
      error += strerror(errno);

      GagErrorMessage(error).serialize(cout);
    }

#ifndef WIN32
    /* !ah! not portable to windows -- works for now. */
    { 
      pid_t current_parent = getppid();
      if (current_parent != parent)
      {
        ErrLog(<<"Unsupervised child found [" << current_parent << "] while looking for ["
  		                            << parent << "] -- crying, exiting.");
        shutdown(&sipStack);
        exit(-1);
      }
    }
#endif

    if (fdset.readyToRead(fileno(stdin)))
    {

      DebugLog ( << "stdin is ready to read" );

      GagMessage *message = GagMessage::getMessage(fileno(stdin));

      if (message)
      {
        conduit.handleMessage(message);
        delete message;
      }
      else
      {
        Data error("Panic! Something is horribly wrong!");
        DebugLog ( << "Received unexpected series of bytes from Gaim" );
        GagErrorMessage(error).serialize(cout);
        conduit.removeAllUsers();
        shutdown(&sipStack);
        exit(-1);
      }
      if (!conduit.isRunning())
      {
        shutdown(&sipStack);
        exit (0);
      }
      sipStack.process(fdset);
      conduit.process();

    }
    else
    {
      sipStack.process(fdset);
      conduit.process();
    }
  }
}
