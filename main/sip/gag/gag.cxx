#if defined(HAVE_CONFIG_HXX)
#include "resiprocate/config.hxx"
#endif

#include <list>
#include <errno.h>

#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/TuIM.hxx"
#include "resiprocate/Security.hxx"
#include "resiprocate/ShutdownMessage.hxx"

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
  while (!done)
  {
      FdSet fdset; 
      stack->buildFdSet(fdset);
      fdset.selectMilliSeconds(1000); 
      stack->process(fdset);
      Message* msg = stack->receiveAny();
      DebugLog(<<"SHUTDOWN ATE: " << msg->brief() );
      done = dynamic_cast<ShutdownMessage*>(msg) != 0;
  }
  return;
  
}

int
main (int argc, char **argv)
{
  // Defaults (override with commandline options)
  int tcpPort = 8060;
  int udpPort = 8060;
  int tlsTcpPort = 8061;
  bool tlsServer = false;

  Log::initialize(Log::FILE, Log::DEBUG, argv[0]);

  // Get the SIP stack up and running
  Security security (tlsServer, true);
  SipStack sipStack (false, &security);

  sipStack.addTransport(UDP, udpPort);
  sipStack.addTransport(TCP, tcpPort);
  sipStack.addTlsTransport(tlsTcpPort);

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
    else
    {
      sipStack.process(fdset);
      conduit.process();
    }
    
  }
}
