#if defined(HAVE_CONFIG_HXX)
#include "resiprocate/config.hxx"
#endif

#include <list>

#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/TuIM.hxx"
#include "resiprocate/Security.hxx"

// GAG headers
#include "GagMessage.hxx"

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

  // Here are all of our TUs
  list<TuIM> tuIMList;

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
      // XXX send error message to GAIM
    }

    if (fdset.readyToRead(fileno(stdin)))
    {
      // XXX process incoming GAIM command
    }
    else
    {
      sipStack.process(fdset);
      list<TuIM>::iterator tu;
      tu = tuIMList.begin();
      while (tu != tuIMList.end())
      {
        tu->process();
        tu++;
      }
    }
    
  }
}
