#if !defined(SIPSTACK_HXX)
#define SIPSTACK_HXX

#include <sipstack/Executive.hxx>
#include <sipstack/TransportSelector.hxx>
#include <sipstack/TransactionMap.hxx>
// #include <sipstack/TransportDirector.hxx>
#include <sipstack/Fifo.hxx>
#include <sipstack/TimerWheel.hxx>

namespace Vocal2
{

  class Data;
  class Message;
  class SipMessage;

class SipStack
{
   public:
      void send(SipMessage* msg);

      // this is only if you want to send to a destination not in the route. You
      // probably don't want to use it. 
      void send(SipMessage* msg, const Data &dest="default" );
      
      SipMessage* receive();

      void process();

      /// returns time in milliseconds when process next needs to be called 
      int getTimeTillNextProcess(); 

      enum ThreadFunction { Timer, UDP, StateMachine, TCP, };

      void runThread( enum ThreadFunction funcType);


   private:

      Executive mExecutive;
      TransportSelector mTransportSelector;

      TransactionMap mTransactionMap;

  //      TransportDirector mTransportDirector;
      
      Fifo<Message> mTUFifo;

      Fifo<Message> mStateMacFifo;

  TimerWheel  mTimers;
};
 
}

#endif
