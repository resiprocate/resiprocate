
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/NetworkAssociation.hxx"
#include "resiprocate/dum/KeepAliveManager.hxx"

using namespace resip;

void 
NetworkAssociation::update(const SipMessage& msg, int keepAliveInterval)
{
   if (mDum && mDum->mKeepAliveManager.get())
   {
      if (!(msg.getSource() == mTarget))
      {
         mDum->mKeepAliveManager->remove(mTarget);
         mTarget = msg.getSource();
         mDum->mKeepAliveManager->add(mTarget, keepAliveInterval);
      }
   }
}

NetworkAssociation::~NetworkAssociation()
{
   if (mDum && mDum->mKeepAliveManager.get())
   {
      mDum->mKeepAliveManager->remove(mTarget);
   }
}
      
   
