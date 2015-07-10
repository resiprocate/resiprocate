
#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "resip/dum/ssl/EncryptionManager.hxx"

#include "resip/stack/ssl/Security.hxx"

#include "rutil/ResipAssert.h"
#include <list>
#include "rutil/BaseException.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/dum/BaseUsage.hxx"
#include "resip/dum/Handles.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/stack/Contents.hxx"
#include "resip/stack/MultipartSignedContents.hxx"
#include "resip/stack/MultipartMixedContents.hxx"
#include "resip/stack/MultipartAlternativeContents.hxx"
#include "resip/stack/MultipartRelatedContents.hxx"
#include "resip/stack/Pkcs7Contents.hxx"
#include "resip/dum/RemoteCertStore.hxx"
#include "resip/stack/SecurityAttributes.hxx"
#include "resip/dum/CertMessage.hxx"
#include "resip/dum/TargetCommand.hxx"
#include "resip/dum/DumDecrypted.hxx"
#include "resip/dum/DumFeature.hxx"
#include "resip/dum/DumFeatureChain.hxx"
#include "resip/dum/OutgoingEvent.hxx"
#include "resip/dum/DumHelper.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/WinLeakCheck.hxx"

#if defined(USE_SSL)

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

static bool isAckOrCancelOrBye(const SipMessage& msg)
{
   MethodTypes method = msg.header(h_RequestLine).getMethod();
   return (method == ACK || method == CANCEL || method == BYE);
}

EncryptionManager::Exception::Exception(const Data& msg, const Data& file, const int line)
   : BaseException(msg,file,line)
{
}

EncryptionManager::EncryptionManager(DialogUsageManager& dum, TargetCommand::Target& target)
   : DumFeature(dum, target)
{
}

EncryptionManager::~EncryptionManager()
{
   for (list<Request*>::iterator it = mRequests.begin(); it != mRequests.end(); ++it)
   {
      delete *it;
   }
   mRequests.clear();
}

void EncryptionManager::setRemoteCertStore(std::auto_ptr<RemoteCertStore> store)
{
   ErrLog(<< "Async currently is not supported");
   resip_assert(0);
   //mRemoteCertStore = store;
}

DumFeature::ProcessingResult EncryptionManager::process(Message* msg)
{
   SipMessage* sipMsg = dynamic_cast<SipMessage*>(msg);

   if (sipMsg)
   {
      if (sipMsg->getContents())
      {
         if (decrypt(sipMsg))
         {
            DebugLog(<< "Decrypted message:" << sipMsg << endl);
            return DumFeature::FeatureDone;
         }
         else
         {
            return DumFeature::EventTaken;
         }
      }
      else
      {
         return DumFeature::FeatureDone;
      }
   }

   OutgoingEvent* event = dynamic_cast<OutgoingEvent*>(msg);
   if (event)
   {
      if (!event->message()->getContents())
      {
         return DumFeature::FeatureDone;
      }

      if (!event->message()->getSecurityAttributes() ||
          event->message()->getSecurityAttributes()->getOutgoingEncryptionLevel() == SecurityAttributes::None || 
          event->message()->getSecurityAttributes()->encryptionPerformed())
      {
         return DumFeature::FeatureDone;
      }
      
      Data senderAor;
      Data recipAor;
      if (event->message()->isRequest())
      {
         senderAor = event->message()->header(h_From).uri().getAor();
         recipAor = event->message()->header(h_To).uri().getAor();
      }
      else
      {
         senderAor = event->message()->header(h_To).uri().getAor();
         recipAor = event->message()->header(h_From).uri().getAor();
      }

      Contents* contents = event->message()->getContents();
      bool setContents = true;
      bool noCerts = false;

      switch (event->message()->getSecurityAttributes()->getOutgoingEncryptionLevel())
      {
         case SecurityAttributes::None:
            setContents = false;
            break;
         case SecurityAttributes::Encrypt:
            contents = encrypt(event->message(), recipAor, &noCerts);
            break;
         case SecurityAttributes::Sign:
            contents = sign(event->message(), senderAor, &noCerts);
            break;
         case SecurityAttributes::SignAndEncrypt:
            contents = signAndEncrypt(event->message(), senderAor, recipAor, &noCerts);
            break;
      }

      if (contents)
      {
         if (setContents)
         {
            event->message()->setContents(auto_ptr<Contents>(contents));
            DumHelper::setEncryptionPerformed(*event->message());
         }
         return DumFeature::FeatureDone;
      }
      else
      {
         if (noCerts)
         {
            return DumFeature::ChainDoneAndEventDone;
         }
         else
         {
            //event->releaseMessage();
            return DumFeature::EventTaken;
         }
      }
   }

   // Todo: change CertMessage to DumFeatureMessage.
   CertMessage* certMsg = dynamic_cast<CertMessage*>(msg);
   if (certMsg)
   {
      EncryptionManager::Result result = processCertMessage(certMsg);
      if (result == Complete)
      {
         return DumFeature::FeatureDone;
      }
      else
      {
         delete msg;
         return DumFeature::EventTaken;
      }
   }

   return DumFeature::FeatureDone;
}

