#include "resiprocate/os/Logger.hxx"

#include "resiprocate/Uri.hxx"

#include "Register.hxx"
#include "Registrar.hxx"
#include "InviteServer.hxx"
#include "InviteClient.hxx"
#include "Transceiver.hxx"


using namespace resip;
using namespace std;
using namespace Loadgen;


#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

void usage()
{
   cout << "Usage: lg port {REG_RECEIVE, INV_RECEIVE}" << endl
        << "       lg port {REG_SEND, INV_SEND} targetHost:port startingExt endingExt [registers]" << endl;
   exit(-1);
}

int 
main(int argc, char* argv[])
{
   Log::initialize(Log::COUT, Log::INFO, argv[0]);
 
   if (!(argc == 3 || argc == 6 || argc == 7))
   {
      usage();
   }

   int port = atoi(argv[1]);
   if (port == 0)
   {
      usage();
   }
   
   if (Data(argv[2]) == "REG_SEND" || Data(argv[2]) == "INV_SEND")
   {
      Uri target;

      ParseBuffer host(argv[3], strlen(argv[3]));
      const char* start = host.position();
      host.skipToChar(Symbols::COLON[0]);
      if (host.position() == host.end())
      {
         usage();
      }
      
      target.host() = host.data(start);
      host.skipChar();
      target.port() = host.integer();
      target.param(p_transport) = "udp";

      InfoLog(<< target);
   
      Transceiver stack(port);
      
      int startExt = atoi(argv[4]);
      int endExt = atoi(argv[5]);
      if (startExt == 0 || endExt == 0 || startExt > endExt)
      {
         usage();
      }
      int numTimes = 0;
      if (argc == 7)
      {
         numTimes = atoi(argv[6]);
         if (numTimes == 0)
         {
            usage();
         }
      }
      if (Data(argv[2]) == "REG_SEND")
      {
         Register reg(stack, target, 
                      startExt, endExt, numTimes);
         reg.go();
      }
      else
      {
         InviteClient inv(stack, target, 
                          startExt, endExt, numTimes);
         inv.go();
      }
   }
   else if (Data(argv[2]) == "REG_RECEIVE")
   {
      Transceiver stack(port);
      Registrar reg(stack);
      reg.go();
   }
   else if (Data(argv[2]) == "INV_RECEIVE")
   {
      Transceiver stack(port);
      InviteServer inv(stack);
      inv.go();
   }
   else
   {
      usage();
   }
   return 0;
}
