#include "resiprocate/dum/RegistrationCreator.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/os/Logger.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

RegistrationCreator::RegistrationCreator(DialogUsageManager& dum, const NameAddr& aor)
   : BaseCreator(dum)
{
   makeInitialRequest(aor, REGISTER);
   mLastRequest.header(h_RequestLine).uri().user() = Data::Empty;
   mLastRequest.header(h_Expires).value() = dum.getProfile()->getDefaultRegistrationTime();

   // add instance parameter to the contact for gruu !cj! TODO 

   // store caller prefs in Contact
}