EncryptionManager::Result EncryptionManager::processCertMessage(CertMessage* message)
{
   InfoLog( << "Received a cert message: " << *message << endl);
   Result ret = Pending;
   list<Request*>::iterator it;
   for (it = mRequests.begin(); it != mRequests.end(); ++it)
   {
      if ((*it)->getId() == message->id().mId) break;
   }

   if (it != mRequests.end())
   {
      InfoLog ( << "Processing the cert message" << endl);
      ret = (*it)->received(message->success(), message->id().mType, message->id().mAor, message->body());
      if (Complete == ret)
      {
         delete *it;
         mRequests.erase(it);
      }
   }

   return ret;
}

Contents* EncryptionManager::sign(SharedPtr<SipMessage> msg, 
                                  const Data& senderAor,
                                  bool* noCerts)
{
   Sign* request = new Sign(mDum, mRemoteCertStore.get(), msg, senderAor, *this);
   Contents* contents;
   *noCerts = false;
   bool async = request->sign(&contents, noCerts);
   if (async)
   {
      InfoLog(<< "Async sign" << endl);
      //request->setTaken();
      mRequests.push_back(request);
   }
   else
   {
      delete request;
   }
   return contents;
}

Contents* EncryptionManager::encrypt(SharedPtr<SipMessage> msg,
                                     const Data& recipientAor,
                                     bool* noCerts)
{
   Encrypt* request = new Encrypt(mDum, mRemoteCertStore.get(), msg, recipientAor, *this);
   Contents* contents;
   *noCerts = false;
   bool async = request->encrypt(&contents, noCerts);
   if (async)
   {
      InfoLog(<< "Async encrypt" << endl);
      //request->setTaken();
      mRequests.push_back(request);
   }
   else
   {
      delete request;
   }
   return contents;
}

Contents* EncryptionManager::signAndEncrypt(SharedPtr<SipMessage> msg,
                                            const Data& senderAor,
                                            const Data& recipAor,
                                            bool* noCerts)
{
   SignAndEncrypt* request = new SignAndEncrypt(mDum, mRemoteCertStore.get(), msg, senderAor, recipAor, *this);
   Contents* contents;
   *noCerts = false;
   bool async = request->signAndEncrypt(&contents, noCerts);
   if (!async)
   {
      delete request;
   }
   else
   {
      InfoLog(<< "Async sign and encrypt" << endl);
      //request->setTaken();
      mRequests.push_back(request);
   }
   return contents;
}

bool EncryptionManager::decrypt(SipMessage* msg)
{
   Decrypt* request = new Decrypt(mDum, mRemoteCertStore.get(), msg, *this);
   bool ret = true;
   
   Helper::ContentsSecAttrs csa;
   ret = request->decrypt(csa);

   if (ret)
   {
      if (csa.mContents.get())
      {
         msg->setContents(csa.mContents);
         
         if (csa.mAttributes.get()) 
         {
            // Security Attributes might already exist on the message if the Identity Handler is enabled
            // if so, ensure we maintain the Identity Strength
            const SecurityAttributes *origSecurityAttributes = msg->getSecurityAttributes();
            if(origSecurityAttributes)
            {
                csa.mAttributes->setIdentityStrength(origSecurityAttributes->getIdentityStrength());
            }
            msg->setSecurityAttributes(csa.mAttributes);
         }
      }
      else
      {
         // no valid contents.
         request->handleInvalidContents();

         if (msg->isRequest() && !isAckOrCancelOrBye(*msg))
         {
            ret = false;
         }
      }
      delete request;
   }   
   else
   {
      InfoLog(<< "Async decrypt" << endl);
      mRequests.push_back(request);
   }
   
   return ret;
}

EncryptionManager::Request::Request(DialogUsageManager& dum,
                                    RemoteCertStore* store,
                                    SharedPtr<SipMessage> msg,
                                    DumFeature& feature)
    : mDum(dum),
     mStore(store),
     mMsgToEncrypt(msg),
     mPendingRequests(0),
     mFeature(feature)
     //mTaken(false)
{
}

EncryptionManager::Request::~Request()
{
   /*
   if (mTaken)
   {
      delete mMsg;
   }
   */
}

void EncryptionManager::Request::response415()
{
   SipMessage* response = Helper::makeResponse(*mMsgToEncrypt, 415);
   mDum.post(response);
   InfoLog(<< "Generated 415" << endl);
}

EncryptionManager::Sign::Sign(DialogUsageManager& dum,
                              RemoteCertStore* store,
                              SharedPtr<SipMessage> msg, 
                              const Data& senderAor,
                              DumFeature& feature)
   : Request(dum, store, msg, feature),
     mSenderAor(senderAor)
{
}

EncryptionManager::Sign::~Sign()
{
}

