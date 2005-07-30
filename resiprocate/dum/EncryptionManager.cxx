#include <cassert>
#include <list>
#include "resiprocate/os/BaseException.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/dum/BaseUsage.hxx"
#include "resiprocate/dum/Handles.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/Security.hxx"
#include "resiprocate/Contents.hxx"
#include "resiprocate/MultipartSignedContents.hxx"
#include "resiprocate/MultipartMixedContents.hxx"
#include "resiprocate/MultipartAlternativeContents.hxx"
#include "resiprocate/MultipartRelatedContents.hxx"
#include "resiprocate/Pkcs7Contents.hxx"
#include "resiprocate/dum/RemoteCertStore.hxx"
#include "resiprocate/SecurityAttributes.hxx"
#include "resiprocate/dum/CertMessage.hxx"
#include "resiprocate/dum/TargetCommand.hxx"
#include "resiprocate/dum/EncryptionManager.hxx"
#include "resiprocate/dum/DumDecrypted.hxx"
#include "resiprocate/dum/DumFeature.hxx"
#include "resiprocate/dum/DumFeatureChain.hxx"
#include "resiprocate/dum/OutgoingEvent.hxx"
#include "resiprocate/os/Logger.hxx"

#if defined(USE_SSL)

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

EncryptionManager::Exception::Exception(const Data& msg, const Data& file, const int line)
   : BaseException(msg,file,line)
{
}

EncryptionManager::EncryptionManager(DialogUsageManager& dum, TargetCommand::Target& target)
   : DumFeature(dum, target),
     mCounter(0)
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
   mRemoteCertStore = store;
}

DumFeature::ProcessingResult EncryptionManager::process(Message* msg)
{
   SipMessage* sipMsg = dynamic_cast<SipMessage*>(msg);
   if (sipMsg)
   {
      if (decrypt(*sipMsg))
      {
         return DumFeature::FeatureDone;
      }
      else
      {
         return DumFeature::EventTaken;
      }
   }

   OutgoingEvent* event = dynamic_cast<OutgoingEvent*>(msg);
   if (event)
   {
      if (!event->message().getContents())
      {
         return DumFeature::FeatureDone;
      }

      Data senderAor;
      Data recipAor;
      if (event->message().isRequest())
      {
         senderAor = event->message().header(h_From).uri().getAor();
         recipAor = event->message().header(h_To).uri().getAor();
      }
      else
      {
         senderAor = event->message().header(h_To).uri().getAor();
         recipAor = event->message().header(h_From).uri().getAor();
      }

      Contents* contents = event->message().getContents();
      bool setContents = true;
      switch (event->encryptionLevel())
      {
         case DialogUsageManager::None:
            setContents = false;
            break;
         case DialogUsageManager::Encrypt:
            contents = encrypt(event->message(), recipAor);
            break;
         case DialogUsageManager::Sign:
            contents = sign(event->message(), senderAor);
            break;
         case DialogUsageManager::SignAndEncrypt:
            contents = signAndEncrypt(event->message(), senderAor, recipAor);
            break;
      }

      if (contents)
      {
         if (setContents)
         {
            event->message().setContents(auto_ptr<Contents>(contents));
         }
         return DumFeature::FeatureDone;
      }
      else
      {
         return DumFeature::EventTaken;
      }
   }

   CertMessage* certMsg = dynamic_cast<CertMessage*>(msg);
   if (certMsg)
   {
      if (processCertMessage(*certMsg) == Complete)
      {
         return DumFeature::FeatureDoneAndEventDone;
      }
      else
      {
         return DumFeature::EventTaken;
      }
   }

   return DumFeature::FeatureDone;
}

EncryptionManager::Result EncryptionManager::processCertMessage(const CertMessage& message)
{
   InfoLog( << "Received a cert message: " << message << endl);
   Result ret = Pending;
   list<Request*>::iterator it;
   for (it = mRequests.begin(); it != mRequests.end(); ++it)
   {
      if ((*it)->getId() == message.id().mId) break;
   }

   if (it != mRequests.end())
   {
      InfoLog ( << "Processing the cert message" << endl);
      Result result = (*it)->received(message.success(), message.id().mType, message.id().mAor, message.body());
      if (Complete == result)
      {
         delete *it;
         mRequests.erase(it);
      }
   }
   return ret;
}

