
class TransportSelector
{

public:
  void process();

  void send( SipMessage& msg );

   void send(SipMessage* msg, const Data& dest="default" );

private:

  UdpTransport* udp;

};