bool EncryptionManager::Sign::sign(Contents** contents, bool* noCerts)
{
   *contents = 0;
   *noCerts = false;
   bool async = false;

   MultipartSignedContents* msc = 0;
   bool missingCert = !mDum.getSecurity()->hasUserCert(mSenderAor);
   bool missingKey = !mDum.getSecurity()->hasUserPrivateKey(mSenderAor);

   if (!missingCert && !missingKey)
   {
      InfoLog(<< "Signing message" << endl);
      msc =  mDum.getSecurity()->sign(mSenderAor, mMsgToEncrypt->getContents());
      *contents = msc;
   }
   else
   {
      if (mStore)
      {
         if (missingCert)
         {
            InfoLog(<< "Fetching cert for " << mSenderAor << endl);
            ++mPendingRequests;
            MessageId id(mMsgToEncrypt->getTransactionId(), mSenderAor, MessageId::UserCert);
            mStore->fetch(mSenderAor, MessageId::UserCert, id, mDum);
         }
         if (missingKey)
         {
            InfoLog(<< "Fetching private key for " << mSenderAor << endl);
            ++mPendingRequests;
            MessageId id(mMsgToEncrypt->getTransactionId(), mSenderAor, MessageId::UserPrivateKey);
            mStore->fetch(mSenderAor, MessageId::UserCert, id, mDum);
         }
         async = true;
      }
      else
      {
         InfoLog(<< "No remote cert store installed" << endl);
         *noCerts = true;
         response415();
      }
   }

   return async;
}

EncryptionManager::Result EncryptionManager::Sign::received(bool success, 
                                                            MessageId::Type type,
                                                            const Data& aor, 
                                                            const Data& data)
{
   resip_assert(mSenderAor==aor);
   resip_assert(mPendingRequests>0&&mPendingRequests<=2);
   Result result = Pending;

   if (success)
   {
      if (type == MessageId::UserCert)
      {
         InfoLog(<< "Adding cert for: " << aor << endl);
         mDum.getSecurity()->addUserCertDER(aor, data);
      }
      else
      {
         InfoLog(<< "Adding private key for " << aor << endl);
         mDum.getSecurity()->addUserPrivateKeyDER(aor, data);
      }         
      if (--mPendingRequests == 0)
      {
         InfoLog(<< "Signing message" << endl);
         MultipartSignedContents* msc = mDum.getSecurity()->sign(aor, mMsgToEncrypt->getContents());
         mMsgToEncrypt->setContents(auto_ptr<Contents>(msc));
         DumHelper::setEncryptionPerformed(*mMsgToEncrypt);
         OutgoingEvent* event = new OutgoingEvent(mMsgToEncrypt);
         //mTaken = false;
         mDum.post(new TargetCommand(mDum.dumOutgoingTarget(), auto_ptr<Message>(event)));
         result = Complete;
      }
   }
   else
   {
      InfoLog(<< "Failed to fetch " << ((type==MessageId::UserCert)? "cert " : "private key ") << "for " << aor <<endl);
      response415();
      result = Complete;
   }
   return result;
}

EncryptionManager::Encrypt::Encrypt(DialogUsageManager& dum, 
                                    RemoteCertStore* store, 
                                    SharedPtr<SipMessage> msg, 
                                    const Data& recipientAor,
                                    DumFeature& feature)
   : Request(dum, store, msg, feature),
     mRecipientAor(recipientAor)
{
}

EncryptionManager::Encrypt::~Encrypt()
{
}

bool EncryptionManager::Encrypt::encrypt(Contents** contents, bool* noCerts)
{
   *contents = 0;
   *noCerts = false;
   bool async = false;

   if (mDum.getSecurity()->hasUserCert(mRecipientAor))
   {
      InfoLog(<< "Encrypting message" << endl);
      MultipartAlternativeContents* alt = dynamic_cast<MultipartAlternativeContents*>(mMsgToEncrypt->getContents());
      if (alt)
      {
         // encrypt the last part.
         MultipartMixedContents::Parts parts = alt->parts();
         Contents* last = mDum.getSecurity()->encrypt(parts.back(), mRecipientAor);
         if (last)
         {
            MultipartAlternativeContents* mac = new MultipartAlternativeContents(*alt);
            delete mac->parts().back();
            mac->parts().pop_back();
            mac->parts().push_back(last);
            *contents = mac;
         }
      }
      else
      {
         *contents =  mDum.getSecurity()->encrypt(mMsgToEncrypt->getContents(), mRecipientAor);
      }
   }
   else
   {
      if (mStore)
      {
         InfoLog(<< "Fetching cert for " << mRecipientAor << endl);
         ++mPendingRequests;
         MessageId id(mMsgToEncrypt->getTransactionId(), mRecipientAor, MessageId::UserCert);
         mStore->fetch(mRecipientAor, MessageId::UserCert, id, mDum);
         async = true;
      }
      else
      {
         InfoLog(<< "No remote cert store installed" << endl);
         *noCerts = true;
         response415();
      }
   }
   return async;
}

