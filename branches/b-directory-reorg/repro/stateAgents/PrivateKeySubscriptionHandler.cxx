#if defined(USE_SSL)

#include "resiprocate/Pkcs8Contents.hxx"
#include "resiprocate/Security.hxx"
#include "dum/ServerSubscription.hxx"
#include "dum/ServerPublication.hxx"
#include "repro/stateAgents/PrivateKeySubscriptionHandler.hxx"

using namespace repro;
using namespace resip;

PrivateKeySubscriptionHandler::PrivateKeySubscriptionHandler(resip::Security& security) : mSecurity(security)
{
}

void 
PrivateKeySubscriptionHandler::onNewSubscription(ServerSubscriptionHandle h, const SipMessage& sub)
{
   if (h->getDocumentKey() != h->getSubscriber())
   {
      h->send(h->accept(403)); // !jf! is this the correct code? 
   }
   else if (mSecurity.hasUserCert(h->getDocumentKey()))
   {
      Pkcs8Contents pkcs(mSecurity.getUserPrivateKeyDER(h->getDocumentKey()));
      h->send(h->update(&pkcs));
   }
   else
   {
      h->reject(404);
   }
}

void 
PrivateKeySubscriptionHandler::onPublished(ServerSubscriptionHandle associated, 
                                           ServerPublicationHandle publication, 
                                           const Contents* contents,
                                           const SecurityAttributes* attrs)
{
   associated->send(associated->update(contents));
}

void 
PrivateKeySubscriptionHandler::onTerminated(ServerSubscriptionHandle)
{
}

void 
PrivateKeySubscriptionHandler::onError(ServerSubscriptionHandle, const SipMessage& msg)
{
}

#endif
