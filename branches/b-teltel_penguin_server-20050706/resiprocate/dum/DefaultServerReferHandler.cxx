#include "resiprocate/dum/DefaultServerReferHandler.hxx"
#include "resiprocate/dum/ServerSubscription.hxx"
#include "resiprocate/os/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

void 
DefaultServerReferHandler::onNewSubscription(ServerSubscriptionHandle, const SipMessage& sub)
{}

void 
DefaultServerReferHandler::onRefresh(ServerSubscriptionHandle, const SipMessage& sub)
{}

void 
DefaultServerReferHandler::onTerminated(ServerSubscriptionHandle)
{}

//!dcm! -- not thread-safe
DefaultServerReferHandler* DefaultServerReferHandler::Instance()
{
   static DefaultServerReferHandler mInstance;
   return &mInstance;
}


bool 
DefaultServerReferHandler::hasDefaultExpires() const
{
   return true;
}

int 
DefaultServerReferHandler::getDefaultExpires() const
{
   return 60;
}
