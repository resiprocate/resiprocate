#if !defined(Repro_CertPublicationHandler_hxx)
#define Repro_CertPublicationHandler_hxx

#if defined(USE_SSL)

#include "resip/dum/PublicationHandler.hxx"

namespace resip
{
class Security;
class SipMessage;
class SecurityAttributes;
class Data;
class Contents;
}

namespace repro
{

class CertPublicationHandler : public resip::ServerPublicationHandler
{
   public:
      CertPublicationHandler(resip::Security& security);
      virtual void onInitial(resip::ServerPublicationHandle h, 
                             const resip::Data& etag, 
                             const resip::SipMessage& pub, 
                             const resip::Contents* contents,
                             const resip::SecurityAttributes* attrs, 
                             UInt32 expires);
      virtual void onExpired(resip::ServerPublicationHandle h, const resip::Data& etag);
      virtual void onRefresh(resip::ServerPublicationHandle, 
                             const resip::Data& etag, 
                             const resip::SipMessage& pub, 
                             const resip::Contents* contents,
                             const resip::SecurityAttributes* attrs,
                             UInt32 expires);
      virtual void onUpdate(resip::ServerPublicationHandle h, 
                            const resip::Data& etag, 
                            const resip::SipMessage& pub, 
                            const resip::Contents* contents,
                            const resip::SecurityAttributes* attrs,
                            UInt32 expires);
      virtual void onRemoved(resip::ServerPublicationHandle h, const resip::Data& etag, const resip::SipMessage& pub, UInt32 expires);
      
   private:
      void add(resip::ServerPublicationHandle h, const resip::Contents* contents);
      
      resip::Security& mSecurity;
};
 
}

#endif 
#endif