Contents* EncryptionManager::sign(const SipMessage& msg, 
                                  const Data& senderAor)
{
   Sign* request = new Sign(mDum, mRemoteCertStore.get(), msg, senderAor, *this);
   Contents* contents;
   bool async = request->sign(&contents);
   if (async)
   {
      InfoLog(<< "Async sign" << endl);
      mRequests.push_back(request);
   }
   else
   {
      delete request;
   }
   return contents;
}

Contents* EncryptionManager::encrypt(const SipMessage& msg,
                                     const Data& recipientAor)
{
   Encrypt* request = new Encrypt(mDum, mRemoteCertStore.get(), msg, recipientAor, *this);
   Contents* contents;
   bool async = request->encrypt(&contents);
   if (async)
   {
      InfoLog(<< "Async encrypt" << endl);
      mRequests.push_back(request);
   }
   else
   {
      delete request;
   }
   return contents;
}

Contents* EncryptionManager::signAndEncrypt(const SipMessage& msg,
                                            const Data& senderAor,
                                            const Data& recipAor)
{
   SignAndEncrypt* request = new SignAndEncrypt(mDum, mRemoteCertStore.get(), msg, senderAor, recipAor, *this);
   Contents* contents;
   bool async = request->signAndEncrypt(&contents);
   if (!async)
   {
      delete request;
   }
   else
   {
      InfoLog(<< "Async sign and encrypt" << endl);
      mRequests.push_back(request);
   }
   return contents;
}

