#if !defined(TRANSPORTSELECTOR_HXX)
#define TRANSPORTSELECTOR_HXX

#include <sipstack/Data.hxx>

namespace Vocal2
{

  class SipMessage;
  class UdpTransport;
  
  class TransportSelector
  {

public:
  void process();

  void send( SipMessage& msg );

   void send(SipMessage* msg, const Data& dest="default" );

private:

  UdpTransport* udp;

};


}

#endif
