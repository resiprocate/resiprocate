#include "ClientAuthManager.hxx"
#include <cassert>

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

bool 
ClientAuthManager::handle(SipMessage& origRequest, const SipMessage& response)
{
   // is this a 401 or 407 

   // do we have credentials for it  - get realm and user 

   

   return true;
}
