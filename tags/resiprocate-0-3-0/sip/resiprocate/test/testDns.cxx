#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/ThreadIf.hxx"
#include "resiprocate/DnsInterface.hxx"
#include "resiprocate/DnsResult.hxx"
#include "resiprocate/SipStack.hxx"


#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

namespace resip
{

class TestDnsHandler : public DnsInterface::Handler
{
   public:
      void handle(DnsResult* result)
      {
         if (result->available())
         {
            InfoLog (<< "Results: " );
            while (result->available())
            {
               InfoLog (<< result->next());
            }
         }
         else
         {
            InfoLog (<< "No more dns results");
         }
      }
};
	
class TestDns : public DnsInterface, public ThreadIf
{
   public:
      TestDns(DnsInterface::Handler* handler) : DnsInterface(handler)
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
   Log::initialize(Log::COUT, Log::DEBUG, argv[0]);
   TestDnsHandler handler;
   TestDns dns(&handler);
   dns.run();
   
   Uri uri;
   uri.scheme() = "sip";
   uri.host() = "cathaynetworks.com";
   for (int i=1; i<argc; i++)
   {
      uri.host() = argv[i];
      dns.lookup(uri, Data::from(i));
   }

   sleep(2);
   
   return 0;
}


