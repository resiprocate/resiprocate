#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/ThreadIf.hxx"
#include "resiprocate/DnsInterface.hxx"
#include "resiprocate/DnsResult.hxx"
#include "resiprocate/SipStack.hxx"


#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

namespace resip
{

class TestDnsHandler : public DnsHandler
{
   public:
      void handle(DnsResult* result)
      {
         switch (result->available())
         {
            case DnsResult::Available:
               InfoLog (<< result->next());
               break;
            case DnsResult::Finished:
               InfoLog (<< "No more dns results");
               break;
            default:
               break;
         }
      }
};
	
class TestDns : public DnsInterface, public ThreadIf
{
   public:
      TestDns() : DnsInterface(false)
      {
      }

      void thread()
      {
         while (!waitForShutdown(100))
         {
            FdSet fdset;
            buildFdSet(fdset);
            fdset.selectMilliSeconds(1);
            process(fdset);
         }
      }
};
 
}

using namespace resip;

int 
main(int argc, char* argv[])
{
   Log::initialize(Log::COUT, Log::INFO, argv[0]);
   TestDnsHandler handler;
   TestDns dns;
   dns.run();
   
   Uri uri;
   uri.scheme() = "sip";
   uri.host() = "cathaynetworks.com";
   for (int i=1; i<argc; i++)
   {
      uri.host() = argv[i];
      dns.lookup(uri, &handler);
   }

   sleep(2);
   
   return 0;
}


