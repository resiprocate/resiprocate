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
      bool isRequest(Message* msg) const;
      bool isFinalResponse(Message* msg) const;
      bool isProvisionalResponse(Message* msg) const;
      bool isFailureResponse(Message* msg) const;
      bool isSuccessResponse(Message* msg) const;
      bool isFromTU(Message* msg) const;
      bool isTranportError(Message* msg) const;
      
      SipStack& mStack;
      Machine mMachine;
      State mState;
      bool mIsReliable;
      
      TransactionState* cancelStateMachine;

      SipMessage* mMsgToRetransmit;
};

ostream& 
operator<<(ostream& strm, const TransactionState& state);

}

#endif
