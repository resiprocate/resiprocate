#if !defined(TRANSACTIONSTATE_HXX)
#define TRANSACTIONSTATE_HXX


#include <sipstack/Message.hxx>


namespace Vocal2
{

class TransactionState
{
   public:
      
      TransactionState();
      
      static void process(); 

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
