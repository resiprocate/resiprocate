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
      TransactionState(SipStack& stack);
      
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

      TransactionState(SipStack& stack, Machine m, State s) : mStack(stack), mMachine(m), mState(s){}
      
   private:
      bool isFinalResponse(Message* msg) const;
      bool isProvisionalResponse(Message* msg) const;
      bool isFailureResponse(Message* msg) const;
      bool isSuccessResponse(Message* msg) const;
      bool isFromTU(Message* msg) const;
      
      const SipStack& mStack;
      Machine mMachine;
      State mState;
            
      TransactionState* cancelStateMachine;

      SipMessage* mMsgToRetransmit;
};

}

#endif
