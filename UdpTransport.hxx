#if !defined(UDPTRANSPORT_HXX)
#define UDPTRANSPORT_HXX

#include <sipstack/Transport.hxx>

namespace Vocal2
{

class SipMessage;

class UdpTransport : public Transport
{
   public:
      UdpTransport(int portNum, Fifo<SipMessage>& fifo);
      virtual  ~UdpTransport();

      virtual void send( const sockaddr_in& address, const  char* buffer, size_t length); //, TransactionId txId) ;
      virtual void process() ;

   private:
      static const unsigned long MaxBufferSize;
};
 
}

#endif
