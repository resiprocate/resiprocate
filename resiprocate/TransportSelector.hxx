#if !defined(TRANSPORTSELECTOR_HXX)
#define TRANSPORTSELECTOR_HXX

#include <sipstack/Data.hxx>
#include <sipstack/SipMessage.hxx>
#include <sipstack/Fifo.hxx>

namespace Vocal2
{

class Message;
class UdpTransport;
  
class TransportSelector
{
   public:
      TransportSelector(int portNum);
      void process();

      void send( SipMessage* msg );

      // I don't think we really need this at this level, handled one level
      // up.
      //   void send(SipMessage* msg, const Data& dest="default" );

   private:

      // this eventually will have to allow for construction and management
      // of n of these guys
      UdpTransport* mUdp;
      int mPortNum;
};

}

#endif