EncryptionManager::Result EncryptionManager::Encrypt::received(bool success, 
                                                               MessageId::Type type,
                                                               const Data& aor, 
                                                               const Data& data)
{
   resip_assert(mRecipientAor==aor);
   resip_assert(type==MessageId::UserCert);
   resip_assert(mPendingRequests==1);
   if (success)
   {
      InfoLog(<< "Adding user cert for " << aor << endl);
      mDum.getSecurity()->addUserCertDER(aor, data);
      --mPendingRequests;
      InfoLog(<< "Encrypting message" << endl);
      Pkcs7Contents* encrypted = mDum.getSecurity()->encrypt(mMsgToEncrypt->getContents(), aor);
      mMsgToEncrypt->setContents(auto_ptr<Contents>(encrypted));
      DumHelper::setEncryptionPerformed(*mMsgToEncrypt);
      OutgoingEvent* event = new OutgoingEvent(mMsgToEncrypt);
      //mTaken = false;
      mDum.post(new TargetCommand(mDum.dumOutgoingTarget(), auto_ptr<Message>(event)));      
   }
   else
   {
      InfoLog(<< "Failed to fetch cert for " << aor << endl);
      response415();
   }
   return Complete;
}

EncryptionManager::SignAndEncrypt::SignAndEncrypt(DialogUsageManager& dum, 
                                                  RemoteCertStore* store, 
                                                  SharedPtr<SipMessage> msg, 
                                                  const Data& senderAor, 
                                                  const Data& recipientAor,
                                                  DumFeature& feature)
   : Request(dum, store, msg, feature),
     mSenderAor(senderAor),
     mRecipientAor(recipientAor)
{
}

EncryptionManager::SignAndEncrypt::~SignAndEncrypt()
{
}

bool EncryptionManager::SignAndEncrypt::signAndEncrypt(Contents** contents, bool* noCerts)
{
   *contents = 0;
   *noCerts = false;
   bool async = false;
   bool missingCert = !mDum.getSecurity()->hasUserCert(mSenderAor);
   bool missingKey = !mDum.getSecurity()->hasUserPrivateKey(mSenderAor);
   bool missingRecipCert = !mDum.getSecurity()->hasUserCert(mRecipientAor);

   if (!missingCert && !missingKey && !missingRecipCert)
   {
      InfoLog(<< "Encrypting and signing message" << endl);
      *contents = doWork();
   }
   else
   {
      if (mStore)
      {
         if (missingCert)
         {
            InfoLog(<< "Fetching cert for " << mSenderAor << endl);
            ++mPendingRequests;
            MessageId id(mMsgToEncrypt->getTransactionId(), mSenderAor, MessageId::UserCert);
            mStore->fetch(mSenderAor, MessageId::UserCert, id, mDum);
         }
         if (missingKey)
         {
            InfoLog(<< "Fetching private key for " << mSenderAor << endl);
            ++mPendingRequests;
            MessageId id(mMsgToEncrypt->getTransactionId(), mSenderAor, MessageId::UserPrivateKey);
            mStore->fetch(mSenderAor, MessageId::UserCert, id, mDum);
         }
         if (missingRecipCert)
         {
            InfoLog(<< "Fetching cert for " << mRecipientAor << endl);
            ++mPendingRequests;
            MessageId id(mMsgToEncrypt->getTransactionId(), mRecipientAor, MessageId::UserCert);
            mStore->fetch(mSenderAor, MessageId::UserCert, id, mDum);
         }
         async = true;
      }
      else
      {
         InfoLog(<< "No remote cert store installed" << endl);
         *noCerts = true;
         response415();
      }
   }

   return async;
}

EncryptionManager::Result EncryptionManager::SignAndEncrypt::received(bool success, 
                                                                      MessageId::Type type,
                                                                      const Data& aor, 
                                                                      const Data& data)
{
   resip_assert(mPendingRequests>0&&mPendingRequests<=3);
   Result result = Pending;
   if (success)
   {
      if (type == MessageId::UserCert)
      {
         resip_assert(aor==mSenderAor||aor==mRecipientAor);
         InfoLog(<< "Adding user cert for " << aor << endl);
         mDum.getSecurity()->addUserCertDER(aor, data);
      }
      else
      {
         resip_assert(aor==mSenderAor);
         InfoLog(<< "Adding private key for " << aor << endl);
         mDum.getSecurity()->addUserPrivateKeyDER(aor, data);
      }

      if (--mPendingRequests == 0)
      {
         InfoLog(<< "Encrypting and signing message" << endl);
         Contents* contents = doWork();
         mMsgToEncrypt->setContents(auto_ptr<Contents>(contents));
         DumHelper::setEncryptionPerformed(*mMsgToEncrypt);
         OutgoingEvent* event = new OutgoingEvent(mMsgToEncrypt);
         //mTaken = false;
         mDum.post(new TargetCommand(mDum.dumOutgoingTarget(), auto_ptr<Message>(event)));
         result = Complete;
      }
   }
   else
   {
      InfoLog(<< "Failed to fetch cert for " << aor << endl);
      response415();
      result = Complete;
   }
   return result;
}

