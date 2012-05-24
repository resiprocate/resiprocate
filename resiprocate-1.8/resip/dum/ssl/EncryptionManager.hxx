#if !defined(RESIP_ENCRYPTIONMANAGER_HXX)
#define RESIP_ENCRYPTIONMANAGER_HXX

#include <memory>

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "rutil/SharedPtr.hxx"
#include "rutil/Data.hxx"
#include "rutil/BaseException.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Contents.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/dum/CertMessage.hxx"
#include "resip/dum/RemoteCertStore.hxx"
#include "resip/dum/DumFeature.hxx"
#include "resip/stack/InvalidContents.hxx"

namespace resip
{
class Security;

class EncryptionManager : public DumFeature
{
   public:
      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg, const Data& file, const int line);
            const char* name() const { return "EncryptionManagerException"; }
      };

      EncryptionManager(DialogUsageManager& dum, TargetCommand::Target& target);
      virtual ~EncryptionManager();
      void setRemoteCertStore(std::auto_ptr<RemoteCertStore> store);
      virtual DumFeature::ProcessingResult process(Message* msg);

   private:

      typedef enum
      {
         Pending,
         Complete
      } Result;

      EncryptionManager::Result processCertMessage(CertMessage* cert);
      Contents* sign(SharedPtr<SipMessage> msg, const Data& senderAor, bool* noCerts);
      Contents* encrypt(SharedPtr<SipMessage> msg, const Data& recipientAor, bool* noCerts);
      Contents* signAndEncrypt(SharedPtr<SipMessage> msg, const Data& senderAor, const Data& recipientAor, bool* noCerts);
      bool decrypt(SipMessage* msg);

      class Request
      {
         public:
            Request(DialogUsageManager& dum, RemoteCertStore* store, SharedPtr<SipMessage> msg, DumFeature& feature);
            virtual ~Request();
            virtual Result received(bool success, MessageId::Type type, const Data& aor, const Data& data) = 0;
            Data getId() const { return mMsgToEncrypt->getTransactionId(); }            
            //void setTaken() { mTaken = true; }
            //void handleInvalidContents(SipMessage*, const Data& originalBody, const Mime& originalType);

         protected:
            DialogUsageManager& mDum;
            RemoteCertStore* mStore;
            SharedPtr<SipMessage> mMsgToEncrypt; // initial message.
            int mPendingRequests;
            DumFeature& mFeature;
            //bool mTaken;

            void response415();
      };

      class Sign : public Request
      {
         public:
            Sign(DialogUsageManager& dum, RemoteCertStore* store, SharedPtr<SipMessage> msg, const Data& senderAor, DumFeature& feature);
            virtual ~Sign();
            Result received(bool success, MessageId::Type type, const Data& aor, const Data& data);
            bool sign(Contents**, bool* noCerts);

         protected:
            Data mSenderAor;
      };

      class Encrypt : public Request
      {
         public:
            Encrypt(DialogUsageManager& dum, RemoteCertStore* store, SharedPtr<SipMessage> msg, const Data& recipientAor, DumFeature& feature);
            virtual ~Encrypt();
            Result received(bool success, MessageId::Type type, const Data& aor, const Data& data);
            bool encrypt(Contents**, bool* noCerts);

         protected:
            Data mRecipientAor;
      };

      class SignAndEncrypt : public Request
      {
         public:
            SignAndEncrypt(DialogUsageManager& dum, RemoteCertStore* store, SharedPtr<SipMessage> msg,  const Data& senderAor, const Data& recipientAor, DumFeature& feature);
            ~SignAndEncrypt();
            Result received(bool success, MessageId::Type type, const Data& aor, const Data& data);
            bool signAndEncrypt(Contents**, bool* noCerts);

         protected:
            Data mSenderAor;
            Data mRecipientAor;

         private:
            Contents* doWork();
      };

      class Decrypt : public Request
      {
         public:
            Decrypt(DialogUsageManager& dum, RemoteCertStore* store, SipMessage* msg, DumFeature& feature);
            virtual ~Decrypt();
            Result received(bool success, MessageId::Type type, const Data& aor, const Data& data);
            bool decrypt(Helper::ContentsSecAttrs& csa);
            const Mime& getOriginalContentsType() const { return mOriginalMsgContentsType; }
            const Data& getOriginalContents() const { return mOriginalMsgContents; }
            void handleInvalidContents();
            Data getId() const { return mMsgToDecrypt->getTransactionId(); }

         private:
            bool isEncrypted();
            bool isSigned(bool noDecryptionKey);
            bool isEncryptedRecurse(Contents**);
            bool isSignedRecurse(Contents**, const Data& decryptorAor, bool noDecryptionKey);
            Helper::ContentsSecAttrs getContents(SipMessage* msg, Security& security, bool noDecryption);
            Contents* getContentsRecurse(Contents**, Security&, bool, SecurityAttributes* attr);
            InvalidContents* createInvalidContents(Contents*);
            bool isMultipart(Contents*);
            Data mDecryptor;
            Data mSigner;
            Data mOriginalMsgContents;
            Mime mOriginalMsgContentsType;
            bool mIsEncrypted; // the whole body is encrypted in original message.
            SipMessage* mMsgToDecrypt; // original messge.
            bool mMessageTaken;
 };

      std::auto_ptr<RemoteCertStore> mRemoteCertStore;

      typedef std::list<Request*> RequestList;
      RequestList mRequests;
};

}

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
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
