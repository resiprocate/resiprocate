#if !defined(UDPTRANSPORT_HXX)
#define UDPTRANSPORT_HXX

#include <sip2/sipstack/Transport.hxx>

namespace Vocal2
{

class UdpTransport : public Transport
{
   public:
      UdpTransport(in_port_t portNum, Fifo<Message>& fifo);
      virtual  ~UdpTransport();

      virtual void send( const sockaddr_in& address, const  char* buffer, size_t length); //, TransactionId txId) ;
      virtual void process() ;

   private:
      static const unsigned long MaxBufferSize;
};
 
}

#endif
