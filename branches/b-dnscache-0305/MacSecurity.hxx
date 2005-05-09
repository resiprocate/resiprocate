#if !defined(RESIP_MACSECURITY_HXX)
#define RESIP_MACSECURITY_HXX

#include "resiprocate/Security.hxx"

namespace resip
{

// Instead of using Mac specific data types
// we pass around handles. This keeps us from
// having to include Mac OS headers.
typedef void * KeychainHandle;

/*
 * Manages certificates in the Mac Keychain.
 */
class MacSecurity : public Security
{
   public:

      MacSecurity(){};

      // load root certificates into memory
      virtual void preload();

   protected:

      // Opens a search handle to certificates store in
      // the X509Anchors keychain
      KeychainHandle openSystemCertStore();
      
      // loads root certificates into memory
      void getCerts();
      
      void closeCertifStore(KeychainHandle searchReference);
};

} // namespace resip

#endif // ndef RESIP_MACSECURITY_HXX
