#include "RegistrationCreator.hxx"

RegistrationCreator::RegistrationCreator(DialogUsageManager& dum)
   : BaseCreator(dum)
{
   makeInitialRequest(mDum.getProfile()->getDefaultAor(), REGISTER);
}
