#include "ServerAuthManager.hxx"
#include <cassert>

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

bool 
ServerAuthManager::handle(UserProfile& userProfile, const SipMessage& response)
{
   assert(0);
   return true;
}
