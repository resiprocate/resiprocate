#if !defined(TRANSACTIONSTATE_HXX)
#define TRANSACTIONSTATE_HXX



namespace Vocal2
{

class Message;
class SipMessage;
class SipStack;

class TransactionState
{
   public:
      TransactionState();
      
      static void process(SipStack& stack); 
     
   private:

      void processClientNonInvite(  Message* msg );
      void processClientInvite(  Message* msg );
      void processServerNonInvite(  Message* msg );
      void processServerInvite(  Message* msg );
      void processStale(  Message* msg );
      
      typedef enum 
      {
         ClientNonInvite,
         ClientInvite,
         ServerNonInvite,
         ServerInvite,
         Stale
      } Machine;
      
      typedef enum 
      {
         Calling,
         Trying,
         Proceeding,
         Completed,
         Terminated
      } State;

      TransactionState(Machine m, State s) : mMachine(m), mState(s){}
      
      static void process(SipStack& stack); 

      void process( Message* msg );
     
   private:

      Machine mMachine;
      State mState;
            
      TransactionState* cancelStateMachine;

      SipMessage* msgToRetransmit;
      
};

}

#endif
