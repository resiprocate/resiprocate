#include "RegistrationCreator.hxx"
#include "DialogUsageManager.hxx"
#include "Profile.hxx"

using namespace resip;

RegistrationCreator::RegistrationCreator(DialogUsageManager& dum, const NameAddr& aor)
   : BaseCreator(dum)
{
   makeInitialRequest(aor, REGISTER);
   mLastRequest.header(h_RequestLine).uri().user() = Data::Empty;
   mLastRequest.header(h_Expires).value() = dum.getProfile()->getDefaultRegistrationTime();

   // add instance parameter to the contact for gruu !cj! TODO 

   // store caller prefs in Contact
}