Contents* EncryptionManager::SignAndEncrypt::doWork()
{
   Contents* contents = 0;
   MultipartAlternativeContents* mac = dynamic_cast<MultipartAlternativeContents*>(mMsgToEncrypt->getContents());
   if (mac)
   {
      MultipartMixedContents::Parts parts = mac->parts();
      Pkcs7Contents* pkcs7 = mDum.getSecurity()->encrypt(parts.back(), mRecipientAor);
      if (pkcs7)
      {
         MultipartAlternativeContents* alt = new MultipartAlternativeContents(*mac);
         delete alt->parts().back();
         alt->parts().pop_back();
         alt->parts().push_back(pkcs7);
         contents = alt;
      }
   }
   else
   {
      contents = mDum.getSecurity()->encrypt(mMsgToEncrypt->getContents() , mRecipientAor);
   }

   if (contents)
   {
      contents = mDum.getSecurity()->sign(mSenderAor, contents);
   }

   return contents;
}

EncryptionManager::Decrypt::Decrypt(DialogUsageManager& dum,
                                    RemoteCertStore* store, 
                                    SipMessage* msg,
                                    DumFeature& feature)
   : Request(dum, store, SharedPtr<SipMessage>(), feature),
     mIsEncrypted(false),
     mMsgToDecrypt(msg),
     mMessageTaken(false)
{
   if (msg->isResponse())
   {
      mDecryptor = msg->header(h_From).uri().getAor();
      mSigner = msg->header(h_To).uri().getAor();
   }
   else
   {
      mDecryptor = msg->header(h_To).uri().getAor();
      mSigner = msg->header(h_From).uri().getAor();
   }
}

EncryptionManager::Decrypt::~Decrypt()
{
   if (mMessageTaken)
   {
      delete mMsgToDecrypt;
   }
}

bool EncryptionManager::Decrypt::decrypt(Helper::ContentsSecAttrs& csa)
{
   bool noDecryptionKey = false;

   if (!dynamic_cast<Pkcs7Contents*>(mMsgToDecrypt->getContents()))
   {
      mOriginalMsgContents = Data(mMsgToDecrypt->getContents()->getHeaderField().getBuffer(), mMsgToDecrypt->getContents()->getHeaderField().getLength());
      mOriginalMsgContentsType = mMsgToDecrypt->getContents()->getType();
   }
   else
   {
      mIsEncrypted = true;
   }

   if (isEncrypted())
   {
      bool missingDecryptorCert = !mDum.getSecurity()->hasUserCert(mDecryptor);
      bool missingDecryptorKey = !mDum.getSecurity()->hasUserPrivateKey(mDecryptor);
      if (missingDecryptorCert || missingDecryptorKey)
      {
         if (mStore)
         {
            if (missingDecryptorCert)
            {
               InfoLog(<< "Fetching user cert for " << mDecryptor << endl);
               ++mPendingRequests;
               MessageId id(mMsgToDecrypt->getTransactionId(), mDecryptor, MessageId::UserCert);
               mStore->fetch(mDecryptor, MessageId::UserCert, id, mDum);
            }

            if (missingDecryptorKey)
            {
               InfoLog(<< "Fetching private key for " << mDecryptor << endl);
               ++mPendingRequests;
               MessageId id(mMsgToDecrypt->getTransactionId(), mDecryptor, MessageId::UserPrivateKey);
               mStore->fetch(mDecryptor, MessageId::UserPrivateKey, id, mDum);
            }
            mMessageTaken = true;
            return false;
         }
         else
         {
            InfoLog(<< "No remote cert store installed" << endl);
            noDecryptionKey = true;
         }
      }
   }

   if (isSigned(noDecryptionKey))
   {
      if (!mDum.getSecurity()->hasUserCert(mSigner))
      {
         if (mStore)
         {
            InfoLog(<< "Fetching user cert for " << mSigner << endl);
            ++mPendingRequests;
            MessageId id(mMsgToDecrypt->getTransactionId(), mSigner, MessageId::UserCert);
            mStore->fetch(mSigner, MessageId::UserCert, id, mDum);
            mMessageTaken = true;
            return false;
         }
         else
         {
            InfoLog(<< "No remote cert store installed" << endl);
         }
      }
   }

   csa = getContents(mMsgToDecrypt, *mDum.getSecurity(), noDecryptionKey);
   return true;
}

