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

      void process( Message* msg );
     
      
   private:

      typedef enum 
      {
         ClientNonInvite,
         ClientInvite,
         ServerNonInvite,
         ServerInvite,
         Stale
      } Machine;
      
      Machine mMachine;
      
      typedef enum 
      {
         Calling,
         Trying,
         Proceeding,
         Completed,
         Terminated
      } State;
      
      State mState;
            
      TransactionState* cancelStateMachine;
      SipMessage* msgToRetransmit;
      
};

}

#endif
