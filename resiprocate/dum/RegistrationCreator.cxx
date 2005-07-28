#include "resiprocate/dum/RegistrationCreator.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/os/Logger.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

RegistrationCreator::RegistrationCreator(DialogUsageManager& dum, const NameAddr& target, UserProfile& userProfile, int RegistrationTime)
   : BaseCreator(dum, userProfile)
{
   makeInitialRequest(target, REGISTER);
   mLastRequest.header(h_RequestLine).uri().user() = Data::Empty;
   mLastRequest.header(h_Expires).value() = RegistrationTime;

   DebugLog ( << "RegistrationCreator::RegistrationCreator: " << mLastRequest);   
   // add instance parameter to the contact for gruu !cj! TODO 

   // store caller prefs in Contact
}

