#if !defined(RESIP_TRANSACTION_CONTROLLER_HXX)
#define RESIP_TRANSACTION_CONTROLLER_HXX

#include "resiprocate/os/Fifo.hxx"
#include "resiprocate/Message.hxx"
#include "resiprocate/TransactionMap.hxx"
#include "resiprocate/TransportSelector.hxx"
#include "resiprocate/StatelessHandler.hxx"
#include "resiprocate/TimerQueue.hxx"
#include "resiprocate/DnsResolver.hxx"

namespace resip
{

class TransactionController
{
   public:
      TransactionController(bool multithreaded, Fifo<Message>& tufifo, bool stateless=false);
      ~TransactionController();

      void process(FdSet& fdset);
      int getTimeTillNextProcessMS();
      void buildFdSet(FdSet& fdset);
      
      // graceful shutdown (eventually)
      void shutdown();

      void addTransport( TransportType protocol, 
                         int port,
                         const Data& hostName,
                         const Data& nic);

      void addTlsTransport( int port, 
                            const Data& keyDir,
                            const Data& privateKeyPassPhrase,
                            const Data& domainname,
                            const Data& hostName,
                            const Data& nic);
      
      void send(SipMessage* msg);

      // Inform the TU that whenever a transaction has been terminated. 
      void registerForTransactionTermination();
      
   private:
      TransactionController(const TransactionController& rhs);
      TransactionController& operator=(const TransactionController& rhs);
      
      bool mMultiThreaded;
      bool mStateless;
      bool mRegisteredForTransactionTermination;
      
      // If true, indicate to the Transaction to ignore responses for which
      // there is no transaction. 
      // !jf! Probably should transmit stray responses statelessly. see RFC3261
      bool mDiscardStrayResponses;

      // fifo used to communicate to the transaction state machine within the
      // stack. Not for external use by the application. May contain, sip
      // messages (requests and responses), timers (used by state machines),
      // asynchronous dns responses, transport errors from the underlying
      // transports, etc. 
      // For stateless stacks, this has a different behavior and does not create
      // a transaction for each request and does not do any special transaction
      // processing for requests or responses
      Fifo<Message> mStateMacFifo;

      // from the sipstack (for convenience)
      Fifo<Message>& mTUFifo;

      // Used to decide which transport to send a sip message on. 
      TransportSelector mTransportSelector;

      // stores all of the transactions that are currently active in this stack 
      TransactionMap mClientTransactionMap;
      TransactionMap mServerTransactionMap;

      // Used to handle the stateless stack incoming requests and responses as
      // well as maintaining a state machine for the async dns responses
      StatelessHandler mStatelessHandler;

      // timers associated with the transactions. When a timer fires, it is
      // placed in the mStateMacFifo
      TimerQueue  mTimers;

      unsigned long StatelessIdCounter;

      friend class DnsResolver;
      friend class StatelessHandler;
      friend class TransactionState;
      friend class TransportSelector;

      friend class TestDnsResolver;
      friend class TestFSM;
};


}


#endif
