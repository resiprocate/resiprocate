#if !defined(UDPTRANSPORT_HXX)
#define UDPTRANSPORT_HXX

#include <sipstack/Transport.hxx>
#include <sipstack/Message.hxx>

namespace Vocal2
{

class SipMessage;

class UdpTransport : public Transport
{
   public:
      UdpTransport(int portNum, Fifo<Message>& fifo);
      virtual  ~UdpTransport();

      virtual void send( const sockaddr_in& address, const  char* buffer, size_t length); //, TransactionId txId) ;
      virtual void process() ;

   private:
      static const unsigned long MaxBufferSize;
};
 
}

#endif
