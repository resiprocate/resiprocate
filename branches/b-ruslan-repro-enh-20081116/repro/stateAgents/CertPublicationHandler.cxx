#if defined(USE_SSL)

#include "resip/stack/Security.hxx"
#include "resip/stack/X509Contents.hxx"
#include "resip/dum/ServerPublication.hxx"
#include "repro/stateAgents/CertPublicationHandler.hxx"

using namespace repro;
using namespace resip;

CertPublicationHandler::CertPublicationHandler(Security& security) : mSecurity(security)
{
}

void 
CertPublicationHandler::onInitial(ServerPublicationHandle h, 
                                  const Data& etag, 
                                  const SipMessage& pub, 
                                  const Contents* contents,
                                  const SecurityAttributes* attrs, 
                                  UInt32 expires)
{
   add(h, contents);
}

void 
CertPublicationHandler::onExpired(ServerPublicationHandle h, const Data& etag)
{
   mSecurity.removeUserCert(h->getPublisher());
}

void 
CertPublicationHandler::onRefresh(ServerPublicationHandle, 
                                  const Data& etag, 
                                  const SipMessage& pub, 
                                  const Contents* contents,
                                  const SecurityAttributes* attrs,
                                  UInt32 expires)
{
}

void 
CertPublicationHandler::onUpdate(ServerPublicationHandle h, 
                                 const Data& etag, 
                                 const SipMessage& pub, 
                                 const Contents* contents,
                                 const SecurityAttributes* attrs,
                                 UInt32 expires)
{
   add(h, contents);
}

void 
CertPublicationHandler::onRemoved(ServerPublicationHandle h, const Data& etag, const SipMessage& pub, UInt32 expires)
{
   mSecurity.removeUserCert(h->getPublisher());
}

void 
CertPublicationHandler::add(ServerPublicationHandle h, const Contents* contents)
{
   if (h->getDocumentKey() == h->getPublisher())
   {
      const X509Contents* x509 = dynamic_cast<const X509Contents*>(contents);
      assert(x509);
      mSecurity.addUserCertDER(h->getPublisher(), x509->getBodyData());
      h->send(h->accept(200));
   }
   else
   {
      h->send(h->accept(403)); // !jf! is this the correct code? 
   }
}

#endif
