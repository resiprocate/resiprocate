#if defined(USE_SSL)

#include "resip/stack/Security.hxx"
#include "resip/stack/Pkcs8Contents.hxx"
#include "resip/dum/ServerPublication.hxx"
#include "repro/stateAgents/PrivateKeyPublicationHandler.hxx"

using namespace repro;
using namespace resip;

PrivateKeyPublicationHandler::PrivateKeyPublicationHandler(Security& security) : mSecurity(security)
{
}

void 
PrivateKeyPublicationHandler::onInitial(ServerPublicationHandle h, 
                                        const Data& etag, 
                                        const SipMessage& pub, 
                                        const Contents* contents,
                                        const SecurityAttributes* attrs, 
                                        int expires)
{
   add(h, contents);
}

void 
PrivateKeyPublicationHandler::onExpired(ServerPublicationHandle h, const Data& etag)
{
   mSecurity.removeUserPrivateKey(h->getPublisher());
}

void 
PrivateKeyPublicationHandler::onRefresh(ServerPublicationHandle, 
                                        const Data& etag, 
                                        const SipMessage& pub, 
                                        const Contents* contents,
                                        const SecurityAttributes* attrs,
                                        int expires)
{
}

void 
PrivateKeyPublicationHandler::onUpdate(ServerPublicationHandle h, 
                                       const Data& etag, 
                                       const SipMessage& pub, 
                                       const Contents* contents,
                                       const SecurityAttributes* attrs,
                                       int expires)
{
   add(h, contents);
}

void 
PrivateKeyPublicationHandler::onRemoved(ServerPublicationHandle h, const Data& etag, const SipMessage& pub, int expires)
{
   mSecurity.removeUserPrivateKey(h->getPublisher());
}

void 
PrivateKeyPublicationHandler::add(ServerPublicationHandle h, const Contents* contents)
{
   if (h->getDocumentKey() == h->getPublisher())
   {
      const Pkcs8Contents* pkcs8 = dynamic_cast<const Pkcs8Contents*>(contents);
      assert(pkcs8);
      mSecurity.addUserPrivateKeyDER(h->getPublisher(), pkcs8->getBodyData());
   }
   else
   {
      h->send(h->accept(403)); // !jf! is this the correct code? 
   }
}

#endif
