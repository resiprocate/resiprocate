#include "resiprocate/dum/SubscriptionHandler.hxx"
#include "resiprocate/dum/ServerSubscription.hxx"
#include "resiprocate/SecurityAttributes.hxx"
#include "resiprocate/Mime.hxx"

using namespace resip;

static Mimes empty;

const Mimes& 
ServerSubscriptionHandler::getSupportedMimeTypes() const
{
   return empty;   
}

void 
ServerSubscriptionHandler::onError(ServerSubscriptionHandle, const SipMessage& msg)
{
}

void 
ServerSubscriptionHandler::onExpiredByClient(ServerSubscriptionHandle, const SipMessage& sub, SipMessage& notify)
{
}

void 
ServerSubscriptionHandler::onExpired(ServerSubscriptionHandle, SipMessage& notify)
{
}

//!dcm! -- a bit clunky, but really want these objects to not have state
bool 
ServerSubscriptionHandler::hasDefaultExpires() const
{
   return false;
}

int 
ServerSubscriptionHandler::getDefaultExpires() const
{
   return -1;
}

void 
ServerSubscriptionHandler::onRefresh(ServerSubscriptionHandle handle, const SipMessage& sub)
{
   handle->send(handle->accept(200));
   handle->send(handle->neutralNotify());
}

void 
ServerSubscriptionHandler::onPublished(ServerSubscriptionHandle associated, 
                                       ServerPublicationHandle publication, 
                                       const Contents* contents,
                                       const SecurityAttributes* attrs)
{
   // do nothing by default
}


void 
ServerSubscriptionHandler::onNotifyRejected(ServerSubscriptionHandle, const SipMessage& msg)
{   
}

