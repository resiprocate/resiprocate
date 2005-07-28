#include "InviteSessionCreator.hxx"
#include "resiprocate/SdpContents.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/MasterProfile.hxx"

using namespace resip;

InviteSessionCreator::InviteSessionCreator(DialogUsageManager& dum, 
                                           const NameAddr& target, 
                                           UserProfile& userProfile,                                           
                                           const SdpContents* initial, 
                                           ServerSubscriptionHandle serverSub)
   : BaseCreator(dum, userProfile),
     mState(Initialized),
     mServerSub(serverSub)
{
   makeInitialRequest(target, INVITE);
   if(mDum.getMasterProfile()->getSupportedOptionTags().find(Token(Symbols::Timer)))
   {
       if(userProfile.getDefaultSessionTime() >= 90)
       {
           getLastRequest().header(h_SessionExpires).value() = userProfile.getDefaultSessionTime();
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

