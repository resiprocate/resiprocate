#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/TransactionController.hxx"
#include "resiprocate/TransactionState.hxx"
#include "resiprocate/os/Logger.hxx"


using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSACTION


TransactionController::TransactionController(Fifo<Message>& tufifo, bool stateless) : 
   mStateless(stateless),
   mRegisteredForTransactionTermination(false),
   mDiscardStrayResponses(false),
   mTUFifo(tufifo),
   mTransportSelector(mStateMacFifo),
   mStatelessHandler(*this),
   mTimers(mStateMacFifo),
   StatelessIdCounter(1)
{
}

TransactionController::~TransactionController()
{
}

void
TransactionController::process(FdSet& fdset)
{
   mTransportSelector.process(fdset);
   mTimers.process();

   while (mStateMacFifo.messageAvailable())
   {
      if (mStateless)
      {
         mStatelessHandler.process();
      }
      else
      {
         TransactionState::process(*this);
      }
   }
}

int 
TransactionController::getTimeTillNextProcessMS()
{
   if ( mStateMacFifo.messageAvailable() ) 
   {
      return 0;
   }
   else if ( mTransportSelector.hasDataToSend() )
   {
      return 0;
   }
   
   int ret = mTimers.msTillNextTimer();

#if 1 // !cj! just keep a max of 500ms for good luck - should not be needed   
   if ( ret > 1 )
   {
      ret = 1;
   }
#endif

   return ret;
} 
   
void 
TransactionController::buildFdSet( FdSet& fdset)
{
   mTransportSelector.buildFdSet( fdset );
}

void
TransactionController::addTransport( TransportType protocol, 
                                     int port,
                                     const Data& hostName,
                                     const Data& nic) 
{
   mTransportSelector.addTransport(protocol, port, hostName, nic);
}

void 
TransactionController::addTlsTransport( int port, 
                                        const Data& keyDir,
                                        const Data& privateKeyPassPhrase,
                                        const Data& domainname,
                                        const Data& hostName,
                                        const Data& nic) 
{
   mTransportSelector.addTlsTransport(domainname, keyDir, privateKeyPassPhrase, port, hostName, nic);
}

void
TransactionController::send(SipMessage* msg)
{
   mStateMacFifo.add(msg);
}

void
TransactionController::registerForTransactionTermination()
{
   mRegisteredForTransactionTermination = true;
}
