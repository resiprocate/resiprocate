#if !defined(SIPSTACK_HXX)
#define SIPSTACK_HXX

#include <sipstack/Fifo.hxx>

namespace Vocal2
{

class SipMessage;

class SipStack
{
   public:
      void send(SipMessage* msg);

      // this is only if you want to send to a destination not in the route. You
      // probably don't want to use it. 
      void send(SipMessage* msg, const Data& dest="default" );
      
      SipMessage* receive();

      void process();

      /// returns time in milliseconds when process next needs to be called 
      int getTimeTillNextProcess(); 

      enum ThreadFunction { Timer, UDP, StateMachine, TCP, };

      void runThread( ThreadFunction );


   private:

      Executive mExecutive;
      TransportSelector mTransportSelector;

      TransactionMap mTransactionMap;

      TransportDirectory mTransportDirector;

      
      Fifo<SipMessage> mTUFifo;

      Fifo<SipMessage> mStateMacFifo;

      Timer mTimers;
};
 
}

