#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <sstream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <sipstack/UdpTransport.hxx>
#include <sipstack/Helper.hxx>
#include <sipstack/SipMessage.hxx>
#include <sipstack/Uri.hxx>
#include <sipstack/Resolver.hxx>
#include <util/Logger.hxx>


using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

int
main(int argc, char* argv[])
{
   Log::initialize(Log::COUT, Log::DEBUG, argv[0]);

   Fifo<Message> received;
   
   UdpTransport* udp = new UdpTransport("localhost", 5070, "default", received);

   NameAddr dest;
   dest.uri().scheme() = "sip";
   dest.uri().user() = "fluffy";
   dest.uri().host() = "kelowna.gloo.net";
   dest.uri().port() = 5070;
   dest.uri().param(p_transport) == "udp";
   
   NameAddr from = dest;
   from.uri().port() = 5061;
   
   SipMessage message = Helper::makeInvite( dest, from, from);
   Resolver resolver(dest.uri());

   message.header(h_Vias).front().transport() = Transport::toData(udp->transport()); 
   message.header(h_Vias).front().sentHost() = udp->hostname();
   message.header(h_Vias).front().sentPort() = udp->port();
   
   Data encoded = message.encode();

   udp->send(&resolver.mCurrent->ipv4, encoded.c_str(), encoded.size()); 
   
   struct timeval tv;
   fd_set fdSet; int fdSetSize;
   FD_ZERO(&fdSet); fdSetSize=0;
   tv.tv_sec=0;
   tv.tv_usec= 1000;

   int  err = select(fdSetSize, &fdSet, NULL, NULL, &tv);
   int e = errno;
   if ( err == -1 )
   {
      InfoLog(<< "Error " << e << " " << strerror(e) << " in select");
   }
   
   udp->process(&fdSet);
   if (received.messageAvailable())
   {
      Message* next = received.getNext();
      DebugLog (<< "got: " << *next);
   }
   
   return 0;
}
