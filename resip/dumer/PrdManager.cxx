#include "resip/dumer/PrdManager.hxx"
#include "resip/stack/SipStack.hxx"
#include "rutil/Logger.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

PrdManager::PrdManager(SipStack& stack, 
                       SharedPtr<MasterProfile> profile, 
                       bool createDefaultFeatures) : 
   mStack(stack),
   mMasterProfile(profile)
{
   mStack.registerTransactionUser(*this);
   
}


PrdManager::~PrdManager()
{
}


void
PrdManager::requestTerminate()
{
   InfoLog (<< "request shutdown");   
   // this instructs the TU to no longer accept new transactions. There is no
   // event in response to this. 
   mStack.requestTransactionUserShutdown(*this);
   
   // !jf! go through all PRDs in some order and request that they go away. 
}

void
PrdManager::onTerminated()
{
   requestShutdown();
}

void 
PrdManager::requestShutdown()
{
   InfoLog (<< "request shutdown");
   // !jf! assert that there are no PRDs in the PrdManager

   // an event will come back from SipStack when this is completed. 
   // TransactionUserMessage->type() == TransactionUserMessage::TransactionUserRemoved
   mShutdownState = RemovingTransactionUser;
   mStack.unregisterTransactionUser(*this);
}


const Data& 
PrdManager::name() const
{
   static Data n("PrdManager");
   return n;
}



SipStack& 
PrdManager::getSipStack()
{
   return mStack;
}

void 
PrdManager::unmanage(Prd& prd)
{
   mFifo.post(new UnmanagePrdManagerCommand(*this, prd.getId()));
}

void 
PrdManager::send(SharedPtr<SipMessage> msg)
{
   std::auto_ptr<SipMessage> toSend(static_cast<SipMessage*>(msg->clone()));
   mFifo.post(new SendPrdManagerCommand(*this, toSend));
}

SharedPtr<MasterProfile> 
PrdManager::getMasterProfile()
{
   return mMasterProfile;
}

void 
PrdManager::post(std::auto_ptr<PrdCommand>, Postable& postable, unsigned long timeMs)
{
}



SharedPtr<T> 
PrdManager::manage(std::auto_ptr<T> prd)
{
   mFifo.post(new ManagePrdManagerCommand(*this, SharedPtr<Prd>(prd.release())));
}

bool 
PrdManager::process()
{
   if (mFifo.messageAvailable())
   {
      internalProcess(std::auto_ptr<Message>(mFifo.getNext()));
   }
   return mFifo.messageAvailable();
}

void
PrdManager::internalProcess(std::auto_ptr<Message> msg)
{
   // After a Stack ShutdownMessage has been received, don't do anything else in dum
   if (mShutdownState == Shutdown)
   {
      return;
   }
   
   TransactionUserMessage* tuMsg = dynamic_cast<TransactionUserMessage*>(msg.get());
   if (tuMsg)
   {
      InfoLog (<< "TU unregistered ");
      assert(mShutdownState == RemovingTransactionUser);
      assert(tuMsg->type() == TransactionUserMessage::TransactionUserRemoved);
      mShutdownState = Shutdown;
      onShutdown();
      return;
   }
   
   PrdManagerCommand* rud = dynamic_cast<PrdManagerCommand*>(msg.get());
   if (prd)
   {
      (*prd)();
      return;
   }
   
   KeepAliveTimeout* keepAliveMsg = dynamic_cast<KeepAliveTimeout*>(msg.get());
   if (keepAliveMsg)
   {
      if (mKeepAliveManager.get())
      {
         mKeepAliveManager->process(*keepAliveMsg);
      }
      return;      
   }

   ConnectionTerminated* terminated = dynamic_cast<ConnectionTerminated*>(msg.get());
   if (terminated)
   {
      DebugLog(<< "connection terminated message");
      if (mConnectionTerminatedEventDispatcher.dispatch(msg.get()))
      {
         msg.release();
      }
      return;
   }
   
   incomingProcess(msg);
}
