#ifndef RESIP_SecurityAttributes_hxx
#define RESIP_SecurityAttributes_hxx

#include "resiprocate/Security.hxx"

namespace resip
{

class SecurityAttributes
{
   public:
      SecurityAttributes()  {};
      ~SecurityAttributes() {};

      Security::SignatureStatus getSignatureStatus() const
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
      
      void setSignatureStatus(Security::SignatureStatus status)
      {
         mSigStatus = status;
      }

      void setIdentity(const Data& identity)
      {
         mIdentity = identity;
      }

      void setSigner(const Data& signer)
      {
         mSigner = signer;
      }

   private:
      bool mIsEncrypted;
      Security::SignatureStatus mSigStatus;
      Data mSigner;
      Data mIdentity;
};

}

#endif
