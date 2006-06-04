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


SharedPtr<MasterProfile> 
PrdManager::getMasterProfile()
{
   return mMasterProfile;
}

SipStack& 
PrdManager::getSipStack()
{
   return mStack;
}

SharedPtr<T> 
PrdManager::manage(std::auto_ptr<T> prd)
{
   
}
