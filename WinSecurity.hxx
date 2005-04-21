#if !defined(RESIP_WINSECURITY_HXX)
#define RESIP_WINSECURITY_HXX

#include "resiprocate/Security.hxx"
#include <wincrypt.h>

namespace resip
{
class WinSecurity : public Security
{
   public:

      typedef enum
      {
         ROOT_CA_STORE=1, //"ROOT": predefined system store
         CA_STORE=2,      //"CA": predefined system store 
         PRIVATE_STORE=3, //"MY": predefined system store; should store the domain certificate/private key
         USERS_STORE=4    //"DOMAIN_USERS" (only for server):
                          //administrator-defined system store; should store the
                          //certificate/private keys for the users associated
                          //with the domain
      } MsCertStoreType;
      //for details on certificate stores, see
      //http://msdn.microsoft.com/library/default.asp?url=/library/en-us/seccrypto/security/certificate_services.asp
	  WinSecurity(){};

      virtual void preload();

   protected:
      HCERTSTORE openSystemCertStore(const Data& name);
      void getCerts(MsCertStoreType eType);
      void closeCertifStore(HCERTSTORE);
};

}
#endif
