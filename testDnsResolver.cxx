#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sipstack/SipStack.hxx"
#include "sipstack/DnsMessage.hxx"

using namespace Vocal2;

int main()
{
   SipStack stack;
   DnsMessage* myMsg;

   Data transactionId = "foo";

   Uri uri;
   uri.scheme() = "sips";
   uri.host() = "www.cisco.com";
   uri.port() = 1588;
    
   stack.mDnsResolver.lookup(transactionId, uri);
   myMsg = dynamic_cast<DnsMessage*>(stack.mStateMacFifo.getNext());
   assert(myMsg);

   for (DnsResolver::TupleIterator ti = myMsg->begin();
        ti != myMsg->end(); ti++)
   {
      std::cerr << "--> Driver result: host=" << inet_ntoa((*ti).ipv4.sin_addr)
                << " port=" << (*ti).port
                << " transport=" << (*ti).transport << std::endl;
      
   }
   
   Via via;
   via.sentHost() = "sj-wall-1.cisco.com";
   via.transport() = "TLS";
//   via.sentPort() = 1812;
//   via.param(p_received) = "localhost";
//   via.param(p_rport) = 1066;
      
   stack.mDnsResolver.lookup(transactionId, via);
   myMsg = dynamic_cast<DnsMessage*>(stack.mStateMacFifo.getNext());
   assert(myMsg);
   
   for (DnsResolver::TupleIterator ti = myMsg->begin();
        ti != myMsg->end(); ti++)
   {
      std::cerr << "--> Driver result: host=" << inet_ntoa((*ti).ipv4.sin_addr)
                << " port=" << (*ti).port
                << " transport=" << (*ti).transport << std::endl;
      
   }
}
