#ifndef Transceiver_hxx
#define Transceiver_hxx

#include "resiprocate/UdpTransport.hxx"
#include "resiprocate/SipMessage.hxx"
#include "Resolver.hxx"

namespace Loadgen
{

class Transceiver  // currently a Udp transceiver
{
   public:
      Transceiver(int port);
      
      //sends the message right away, fully populates the front via of the message
      void send(const resip::Resolver& target,  
                resip::SipMessage& msg);

      void send(resip::SipMessage& message);
      
      //blocks for up to waitMs, returns null if no message was received.
      //caller of receive owns the memory.
      resip::SipMessage* receive(int waitMs);

      const resip::Uri& contactUri() { return mContactUri; }
      
   private:
      resip::Fifo<resip::Message> mReceived;
      resip::UdpTransport mUdp;
      resip::Uri mContactUri;
      resip::FdSet mFdset;
};

 
}

#endif
