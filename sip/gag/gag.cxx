#if defined(HAVE_CONFIG_HXX)
#include "resiprocate/config.hxx"
#endif

#include <list>
#include <errno.h>

#ifndef WIN32
#include <unistd.h>
#include <sys/types.h>
#endif

#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/TuIM.hxx"
#include "resiprocate/Security.hxx"
#include "resiprocate/ShutdownMessage.hxx"

#include "contrib/getopt/getopt.h"

// GAG headers
#include "GagMessage.hxx"
#include "GagConduit.hxx"

using namespace resip;

using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::APP

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
  int s;
  int status;
  struct sockaddr_in them;

  DebugLog (<<"Using loopback (127.0.0.1:48879) for communications");

  memset((void *)&them, 0, sizeof(them));
  them.sin_family = AF_INET;
  them.sin_port = 0xBEEF;
  inet_aton("127.0.0.1", &(them.sin_addr));

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
  status = dup2(s, 0);
  assert(status >= 0);
  status = dup2(s, 1);
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

  Log::initialize(Log::FILE, Log::DEBUG, argv[0]);

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

  sipStack.addTransport(UDP, udpPort);
  sipStack.addTransport(TCP, tcpPort);
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
	// !ah! not portable to windows -- works for now.
    if (getppid() != parent)
    {
      ErrLog(<<"Unsupervised child -- crying, exiting.");
      shutdown(&sipStack);
      exit(-1);
    }
#endif

    if (fdset.readyToRead(fileno(stdin)))
    {
      DebugLog ( << "stdin is ready to read" );
      GagMessage *message = GagMessage::getMessage(cin);
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
    }
    sipStack.process(fdset);
    conduit.process();
  }
}
