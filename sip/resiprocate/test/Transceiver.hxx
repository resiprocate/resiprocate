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
      void send(const Vocal2::Resolver& target,  
                Vocal2::SipMessage& msg);

      void send(Vocal2::SipMessage& message);
      
      //blocks for up to waitMs, returns null if no message was received.
      //caller of receive owns the memory.
      Vocal2::SipMessage* receive(int waitMs);

      const Vocal2::Uri& contactUri() { return mContactUri; }
      
   private:
      Vocal2::Fifo<Vocal2::Message> mReceived;
      Vocal2::UdpTransport mUdp;
      Vocal2::Uri mContactUri;
      Vocal2::FdSet mFdset;
};

 
}

#endif
