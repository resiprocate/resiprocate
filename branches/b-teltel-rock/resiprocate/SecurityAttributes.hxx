#ifndef RESIP_SecurityAttributes_hxx
#define RESIP_SecurityAttributes_hxx

#include "resiprocate/os/Data.hxx"

namespace resip
{

enum SignatureStatus
{
   SignatureNone, // there is no signature
   SignatureIsBad,
   SignatureTrusted, // It is signed with trusted signature
   SignatureCATrusted, // signature is new and is signed by a root we trust
   SignatureNotTrusted // signature is new and is not signed by a CA we
};

class SecurityAttributes
{
   public:
      SecurityAttributes();
      ~SecurityAttributes();      

      typedef enum {From, FailedIdentity, Identity} IdentityStrength;

      SignatureStatus getSignatureStatus() const
      {
         return mSigStatus;
      }

      bool isEncrypted() const
      {
         return mIsEncrypted;
      }
      void setEncrypted()
      {
         mIsEncrypted = true;
      }
      
      void setSignatureStatus(SignatureStatus status)
      {
         mSigStatus = status;
      }

      void setIdentity(const Data& identity)
      {
         mIdentity = identity;
      }

      void setIdentityStrength(IdentityStrength strength)
      {
         mStrength = strength;         
      }      

      void setSigner(const Data& signer)
      {
         mSigner = signer;
      }

   private:
      bool mIsEncrypted;
      SignatureStatus mSigStatus;
      Data mSigner;
      Data mIdentity;
      IdentityStrength mStrength;
};

}

#endif