bool EncryptionManager::decrypt(SipMessage& msg)
{
   SipMessage copy(msg);
   Decrypt* request = new Decrypt(mDum, mRemoteCertStore.get(), copy, *this);
   Helper::ContentsSecAttrs csa;
   bool ret = request->decrypt(csa);
   if (ret)
   {
      if (csa.mContents.get())
      {
         msg.setContents(csa.mContents);
      }
      if (csa.mAttributes.get()) 
      {
         msg.setSecurityAttributes(csa.mAttributes);
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
                                    const SipMessage& msg,
                                    DumFeature& feature)
   : mDum(dum),
     mStore(store),
     mMsg(msg),
     mPendingRequests(0),
     mFeature(feature)
{
}

EncryptionManager::Request::~Request()
{
}

void EncryptionManager::Request::response415()
{
   SipMessage* response = Helper::makeResponse(mMsg, 415);
   mDum.post(response);
   InfoLog(<< "Generated 415" << endl);
}

EncryptionManager::Sign::Sign(DialogUsageManager& dum,
                              RemoteCertStore* store,
                              const SipMessage& msg, 
                              const Data& senderAor,
                              DumFeature& feature)
   : Request(dum, store, msg, feature),
     mSenderAor(senderAor)
{
}

EncryptionManager::Sign::~Sign()
{
}

bool EncryptionManager::Sign::sign(Contents** contents)
{
   *contents = 0;
   bool async = false;
   if (0 != mPendingRequests) return 0;

   MultipartSignedContents* msc = 0;
   bool missingCert = !mDum.getSecurity()->hasUserCert(mSenderAor);
   bool missingKey = !mDum.getSecurity()->hasUserPrivateKey(mSenderAor);

   if (!missingCert && !missingKey)
   {
      InfoLog(<< "Signing message" << endl);
      msc =  mDum.getSecurity()->sign(mSenderAor, mMsg.getContents());
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
            MessageId id(mMsg.getTransactionId(), mSenderAor, MessageId::UserCert);
            mStore->fetch(mSenderAor, MessageId::UserCert, id, mDum);
         }
         if (missingKey)
         {
            InfoLog(<< "Fetching private key for " << mSenderAor << endl);
            ++mPendingRequests;
            MessageId id(mMsg.getTransactionId(), mSenderAor, MessageId::UserPrivateKey);
            mStore->fetch(mSenderAor, MessageId::UserCert, id, mDum);
         }
         async = true;
      }
      else
      {
         InfoLog(<< "No remote cert store installed" << endl);
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
   assert(mSenderAor==aor);
   assert(mPendingRequests>0&&mPendingRequests<=2);
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
         MultipartSignedContents* msc = mDum.getSecurity()->sign(aor, mMsg.getContents());
         auto_ptr<Contents> contents(msc);
         mMsg.setContents(contents);
         OutgoingEvent* event = new OutgoingEvent(mMsg, DialogUsageManager::None);
         mFeature.postCommand(auto_ptr<Message>(event));
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
                                    const SipMessage& msg, 
                                    const Data& recipientAor,
                                    DumFeature& feature)
   : Request(dum, store, msg, feature),
     mRecipientAor(recipientAor)
{
}

EncryptionManager::Encrypt::~Encrypt()
{
}

bool EncryptionManager::Encrypt::encrypt(Contents** contents)
{
   *contents = 0;
   bool async = false;
   if (0 != mPendingRequests) return 0;

   if (mDum.getSecurity()->hasUserCert(mRecipientAor))
   {
      InfoLog(<< "Encrypting message" << endl);
      MultipartAlternativeContents* alt = dynamic_cast<MultipartAlternativeContents*>(mMsg.getContents());
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
         *contents =  mDum.getSecurity()->encrypt(mMsg.getContents(), mRecipientAor);
      }
   }
   else
   {
      if (mStore)
      {
         InfoLog(<< "Fetching cert for " << mRecipientAor << endl);
         ++mPendingRequests;
         MessageId id(mMsg.getTransactionId(), mRecipientAor, MessageId::UserCert);
         mStore->fetch(mRecipientAor, MessageId::UserCert, id, mDum);
         async = true;
      }
      else
      {
         InfoLog(<< "No remote cert store installed" << endl);
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
   assert(mRecipientAor==aor);
   assert(type==MessageId::UserCert);
   assert(mPendingRequests==1);
   if (success)
   {
      InfoLog(<< "Adding user cert for " << aor << endl);
      mDum.getSecurity()->addUserCertDER(aor, data);
      --mPendingRequests;
      InfoLog(<< "Encrypting message" << endl);
      Pkcs7Contents* encrypted = mDum.getSecurity()->encrypt(mMsg.getContents(), aor);
      mMsg.setContents(auto_ptr<Contents>(encrypted));
      OutgoingEvent* event = new OutgoingEvent(mMsg, DialogUsageManager::None);
      mFeature.postCommand(auto_ptr<Message>(event));
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
                                                  const SipMessage& msg, 
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

bool EncryptionManager::SignAndEncrypt::signAndEncrypt(Contents** contents)
{
   *contents = 0;
   bool async = false;
   if (0 != mPendingRequests) return 0;

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
            MessageId id(mMsg.getTransactionId(), mSenderAor, MessageId::UserCert);
            mStore->fetch(mSenderAor, MessageId::UserCert, id, mDum);
         }
         if (missingKey)
         {
            InfoLog(<< "Fetching private key for " << mSenderAor << endl);
            ++mPendingRequests;
            MessageId id(mMsg.getTransactionId(), mSenderAor, MessageId::UserPrivateKey);
            mStore->fetch(mSenderAor, MessageId::UserCert, id, mDum);
         }
         if (missingRecipCert)
         {
            InfoLog(<< "Fetching cert for " << mRecipientAor << endl);
            ++mPendingRequests;
            MessageId id(mMsg.getTransactionId(), mRecipientAor, MessageId::UserCert);
            mStore->fetch(mSenderAor, MessageId::UserCert, id, mDum);
         }
         async = true;
      }
      else
      {
         InfoLog(<< "No remote cert store installed" << endl);
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
   assert(mPendingRequests>0&&mPendingRequests<=3);
   Result result = Pending;
   if (success)
   {
      if (type == MessageId::UserCert)
      {
         assert(aor==mSenderAor||aor==mRecipientAor);
         InfoLog(<< "Adding user cert for " << aor << endl);
         mDum.getSecurity()->addUserCertDER(aor, data);
      }
      else
      {
         assert(aor==mSenderAor);
         InfoLog(<< "Adding private key for " << aor << endl);
         mDum.getSecurity()->addUserPrivateKeyDER(aor, data);
      }

      if (--mPendingRequests == 0)
      {
         InfoLog(<< "Encrypting and signing message" << endl);
         Contents* contents = doWork();
         mMsg.setContents(auto_ptr<Contents>(contents));
         OutgoingEvent* event = new OutgoingEvent(mMsg, DialogUsageManager::None);
         mFeature.postCommand(auto_ptr<Message>(event));
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
   MultipartAlternativeContents* mac = dynamic_cast<MultipartAlternativeContents*>(mMsg.getContents());
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
      contents = mDum.getSecurity()->encrypt(mMsg.getContents() , mRecipientAor);
   }

   if (contents)
   {
      contents = mDum.getSecurity()->sign(mSenderAor, contents);
   }

   return contents;
}

EncryptionManager::Decrypt::Decrypt(DialogUsageManager& dum,
                                    RemoteCertStore* store, 
                                    const SipMessage& msg,
                                    DumFeature& feature)
   : Request(dum, store, msg, feature)
{
   if (msg.isResponse())
   {
      mDecryptor = msg.header(h_From).uri().getAor();
      mSigner = msg.header(h_To).uri().getAor();
   }
   else
   {
      mDecryptor = msg.header(h_To).uri().getAor();
      mSigner = msg.header(h_From).uri().getAor();
   }

   if (msg.getContents())
   {
      mContents.reset(msg.getContents()->clone());
   }
}

EncryptionManager::Decrypt::~Decrypt()
{
}

bool EncryptionManager::Decrypt::decrypt(Helper::ContentsSecAttrs& csa)
{
   if (0 != mPendingRequests) return false;

   bool noDecryptionKey = false;

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
               MessageId id(mMsg.getTransactionId(), mDecryptor, MessageId::UserCert);
               mStore->fetch(mDecryptor, MessageId::UserCert, id, mDum);
            }

            if (missingDecryptorKey)
            {
               InfoLog(<< "Fetching private key for " << mDecryptor << endl);
               ++mPendingRequests;
               MessageId id(mMsg.getTransactionId(), mDecryptor, MessageId::UserPrivateKey);
               mStore->fetch(mDecryptor, MessageId::UserPrivateKey, id, mDum);
            }
            return false;
         }
         else
         {
            InfoLog(<< "No remote cert store installed" << endl);
            noDecryptionKey = true;
         }
      }
   }

   if (!noDecryptionKey && isSigned())
   {
      if (!mDum.getSecurity()->hasUserCert(mSigner))
      {
         if (mStore)
         {
            InfoLog(<< "Fetching user cert for " << mSigner << endl);
            ++mPendingRequests;
            MessageId id(mMsg.getTransactionId(), mSigner, MessageId::UserCert);
            mStore->fetch(mSigner, MessageId::UserCert, id, mDum);
            return false;
         }
         else
         {
            InfoLog(<< "No remote cert store installed" << endl);
         }
      }
   }

   csa = getContents(mMsg, *mDum.getSecurity(), noDecryptionKey);

   return true;
}

EncryptionManager::Result EncryptionManager::Decrypt::received(bool success, 
                                                               MessageId::Type type, 
                                                               const Data& aor, 
                                                               const Data& data)
{
   Result result = Complete;
   assert(mPendingRequests>0 && mPendingRequests<=2);
   if (success)
   {
      if (aor == mSigner)
      {
         assert(MessageId::UserCert == type);
         assert(mPendingRequests==1);
         --mPendingRequests;
         InfoLog(<< "Adding user cert for " << aor << endl);
         mDum.getSecurity()->addUserCertDER(aor, data);
      }
      else
      {
         assert(aor == mDecryptor);
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
            if (isSigned())
            {
               if (!mDum.getSecurity()->hasUserCert(mSigner))
               {
                  InfoLog(<< "Fetching user cert for " << mSigner << endl);
                  ++mPendingRequests;
                  MessageId id(mMsg.getTransactionId(), mSigner, MessageId::UserCert);
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
      csa = getContents(mMsg, *mDum.getSecurity(), 
                        (!mDum.getSecurity()->hasUserCert(mDecryptor) || !mDum.getSecurity()->hasUserPrivateKey(mDecryptor)));
      if (csa.mContents.get())
      {
         mMsg.setContents(csa.mContents);
      }
      if (csa.mAttributes.get()) 
      {
         mMsg.setSecurityAttributes(csa.mAttributes);
      }
      DumDecrypted* decrypted = new DumDecrypted(mMsg);
      mDum.post(decrypted);
   }

   return result;
}

bool EncryptionManager::Decrypt::isEncrypted()
{
   return isEncryptedRecurse(mMsg.getContents());
}

bool EncryptionManager::Decrypt::isSigned()
{
   return isSignedRecurse(mMsg.getContents(), mDecryptor);
}

bool EncryptionManager::Decrypt::isEncryptedRecurse(Contents* contents)
{
   Pkcs7Contents* pk;
   if ((pk = dynamic_cast<Pkcs7Contents*>(contents)))
   {
      return true;
   }

   MultipartSignedContents* mps;
   if ((mps = dynamic_cast<MultipartSignedContents*>(contents)))
   {
      return isEncryptedRecurse(*(mps->parts().begin()));
   }

   MultipartAlternativeContents* alt;
   if ((alt = dynamic_cast<MultipartAlternativeContents*>(contents)))
   {
      for (MultipartAlternativeContents::Parts::reverse_iterator i = alt->parts().rbegin();
           i != alt->parts().rend(); ++i)
      {
         if (isEncryptedRecurse(*i))
         {
            return true;
         }
      }
   }

   MultipartMixedContents* mult;
   if ((mult = dynamic_cast<MultipartMixedContents*>(contents)))
   {
      for (MultipartMixedContents::Parts::iterator i = mult->parts().begin();
           i != mult->parts().end(); ++i)
      {
         if (isEncryptedRecurse(*i))
         {
            return true;
         }
      }
   }

   return false;
}

bool EncryptionManager::Decrypt::isSignedRecurse(Contents* contents,
                                                 const Data& decryptorAor)
{
   Pkcs7Contents* pk;
   if ((pk = dynamic_cast<Pkcs7Contents*>(contents)))
   {
      Contents* decrypted = mDum.getSecurity()->decrypt(decryptorAor, pk);
      bool ret = isSignedRecurse(decrypted, decryptorAor);
      delete decrypted;
      return ret;
   }

   MultipartSignedContents* mps;
   if ((mps = dynamic_cast<MultipartSignedContents*>(contents)))
   {
      return true;
   }

   MultipartAlternativeContents* alt;
   if ((alt = dynamic_cast<MultipartAlternativeContents*>(contents)))
   {
      for (MultipartAlternativeContents::Parts::reverse_iterator i = alt->parts().rbegin();
           i != alt->parts().rend(); ++i)
      {
         if (isSignedRecurse(*i, decryptorAor))
         {
            return true;
         }
      }
   }

   MultipartMixedContents* mult;
   if ((mult = dynamic_cast<MultipartMixedContents*>(contents)))
   {
      for (MultipartMixedContents::Parts::iterator i = mult->parts().begin();
           i != mult->parts().end(); ++i)
      {
         if (isSignedRecurse(*i, decryptorAor))
         {
            return true;
         }
      }
   }

   return false;
}

Helper::ContentsSecAttrs EncryptionManager::Decrypt::getContents(const SipMessage& message,
                                                                 Security& security,
                                                                 bool noDecryptionKey)
{
   SecurityAttributes* attr = new SecurityAttributes;
   attr->setIdentity(message.header(h_From).uri().getAor());
   Contents* contents = message.getContents();
   if (contents)
   {
      contents = getContentsRecurse(contents, security, noDecryptionKey, attr);
   }

   std::auto_ptr<Contents> c(contents);
   std::auto_ptr<SecurityAttributes> a(attr);
   return Helper::ContentsSecAttrs(c, a);
}

Contents* EncryptionManager::Decrypt::getContentsRecurse(Contents* tree,
                                                         Security& security,
                                                         bool noDecryptionKey,
                                                         SecurityAttributes* attributes)
{
   Pkcs7Contents* pk;
   if ((pk = dynamic_cast<Pkcs7Contents*>(tree)))
   {
      if (noDecryptionKey) return 0;
      Contents* contents = security.decrypt(mDecryptor, pk);
      if (contents)
      {
         attributes->setEncrypted();
      }
      return contents;
   }

   MultipartSignedContents* mps;
   if ((mps = dynamic_cast<MultipartSignedContents*>(tree)))
   {
      Data signer;
      SignatureStatus sigStatus = SignatureIsBad;
      Contents* contents = getContentsRecurse(security.checkSignature(mps, &signer, &sigStatus),
                                              security, noDecryptionKey, attributes);
      attributes->setSigner(signer);
      attributes->setSignatureStatus(sigStatus);
      return contents;
   }

   MultipartAlternativeContents* alt;
   if ((alt = dynamic_cast<MultipartAlternativeContents*>(tree)))
   {
      for (MultipartAlternativeContents::Parts::reverse_iterator i = alt->parts().rbegin();
           i != alt->parts().rend(); ++i)
      {
         Contents* contents = getContentsRecurse(*i, security, noDecryptionKey, attributes);
         if (contents)
         {
            return contents;
         }
      }
   }

   MultipartMixedContents* mult;
   if ((mult = dynamic_cast<MultipartMixedContents*>(tree)))
   {
      for (MultipartMixedContents::Parts::iterator i = mult->parts().begin();
           i != mult->parts().end(); ++i)
      {
         Contents* contents = getContentsRecurse(*i, security, noDecryptionKey, attributes);
         if (contents)
         {
            return contents;
         }
      }
   }

   return tree->clone();

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
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEOR
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