EncryptionManager::Result EncryptionManager::Decrypt::received(bool success, 
                                                               MessageId::Type type, 
                                                               const Data& aor, 
                                                               const Data& data)
{
   Result result = Complete;
   resip_assert(mPendingRequests>0 && mPendingRequests<=2);
   if (success)
   {
      if (aor == mSigner)
      {
         resip_assert(MessageId::UserCert == type);
         resip_assert(mPendingRequests==1);
         --mPendingRequests;
         InfoLog(<< "Adding user cert for " << aor << endl);
         mDum.getSecurity()->addUserCertDER(aor, data);
      }
      else
      {
         resip_assert(aor == mDecryptor);
         if (MessageId::UserCert == type)
         {
            InfoLog(<< "Adding user cert for " << aor << endl);
            mDum.getSecurity()->addUserCertDER(aor, data);
         }
         else
         {
            InfoLog(<< "Adding private key for " << aor << endl);
            mDum.getSecurity()->addUserPrivateKeyDER(aor, data);
         }

         if (--mPendingRequests == 0)
         {
            if (isSigned(false))
            {
               if (!mDum.getSecurity()->hasUserCert(mSigner))
               {
                  InfoLog(<< "Fetching user cert for " << mSigner << endl);
                  ++mPendingRequests;
                  MessageId id(mMsgToDecrypt->getTransactionId(), mSigner, MessageId::UserCert);
                  mStore->fetch(mSigner, MessageId::UserCert, id, mDum);
                  result = Pending;
               }
            }
         }
         else
         {
            result = Pending;
         }
      }
   }
   else
   {
      InfoLog(<< "Failed to fetch cert for " << aor << endl);
   }

   if (Complete == result)
   {
      Helper::ContentsSecAttrs csa;
      csa = getContents(mMsgToDecrypt, *mDum.getSecurity(), 
                        (!mDum.getSecurity()->hasUserCert(mDecryptor) || !mDum.getSecurity()->hasUserPrivateKey(mDecryptor)));


      if (csa.mContents.get())
      {
         csa.mContents->checkParsed();
         mMsgToDecrypt->setContents(csa.mContents);
         
         if (csa.mAttributes.get()) 
         {
            mMsgToDecrypt->setSecurityAttributes(csa.mAttributes);
         }         
      }
      else
      {
         // no valid contents.
         ErrLog(<< "No valid contents in message received" << endl);
         handleInvalidContents();

         if (mMsgToDecrypt->isRequest() && !isAckOrCancelOrBye(*mMsgToDecrypt))
         {
            return result;
         }
      }      

      // Todo: make CertMessage DumFeatureMessage and get rid of DumDecrypted.
      // Currently the message will not be processed by 
      // any features in the chain after EncryptionManager.
      DumDecrypted* decrypted = new DumDecrypted(*mMsgToDecrypt);
      mDum.post(decrypted);
   }

   return result;
}

bool EncryptionManager::Decrypt::isEncrypted()
{
   Contents* contents = mMsgToDecrypt->getContents();
   return isEncryptedRecurse(&contents);
}

bool EncryptionManager::Decrypt::isSigned(bool noDecryptionKey)
{
   Contents* contents = mMsgToDecrypt->getContents();
   return isSignedRecurse(&contents, mDecryptor, noDecryptionKey);
}

bool EncryptionManager::Decrypt::isEncryptedRecurse(Contents** contents)
{
   InvalidContents* ic;
   if ((ic = dynamic_cast<InvalidContents*>(*contents)))
   {
      return false;
   }

   Pkcs7Contents* pk;
   if ((pk = dynamic_cast<Pkcs7Contents*>(*contents)))
   {
      return true;
   }

   MultipartSignedContents* mps;
   if ((mps = dynamic_cast<MultipartSignedContents*>(*contents)))
   {
      try
      {
         mps->parts();
      }
      catch (BaseException& e)
      {
         // structure of multipart is bad. this is unrecoverable error,
         // replace the whole multipart contents with an InvalidContents.
         ErrLog(<< e.name() << endl << e.getMessage());

         if (*contents == mMsgToDecrypt->getContents())
         {
            mMsgToDecrypt->setContents(auto_ptr<Contents>(createInvalidContents(mps)));
         }
         else
         {
            *contents = createInvalidContents(mps);
            delete mps;
         }
         return false;
      }

      return isEncryptedRecurse(&(*(mps->parts().begin())));
   }

   MultipartAlternativeContents* alt = dynamic_cast<MultipartAlternativeContents*>(*contents);
   if (alt)
   {
      try
      {
         alt->parts();
      }
      catch (BaseException& e)
      {
         ErrLog(<< e.name() << endl << e.getMessage());
         if (*contents == mMsgToDecrypt->getContents())
         {
            mMsgToDecrypt->setContents(auto_ptr<Contents>(createInvalidContents(alt)));
         }
         else
         {
            *contents = createInvalidContents(alt);
            delete alt;
         }
         return false;
      }

      for (MultipartAlternativeContents::Parts::reverse_iterator i = alt->parts().rbegin();
           i != alt->parts().rend(); ++i)
      {
         if (isEncryptedRecurse(&(*i)))
         {
            return true;
         }
      }

      return false;
   }

   if (dynamic_cast<MultipartMixedContents*>(*contents))
   {
      return false;
   }

   return false;
}

