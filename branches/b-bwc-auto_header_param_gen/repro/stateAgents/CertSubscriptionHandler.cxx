#if defined(USE_SSL)

#include "resip/stack/Security.hxx"
#include "resip/stack/X509Contents.hxx"
#include "repro/stateAgents/CertSubscriptionHandler.hxx"

using namespace repro;
using namespace resip;

CertSubscriptionHandler::CertSubscriptionHandler(Security& security) : mSecurity(security)
{
}

void 
CertSubscriptionHandler::onNewSubscription(ServerSubscriptionHandle h, const SipMessage& sub)
{
   if (!mSecurity.hasUserCert(h->getDocumentKey()))
   {
      // !jf! really need to do this async. send neutral state in the meantime,
      // blah blah blah
      mSecurity.generateUserCert(h->getDocumentKey());
   }

   if (mSecurity.hasUserCert(h->getDocumentKey()))
   {
      X509Contents x509(mSecurity.getUserCertDER(h->getDocumentKey()));
      h->send(h->update(&x509));
   }
   else
   {
      h->reject(404);
   }
}

void 
CertSubscriptionHandler::onPublished(ServerSubscriptionHandle associated, 
                                     ServerPublicationHandle publication, 
                                     const Contents* contents,
                                     const SecurityAttributes* attrs)
{
   associated->send(associated->update(contents));
}


void 
CertSubscriptionHandler::onTerminated(ServerSubscriptionHandle)
{
}

void 
CertSubscriptionHandler::onError(ServerSubscriptionHandle, const SipMessage& msg)
{
}

#endif
