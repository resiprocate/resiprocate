#if !defined(TRANSACTIONSTATE_HXX)
#define TRANSACTIONSTATE_HXX

namespace Vocal2
{

class TransactionState
{
   public:
      
      TransactionState();
      
      static void process(); 

      void process( SipMessage* msg );
     
      
   private:

      enum Machine 
      {
         ClientNonInvite,
         ClientInvite,
         ServerNonInvite,
         ServerInvite,
         Stale
      }
      Machine mMachine;
      
      enum State
      {
         Calling,
         Trying,
         Proceeding,
         Completed,
         Terminated
      }
      State mState;
            
      TransactionState cancelStateMachine;
      SipMessage* msgToRetransmit;
      
};

}

#endif