bool EncryptionManager::Decrypt::isSignedRecurse(Contents** contents,
                                                 const Data& decryptorAor,
                                                 bool noDecryptionKey)
{
   InvalidContents* ic;
   if ((ic = dynamic_cast<InvalidContents*>(*contents)))
   {
      return false;
   }

   Pkcs7Contents* pk;
   if ((pk = dynamic_cast<Pkcs7Contents*>(*contents)))
   {
      if (noDecryptionKey) 
      {
         return false;
      }

      Contents* decrypted = mDum.getSecurity()->decrypt(decryptorAor, pk);
      bool ret = false;
      
      if (decrypted)
      {
         if (*contents == mMsgToDecrypt->getContents())
         {
            mOriginalMsgContents = Data(decrypted->getHeaderField().getBuffer(), decrypted->getHeaderField().getLength());
            mOriginalMsgContentsType = decrypted->getType();
         }
         
         try
         {
            decrypted->checkParsed();
            if (isMultipart(decrypted))
            {
               if (dynamic_cast<MultipartSignedContents*>(decrypted))
               {
                  ret = true;
               }
               else
               {
                  if (*contents == mMsgToDecrypt->getContents())
                  {
                     mMsgToDecrypt->setContents(auto_ptr<Contents>(decrypted));
                     *contents = mMsgToDecrypt->getContents();
                  }
                  else
                  {
                     *contents = decrypted;
                     delete pk;
                  }
                  return isSignedRecurse(contents, decryptorAor, noDecryptionKey);
               }
            }
         }
         catch (BaseException& e)
         {
            ErrLog(<< e.name() << endl << e.getMessage());

            if (*contents == mMsgToDecrypt->getContents())
            {
               mMsgToDecrypt->setContents(auto_ptr<Contents>(createInvalidContents(decrypted)));
            }
            else
            {
               *contents = createInvalidContents(decrypted);
               delete pk;
            }
         }

         delete decrypted;
      }

      return ret;
   }

   MultipartSignedContents* mps;
   if ((mps = dynamic_cast<MultipartSignedContents*>(*contents)))
   {
      return true;
   }

   MultipartAlternativeContents* alt = dynamic_cast<MultipartAlternativeContents*>(*contents);
   if (alt)
   {
      try
      {
         alt->parts();
      }
      catch (BaseException& e)
      {
         // structure of multipart is bad. this is unrecoverable error,
         // replace the whole multipart contents with an InvalidContents.
         ErrLog(<< e.name() << endl << e.getMessage());

         if (*contents == mMsgToDecrypt->getContents())
         {
            mMsgToDecrypt->setContents(auto_ptr<Contents>(createInvalidContents(alt)));
         }
         else
         {
            *contents = createInvalidContents(alt);
            delete alt;
         }

         return false;
      }

      for (MultipartAlternativeContents::Parts::reverse_iterator i = alt->parts().rbegin();
           i != alt->parts().rend(); ++i)
      {
         if (isSignedRecurse(&(*i), decryptorAor, noDecryptionKey))
         {
            return true;
         }
      }

      return false;
   }

   if (dynamic_cast<MultipartMixedContents*>(*contents))
   {
      return false;
   }

   return false;
}

Helper::ContentsSecAttrs EncryptionManager::Decrypt::getContents(SipMessage* message,
                                                                 Security& security,
                                                                 bool noDecryptionKey)
{
   SecurityAttributes* attr = new SecurityAttributes;
   attr->setIdentity(message->header(h_From).uri().getAor());
   Contents* contents = message->getContents();
   if (contents)
   {
      contents = getContentsRecurse(&contents, security, noDecryptionKey, attr);
   }

   if (contents)
   {
      if (mIsEncrypted)
      {
         attr->setEncrypted();
      }
   }

   std::auto_ptr<Contents> c(contents);
   std::auto_ptr<SecurityAttributes> a(attr);
   return Helper::ContentsSecAttrs(c, a);
}

