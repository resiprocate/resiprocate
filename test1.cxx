#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "sipstack/UdpTransport.hxx"
#include "sipstack/Helper.hxx"
#include "sipstack/SipMessage.hxx"
#include "sipstack/Uri.hxx"
#include "sipstack/Resolver.hxx"
#include "util/Logger.hxx"
#include "util/DataStream.hxx"

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

int
main(int argc, char* argv[])
{
   Log::initialize(Log::COUT, Log::INFO, argv[0]);
   int runs = 100000;
   if (argc == 2)
   {
      runs = atoi(argv[1]);
   }
   cout << "Performing " << runs << " runs." << endl;
   
   Fifo<Message> received;
   
   UdpTransport* udp = new UdpTransport("localhost", 5070, "default", received);

   NameAddr dest;
   dest.uri().scheme() = "sip";
   dest.uri().user() = "fluffy";
   dest.uri().host() = "localhost";
   dest.uri().port() = 5070;
   dest.uri().param(p_transport) == "udp";
   
   NameAddr from = dest;
   from.uri().port() = 5070;
   
   
   struct timeval tv;
   
   for (int i=0; i<runs; i++)
   {
      auto_ptr<SipMessage> message = auto_ptr<SipMessage>(Helper::makeInvite( dest, from, from));
      Resolver resolver(dest.uri());
      
      Transport::Type t = udp->transport();
      Data foo = Transport::toData(t); 
      message->header(h_Vias).front().transport() = foo;
      message->header(h_Vias).front().sentHost() = udp->hostname();
      message->header(h_Vias).front().sentPort() = udp->port();

      Data* encoded = new Data(2048, true);
      DataStream strm(*encoded);
      message->encode(strm);
      strm.flush();
      udp->send(resolver.mCurrent->ipv4, encoded);
      
      fd_set fdSet; 
      int fdSetSize=0;
      FD_ZERO(&fdSet); 
      udp->buildFdSet(&fdSet, &fdSetSize);
      
      tv.tv_sec=0;
      tv.tv_usec= 5000;
      
      int  err = select(fdSetSize, &fdSet, NULL, NULL, &tv);
      int e = errno;
      if ( err == -1 )
      {
         InfoLog(<< "Error " << e << " " << strerror(e) << " in select");
      }

      udp->process(&fdSet);

      if (received.messageAvailable())
      {
         SipMessage* next = dynamic_cast<SipMessage*>(received.getNext());
         DebugLog (<< "got: " << next->brief());
         assert (next->header(h_RequestLine).uri().host() == "localhost");
         assert (next->header(h_To).uri().host() == "localhost");
         assert (next->header(h_From).uri().host() == "localhost");
         assert (!next->header(h_Vias).begin()->sentHost().empty());
         assert (next->header(h_Contacts).begin()->uri().host() == "localhost");
         assert (!next->header(h_CallId).value().empty());
         
         delete next;
      }
   }
   delete udp;

   return 0;
}
