#if !defined(TRANSPORTSELECTOR_HXX)
#define TRANSPORTSELECTOR_HXX

#include <sipstack/SipMessage.hxx>
#include <util/Data.hxx>
#include <util/Fifo.hxx>

namespace Vocal2
{

class SipMessage;
class UdpTransport;
class SipStack;

class TransportSelector
{
   public:
      TransportSelector(SipStack& stack);
      void process(fd_set* fdSet);

      void send( SipMessage* msg );

      // I don't think we really need this at this level, handled one level
      // up.
      //   void send(SipMessage* msg, const Data& dest="default" );
	
	void buildFdSet( fd_set* fdSet, int* fdSetSize );
	
   private:

      // this eventually will have to allow for construction and management
      // of n of these guys
      SipStack& mStack;
      UdpTransport* mUdp;
};

}

#endif