Contents* EncryptionManager::Decrypt::getContentsRecurse(Contents** tree,
                                                         Security& security,
                                                         bool noDecryptionKey,
                                                         SecurityAttributes* attributes)
{
   InvalidContents* ic;
   if ((ic = dynamic_cast<InvalidContents*>(*tree)))
   {
      return 0;
   }

   Pkcs7Contents* pk;
   if ((pk = dynamic_cast<Pkcs7Contents*>(*tree)))
   {
      if (noDecryptionKey) 
      {
         return 0;
      }

      Contents* contents = security.decrypt(mDecryptor, pk);
      if (contents)
      {
         if (*tree == mMsgToDecrypt->getContents())
         {
            mOriginalMsgContents = Data(contents->getHeaderField().getBuffer(), contents->getHeaderField().getLength());
            mOriginalMsgContentsType = contents->getType();
         }

         try
         {
            contents->checkParsed();
            if (isMultipart(contents))
            {
               if (*tree == mMsgToDecrypt->getContents())
               {
                  mMsgToDecrypt->setContents(auto_ptr<Contents>(contents));
                  *tree = mMsgToDecrypt->getContents();
               }
               else
               {
                  *tree = contents;
                  delete pk;
               }

               return getContentsRecurse(tree, security, noDecryptionKey, attributes);
            }
            else
            {
               attributes->setEncrypted();
            }
         }
         catch (BaseException& e)
         {
            ErrLog(<< e.name() << endl << e.getMessage());

            if (*tree == mMsgToDecrypt->getContents())
            {
               mMsgToDecrypt->setContents(auto_ptr<Contents>(createInvalidContents(contents)));
            }
            else
            {
               *tree = createInvalidContents(contents);
               delete pk;
            }

            delete contents;
            return 0;
         }
      }

      return contents;
   }

   MultipartSignedContents* mps;
   if ((mps = dynamic_cast<MultipartSignedContents*>(*tree)))
   {
      Data signer;
      SignatureStatus sigStatus = SignatureIsBad;
      Contents* tmp = security.checkSignature(mps, &signer, &sigStatus);
      Contents* contents = getContentsRecurse(&tmp,
                                              security, noDecryptionKey, attributes);
      attributes->setSigner(signer);
      attributes->setSignatureStatus(sigStatus);
      return contents;
   }

   MultipartAlternativeContents* alt;
   if ((alt = dynamic_cast<MultipartAlternativeContents*>(*tree)))
   {
      try
      {
         alt->parts();
      }
      catch (BaseException& e)
      {
         // structure of multipart is bad. this is an unrecoverable error,
         // replace the whole multipart contents with an InvalidContents.
         ErrLog(<< e.name() << endl << e.getMessage());

         if (*tree == mMsgToDecrypt->getContents())
         {
            mMsgToDecrypt->setContents(auto_ptr<Contents>(createInvalidContents(alt)));
         }
         else
         {
            *tree = createInvalidContents(alt);
            delete alt;
         }

         return 0;
      }

      for (MultipartAlternativeContents::Parts::reverse_iterator i = alt->parts().rbegin();
           i != alt->parts().rend(); ++i)
      {
         Contents* contents = getContentsRecurse(&(*i), security, noDecryptionKey, attributes);
         if (contents)
         {
            return contents;
         }
      }

      return 0;
   }

   MultipartMixedContents* mult;
   if ((mult = dynamic_cast<MultipartMixedContents*>(*tree)))
   {
      try
      {
         mult->parts();
      }
      catch (BaseException& e)
      {
         // structure of multipart is bad. this is unrecoverable error,
         // replace the whole multipart contents with an InvalidContents.
         ErrLog(<< e.name() << endl << e.getMessage());

         if (*tree == mMsgToDecrypt->getContents())
         {
            mMsgToDecrypt->setContents(auto_ptr<Contents>(createInvalidContents(mult)));
         }
         else
         {
            *tree = createInvalidContents(mult);
            delete mult;
         }

         return 0;
      }

      // for now, the multipart/mixed is returned untouched.
      return mult->clone();
   }

   Contents* ret = 0;

   try
   {
      (*tree)->checkParsed();
      ret = (*tree)->clone();
   }
   catch (BaseException& e)
   {
      ErrLog(<< e.name() << endl << e.getMessage());

      if (*tree == mMsgToDecrypt->getContents())
      {
         mMsgToDecrypt->setContents(auto_ptr<Contents>(createInvalidContents(*tree)));
      }
      else
      {
         Contents* tmp = *tree;
         *tree = createInvalidContents(*tree);
         delete tmp;
      }
   }

   return ret;

}

// Todo: move to DumHelper.
InvalidContents*
EncryptionManager::Decrypt::createInvalidContents(Contents* orig)
{
   Data original(orig->getHeaderField().getBuffer(), orig->getHeaderField().getLength());
   return new InvalidContents(original, orig->getType());
}

bool
EncryptionManager::Decrypt::isMultipart(Contents* contents)
{
   return (
      dynamic_cast<MultipartSignedContents*>(contents) ||
      dynamic_cast<MultipartAlternativeContents*>(contents) ||
      dynamic_cast<MultipartMixedContents*>(contents)
      );
}

void
EncryptionManager::Decrypt::handleInvalidContents()
{
   if (mMsgToDecrypt->isRequest())
   {
      if (isAckOrCancelOrBye(*mMsgToDecrypt))
      {
         DebugLog(<< "No valid contents in the request" << endl);
         InvalidContents* invalid = new InvalidContents(mOriginalMsgContents, mOriginalMsgContentsType);
         mMsgToDecrypt->setContents(auto_ptr<Contents>(invalid));
      }
      else
      {
         DebugLog(<< "No valid contents in the request -- reject with 400" << endl);
         SipMessage response;
         Helper::makeResponse(response, *mMsgToDecrypt, 400, Data::Empty, mMsgToDecrypt->header(h_RequestLine).uri().host() , "Invalid message body");
         mDum.getSipStack().send(response);
      }
   }
   else
   {
      DebugLog(<< "No valid contents in the response" << endl);
      InvalidContents* invalid = new InvalidContents(mOriginalMsgContents, mOriginalMsgContentsType);
      mMsgToDecrypt->setContents(auto_ptr<Contents>(invalid));
   }
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
