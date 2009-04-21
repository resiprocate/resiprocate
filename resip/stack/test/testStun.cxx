#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#if defined (HAVE_POPT_H) 
#include <popt.h>
#else
#ifndef WIN32
#warning "will not work very well without libpopt"
#endif
#endif

#include <sys/types.h>
#include <iostream>
#include <memory>

#include "rutil/Logger.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/StackThread.hxx"
#include "resip/stack/stun/stun.h"
#include "resip/stack/stun/udp.h"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

int
main(int argc, char* argv[])
{

   char* logType = "cout";
   char* logLevel = "ALERT";
   char* proto = "tcp";
   char* bindAddr = 0;

   int runs = 10000;
   int window = 100;
   int seltime = 0;
   int v6 = 0;
   int invite=0;
   
#if defined(HAVE_POPT_H)
   struct poptOption table[] = {
      {"log-type",    'l', POPT_ARG_STRING, &logType,   0, "where to send logging messages", "syslog|cerr|cout"},
      {"log-level",   'v', POPT_ARG_STRING, &logLevel,  0, "specify the default log level", "DEBUG|INFO|WARNING|ALERT"},
      {"num-runs",    'r', POPT_ARG_INT,    &runs,      0, "number of calls in test", 0},
      {"window-size", 'w', POPT_ARG_INT,    &window,    0, "number of concurrent transactions", 0},
      { "select-time", 's', POPT_ARG_INT,    &seltime,   0, "number of runs in test", 0},
      {"protocol",    'p', POPT_ARG_STRING, &proto,     0, "protocol to use (tcp | udp)", 0},
      {"bind",        'b', POPT_ARG_STRING, &bindAddr,  0, "interface address to bind to",0},
      {"v6",          '6', POPT_ARG_NONE,   &v6     ,   0, "ipv6", 0},
      {"invite",      'i', POPT_ARG_NONE,   &invite     ,   0, "send INVITE/BYE instead of REGISTER", 0},
      POPT_AUTOHELP
      { NULL, 0, 0, NULL, 0 }
   };
   
   poptContext context = poptGetContext(NULL, argc, const_cast<const char**>(argv), table, 0);
   poptGetNextOpt(context);
#endif
   Log::initialize(logType, logLevel, argv[0]);
   cout << "Performing " << runs << " runs." << endl;

   IpVersion version = (v6 ? V6 : V4);
   SipStack receiver;
   
   int stunPort = 25080 + rand()& 0x7fff;   
   receiver.addTransport(UDP, stunPort, version, StunEnabled);

   StackThread sthread(receiver);
   sthread.run();
   
   UInt64 startTime = Timer::getTimeMs();
   int outstanding=0;
   int sent = 0;

   int count=0;
   StunMessage stun;
   StunAtrString username;
   memset(&username, 0, sizeof(StunAtrString));
   memset(&stun, 0, sizeof(StunMessage));

   bool verbose = false;

   StunAddress4 stunServer;
   stunParseServerName( "127.0.0.1", stunServer);
   stunServer.port = stunPort;

   int sendPort = 25080 + rand()& 0x7fff;   
   int myFd = openPort(sendPort, stunServer.addr , verbose);
   
   for (count=0; count<runs; ++count)
   {
      stunBuildReqSimple(&stun, username, false, false, 1);
      char buf[STUN_MAX_MESSAGE_SIZE];
      int len = STUN_MAX_MESSAGE_SIZE;
      len = stunEncodeMessage( stun, buf, len, username, verbose );
      sendMessage( myFd, buf, len, stunServer.addr, stunServer.port, verbose );

      StunAddress4 from;

      char msg[STUN_MAX_MESSAGE_SIZE];
      int msgLen = STUN_MAX_MESSAGE_SIZE;
      getMessage( myFd,
                  msg,
                  &msgLen,
                  &from.addr,
                  &from.port,verbose );
      
      StunMessage resp;
      memset(&resp, 0, sizeof(StunMessage));
      
      if ( verbose ) clog << "Got a response" << endl;
      bool ok = stunParseMessage( msg,msgLen, resp,verbose );
      
      if ( verbose )
      {
         clog << "\t ok=" << ok << endl;
         clog << "\t id=" << resp.msgHdr.id << endl;
         clog << "\t mappedAddr=" << resp.mappedAddress.ipv4 << endl;
         clog << "\t changedAddr=" << resp.changedAddress.ipv4 << endl;
         clog << endl;
      }
   }

   InfoLog (<< "Finished " << count << " runs");
   
   UInt64 elapsed = Timer::getTimeMs() - startTime;
   if (!invite)
   {
      cout << runs << " registrations peformed in " << elapsed << " ms, a rate of " 
           << runs / ((float) elapsed / 1000.0) << " transactions per second.]" << endl;
   }
   else
   {
      cout << runs << " calls peformed in " << elapsed << " ms, a rate of " 
           << runs / ((float) elapsed / 1000.0) << " calls per second.]" << endl;
   }
   cout << "Note: this test runs both sides (client and server)" << endl;
   
   sthread.shutdown();
   sthread.join();

#if defined(HAVE_POPT_H)
   poptFreeContext(context);
#endif
   return 0;
}
