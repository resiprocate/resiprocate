#ifndef RESIP_SecurityAttributes_hxx
#define RESIP_SecurityAttributes_hxx

#include <iostream>

#include "rutil/Data.hxx"

namespace resip
{

enum SignatureStatus
{
   SignatureNone, // there is no signature
   SignatureIsBad,
   SignatureTrusted, // It is signed with trusted signature
   SignatureCATrusted, // signature is new and is signed by a root we trust
   SignatureNotTrusted, // signature is new and is not signed by a CA we
   SignatureSelfSigned
};

class SecurityAttributes
{
   public:
      SecurityAttributes();
      ~SecurityAttributes();      

      typedef enum {None, Sign, Encrypt, SignAndEncrypt} OutgoingEncryptionLevel;

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

      const Data& getIdentity() const
      {
         return mIdentity;
      }

      void setIdentityStrength(IdentityStrength strength)
      {
         mStrength = strength;         
      }      

      IdentityStrength getIdentityStrength() const
      {
         return mStrength;
      }
      
      void setSigner(const Data& signer)
      {
         mSigner = signer;
      }

      const Data& getSigner() const
      {
         return mSigner;
      }

      OutgoingEncryptionLevel getOutgoingEncryptionLevel() const
      {
         return mLevel;
      }

      void setOutgoingEncryptionLevel(OutgoingEncryptionLevel level)
      {
         mLevel = level;
      }

      bool encryptionPerformed() const
      {
         return mEncryptionPerformed;
      }

      void setEncryptionPerformed(bool performed)
      {
         mEncryptionPerformed = performed;
      }

   friend EncodeStream& operator<<(EncodeStream& strm, const SecurityAttributes& sa);

   private:
      bool mIsEncrypted;
      SignatureStatus mSigStatus;
      Data mSigner;
      Data mIdentity;
      IdentityStrength mStrength;
      OutgoingEncryptionLevel mLevel; // for outgoing messages.
      bool mEncryptionPerformed;
};

   EncodeStream& operator<<(EncodeStream& strm, const SecurityAttributes& sa);
}

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
