#if !defined(TRANSACTIONSTATE_HXX)
#define TRANSACTIONSTATE_HXX

#include <iostream>

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
         Confirmed,
         Terminated,
         Bogus
      } State;

      TransactionState(SipStack& stack, Machine m, State s);
      ~TransactionState();
      
      void processClientNonInvite(  Message* msg );
      void processClientInvite(  Message* msg );
      void processServerNonInvite(  Message* msg );
      void processServerInvite(  Message* msg );
      void processStale(  Message* msg );
      
      

   private:
      bool isRequest(Message* msg) const;
      bool isTimer(Message* msg) const;
      bool isResponse(Message* msg, int lower=0, int upper=699) const;
      bool isFromTU(Message* msg) const;
      bool isTranportError(Message* msg) const;
      bool isSentIndication(Message* msg) const;
      void sendToTU(Message* msg) const;
      void sendToWire(Message* msg) const;
      SipMessage* make100(SipMessage* request) const;
      
      SipStack& mStack;
      Machine mMachine;
      State mState;
      bool mIsReliable;
      
      TransactionState* mCancelStateMachine;

      SipMessage* mMsgToRetransmit;

      friend std::ostream& operator<<(std::ostream& strm, const TransactionState& state);
};


}

#endif
