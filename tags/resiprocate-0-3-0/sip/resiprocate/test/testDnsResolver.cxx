#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "resiprocate/os/Logger.hxx"
#include "resiprocate/DnsResolver.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

namespace resip
{
	
class TestDnsResolver
{
public:
	static void test();
};
 
}

#include "resiprocate/SipStack.hxx"

using namespace resip;




void
TestDnsResolver::test()
{
   SipStack stack;
   Data transactionId = "foo";

   Uri uri;
   uri.scheme() = "sips";
   uri.host() = "www.cisco.com";
   uri.port() = 1588;
    
   stack.mDnsResolver.lookup(transactionId, uri);
   
   Via via;
   via.sentHost() = "sj-wall-1.cisco.com";
   via.transport() = "TLS";
   //via.sentPort() = 1812;
   //via.param(p_received) = "localhost";
   //via.param(p_rport) = 1066;
      
   stack.mDnsResolver.lookup(transactionId, via);


   while (1)
   {
      FdSet fdset;
      stack.mDnsResolver.buildFdSet(fdset);
      int err = fdset.selectMilliSeconds(1);
      assert (err != -1);
      
      stack.mDnsResolver.process(fdset);
      DnsResolver::DnsMessage* myMsg = dynamic_cast<DnsResolver::DnsMessage*>(stack.mStateMacFifo.getNext());
      assert(myMsg);
   
      for (DnsResolver::TupleIterator ti = myMsg->mTuples.begin();
           ti != myMsg->mTuples.end(); ti++)
      {
         DebugLog(<< "--> Driver result: host=" << inet_ntoa(ti->ipv4)
                  << " port=" << ti->port
                  << " transport=" << ti->transport);
      }
   }
}


int 
main(int argc, char* argv[])
{
   Log::initialize(Log::COUT, argc > 1 ? Log::toLevel(argv[1]) :  Log::DEBUG, argv[0]);
   TestDnsResolver::test();
   return 0;
}


