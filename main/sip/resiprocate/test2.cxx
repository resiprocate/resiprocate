#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <memory>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "sipstack/Helper.hxx"
#include "sipstack/SipMessage.hxx"
#include "sipstack/Uri.hxx"
#include "sipstack/SipStack.hxx"
#include "util/Logger.hxx"


using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

int
main(int argc, char* argv[])
{
   Log::initialize(Log::COUT, argc > 1 ? Log::toLevel(argv[1]) :  Log::INFO, argv[0]);
   
   SipStack stack1;
   SipStack stack2;
   stack1.addTransport(Transport::UDP, 5070);
   stack2.addTransport(Transport::UDP, 5080);

   NameAddr dest;
   dest.uri().scheme() = "sip";
   dest.uri().user() = "fluffy";
   dest.uri().host() = "localhost";
   dest.uri().port() = 5080;
   dest.uri().param(p_transport) = "udp";
   
   NameAddr from = dest;
   from.uri().port() = 5070;
   
   
   struct timeval tv;
   
   for (int i=0; i<1; i++)
   {
      {
         auto_ptr<SipMessage> message = auto_ptr<SipMessage>(Helper::makeInvite( dest, from, from));

         fd_set fdSet; 
         int fdSetSize=0;
         FD_ZERO(&fdSet); 
         stack1.buildFdSet(&fdSet, &fdSetSize);

         tv.tv_sec=0;
         tv.tv_usec= 5000;
         int  err = select(fdSetSize, &fdSet, NULL, NULL, &tv);
         assert (err != -1);
      
         stack1.send(*message);
         stack1.process(&fdSet);
      
         SipMessage* received = (stack1.receive());
         if (received)
         {
            InfoLog (<< "got: " << received->brief());
            assert (received->header(h_RequestLine).uri().host() == "localhost");
            assert (received->header(h_To).uri().host() == "localhost");
            assert (received->header(h_From).uri().host() == "localhost");
            assert (!received->header(h_Vias).begin()->sentHost().empty());
            assert (received->header(h_Contacts).begin()->uri().host() == "localhost");
            assert (!received->header(h_CallId).value().empty());
         }
      
         delete received;
      }
      
      
      {
         fd_set fdSet; 
         int fdSetSize=0;
         FD_ZERO(&fdSet); 
         stack2.buildFdSet(&fdSet, &fdSetSize);
         int  err = select(fdSetSize, &fdSet, NULL, NULL, &tv);
         assert (err != -1);
      
         stack2.process(&fdSet);
      
         SipMessage* received = (stack2.receive());
         if (received)
         {
            InfoLog (<< "got: " << received->brief());
            assert (received->header(h_RequestLine).uri().host() == "localhost");
            assert (received->header(h_To).uri().host() == "localhost");
            assert (received->header(h_From).uri().host() == "localhost");
            assert (!received->header(h_Vias).begin()->sentHost().empty());
            assert (received->header(h_Contacts).begin()->uri().host() == "localhost");
            assert (!received->header(h_CallId).value().empty());
         }
      
         delete received;
      }
   }

   return 0;
}
