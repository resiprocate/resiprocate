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

// GAG headers
#include "GagMessage.hxx"
#include "GagConduit.hxx"

using namespace resip;

using namespace std;

#define RESIPROCATE_SYSTEM Subsystem::App

int
main (int argc, char **argv)
{
  // Defaults (override with commandline options)
  int tcpPort = 5060;
  int udpPort = 5060;
  int tlsTcpPort = 5061;
  bool tlsServer = false;

  // Get the SIP stack up and running
  Security security (tlsServer, true);
  SipStack sipStack (false, &security);

  sipStack.addTransport(UDP, udpPort);
  sipStack.addTransport(TCP, tcpPort);
  sipStack.addTlsTransport(tlsTcpPort);

  GagConduit conduit(sipStack);

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

      GagErrorMessage errorMessage(error);
      errorMessage.serialize(cout);
    }

    if (fdset.readyToRead(fileno(stdin)))
    {
      GagMessage *message = GagMessage::getMessage(cin);
      conduit.handleMessage(message);
      delete message;
    }
    else
    {
      sipStack.process(fdset);
      conduit.process();
    }
    
  }
}
