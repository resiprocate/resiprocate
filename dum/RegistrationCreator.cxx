#include "RegistrationCreator.hxx"
#include "DialogUsageManager.hxx"
#include "Profile.hxx"

using namespace resip;

RegistrationCreator::RegistrationCreator(DialogUsageManager& dum)
   : BaseCreator(dum)
{
   makeInitialRequest(mDum.getProfile()->getDefaultAor(), REGISTER);
}
