#include "InviteSessionCreator.hxx"
#include "resiprocate/SdpContents.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/Profile.hxx"

using namespace resip;

InviteSessionCreator::InviteSessionCreator(DialogUsageManager& dum, 
                                           const NameAddr& target, 
                                           const NameAddr& from,
                                           const SdpContents* initial, 
                                           ServerSubscriptionHandle serverSub)
   : BaseCreator(dum),
     mState(Initialized),
     mServerSub(serverSub)
{
   makeInitialRequest(target, from, INVITE);
   if(mDum.getProfile()->getSupportedOptionTags().find(Token(Symbols::Timer)))
   {
       if(mDum.getProfile()->getDefaultSessionTime() >= 90)
       {
           getLastRequest().header(h_SessionExpires).value() = mDum.getProfile()->getDefaultSessionTime();
           //getLastRequest().header(h_MinSE).value() = 90;
       }
   }
   if (initial)
   {
      mInitialOffer = static_cast<SdpContents*>(initial->clone());
      getLastRequest().setContents(mInitialOffer);
   }
}

InviteSessionCreator::~InviteSessionCreator()
{
	delete mInitialOffer;
}

void
InviteSessionCreator::end()
{
   assert(0);
}

void
InviteSessionCreator::dispatch(const SipMessage& msg)
{
   // !jf! does this have to do with CANCELing all of the branches associated
   // with a single invite request
}

const SdpContents* 
InviteSessionCreator::getInitialOffer() const
{
   return mInitialOffer;
}

