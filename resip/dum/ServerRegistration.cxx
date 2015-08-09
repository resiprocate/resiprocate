#include "resip/stack/ExtensionParameter.hxx"
#include "resip/stack/InteropHelper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/ServerRegistration.hxx"
#include "resip/dum/Dialog.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/RegistrationPersistenceManager.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Timer.hxx"
#include "rutil/TransportType.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

ServerRegistrationHandle 
ServerRegistration::getHandle()
{
   return ServerRegistrationHandle(mDum, getBaseHandle().getId());
}

ServerRegistration::ServerRegistration(DialogUsageManager& dum,  DialogSet& dialogSet, const SipMessage& request)
   : NonDialogUsage(dum, dialogSet),
     mRequest(request),
      mDidOutbound(false),
      mAsyncState(asyncStateNone)
{}

ServerRegistration::~ServerRegistration()
{
   mDialogSet.mServerRegistration = 0;
}

void 
ServerRegistration::end()
{
}

static Token outbound(Symbols::Outbound);
void
ServerRegistration::accept(SipMessage& ok)
{
   ok.remove(h_Contacts);

   InfoLog( << "accepted a registration " << mAor );

   if (mDidOutbound)
   {
      ok.header(h_Requires).push_back(outbound);
      if(InteropHelper::getFlowTimerSeconds() > 0)
      {
         ok.header(h_FlowTimer).value() = InteropHelper::getFlowTimerSeconds();         
         mDum.getSipStack().enableFlowTimer(mRequest.getSource());
      }
   }

   if (!mDum.mServerRegistrationHandler->asyncProcessing())
   {
      // Add all registered contacts to the message.
      RegistrationPersistenceManager *database = mDum.mRegistrationPersistenceManager;

      ContactList contacts;
      database->getContacts(mAor, contacts);

      //removes expired entries from the ok msg as well as calls the database to remove expired contacts.
      processFinalOkMsg(ok,contacts);

      database->unlockRecord(mAor);

      SharedPtr<SipMessage> msg(static_cast<SipMessage*>(ok.clone()));
      mDum.send(msg);
      delete(this);
   }
   else
   {
      if (mAsyncState == asyncStateQueryOnly)
      {
         if (!mAsyncLocalStore.get())
         {
            resip_assert(0);
         }
         else
         {
            std::auto_ptr<ContactRecordTransactionLog> log;
            std::auto_ptr<ContactPtrList> contacts;

            mAsyncLocalStore->releaseLog(log,contacts);

            if (contacts.get())
            {
               asyncProcessFinalOkMsg(ok,*contacts);
            }
         }

         SharedPtr<SipMessage> msg(static_cast<SipMessage*>(ok.clone()));
         mDum.send(msg);
         delete(this);        
      }
      else
      {
         if (!mAsyncLocalStore.get())
         {
            resip_assert(0);
            return;
         }
         //This register was accepted, but still need to apply the changes made by this register and then
         //receive a final contact list before sending the 200.
         mAsyncState = asyncStateAcceptedWaitingForFinalContactList;

         std::auto_ptr<ContactRecordTransactionLog> log;
         std::auto_ptr<ContactPtrList> modifiedContacts;

         mAsyncLocalStore->releaseLog(log,modifiedContacts);

         mAsyncOkMsg = SharedPtr<SipMessage>(static_cast<SipMessage*>(ok.clone()));
         mDum.mServerRegistrationHandler->asyncUpdateContacts(getHandle(),mAor,modifiedContacts,log);
         //!WARN! Must not access this object beyond this point. The client my call reject() or accept(), deleting this object.  Also, watch out for local objects that are still in scope and access this object on destruction.
         return;
      }
   }
}

void
ServerRegistration::accept(int statusCode)
{
   SipMessage success;
   mDum.makeResponse(success, mRequest, statusCode);
   // .bwc. Copy Path headers if present, indicate Path support
   // ?bwc? If path headers are present, but client indicates no path support, 
   // RFC 3327 says it is a matter of policy of whether to accept the 
   // registration or not. What should we do here? Is it worth it to make this 
   // configurable?
   if(!mRequest.empty(h_Paths))
   {
      success.header(h_Paths)=mRequest.header(h_Paths);
      success.header(h_Supporteds).push_back(Token(Symbols::Path));
   }
   accept(success);
}

void
ServerRegistration::reject(int statusCode)
{
   InfoLog( << "rejected a registration " << mAor << " with statusCode=" << statusCode );

   // First, we roll back the contact database to
   // the state it was before the registration request.

   // Async processing hasn't actually updated the database yet, so no need to roll back.
   if (mDum.mServerRegistrationHandler && !mDum.mServerRegistrationHandler->asyncProcessing())
   {
      // Rollback changes, since rejected
      RegistrationPersistenceManager *database = mDum.mRegistrationPersistenceManager;
      database->removeAor(mAor);
      if (mOriginalContacts.get())
      {
         database->addAor(mAor, *mOriginalContacts);
      }
      database->unlockRecord(mAor);
   }

   SharedPtr<SipMessage> failure(new SipMessage);
   mDum.makeResponse(*failure, mRequest, statusCode);
   failure->remove(h_Contacts);
   mDum.send(failure);
   delete(this);
}

void 
ServerRegistration::dispatch(const SipMessage& msg)
{
   DebugLog( << "got a registration" );
   
   resip_assert(msg.isRequest());
   ServerRegistrationHandler* handler = mDum.mServerRegistrationHandler;
   RegistrationPersistenceManager *database = mDum.mRegistrationPersistenceManager;

   if (!handler || (!handler->asyncProcessing() && !database))
   {
      // ?bwc? This is a server error; why are we sending a 4xx?
      // ?jmatthewsr? Possibly because of the note in section 21.5.2 about recognizing the method, but not supporting it?
       DebugLog( << "No handler or DB - sending 405" );
       
       SharedPtr<SipMessage> failure(new SipMessage);
       mDum.makeResponse(*failure, msg, 405);
       mDum.send(failure);
       delete(this);
       return;
    }

    mAor = msg.header(h_To).uri().getAorAsUri(msg.getSource().getType());

   // Checks to see whether this scheme is valid, and supported.
   if (!((mAor.scheme()=="sip" || mAor.scheme()=="sips")
         && mDum.getMasterProfile()->isSchemeSupported(mAor.scheme())))
   {
       DebugLog( << "Bad scheme in Aor" );
       
       SharedPtr<SipMessage> failure(new SipMessage);
       mDum.makeResponse(*failure, msg, 400);
       failure->header(h_StatusLine).reason() = "Bad/unsupported scheme in To: " + mAor.scheme();
       mDum.send(failure);
       delete(this);
       return;
   }
   
   if (handler->asyncProcessing())
   {
      mAsyncState = asyncStateWaitingForInitialContactList;
      handler->asyncGetContacts(getHandle(),mAor);
      //!WARN! Must not access this object beyond this point. The client my call reject() or accept(), deleting this object.  Also, watch out for local objects that are still in scope and access this object on destruction.
      return;
   }

   processRegistration(msg);
}

void
ServerRegistration::processRegistration(const SipMessage& msg)
{
   ServerRegistrationHandler* handler = mDum.mServerRegistrationHandler;
   RegistrationPersistenceManager *database = mDum.mRegistrationPersistenceManager;

   enum {ADD, REMOVE, REFRESH} operation = REFRESH;

   UInt32 globalExpires=3600;
   UInt32 returnCode=0;
   handler->getGlobalExpires(msg,mDum.getMasterProfile(),globalExpires,returnCode);

   bool async = handler->asyncProcessing();

   if (returnCode >= 400)
   {
      SharedPtr<SipMessage> failure(new SipMessage);
      mDum.makeResponse(*failure, msg, returnCode);
      if (423 == returnCode)
      {
         failure->header(h_StatusLine).reason() = "Interval Too Brief";
         failure->header(h_MinExpires).value() = globalExpires;
      }
      mDum.send(failure);
      delete(this);
      return;
   }

   if (!async)
   {
      database->lockRecord(mAor);

      mOriginalContacts = resip::SharedPtr<ContactList>(new ContactList);
      database->getContacts(mAor, *mOriginalContacts);
   }

   // If no contacts are present in the request, this is simply a query.
   if (!msg.exists(h_Contacts))
   {
      if (async)
      {
         mAsyncState = asyncStateQueryOnly;
      }
      handler->onQuery(getHandle(), msg);
      //!WARN! Must not access this object beyond this point. The client my call reject() or accept(), deleting this object.  Also, watch out for local objects that are still in scope and access this object on destruction.
      return;
   }

   ParserContainer<NameAddr> contactList(msg.header(h_Contacts));
   ParserContainer<NameAddr>::iterator i(contactList.begin());
   ParserContainer<NameAddr>::iterator iEnd(contactList.end());
   UInt64 now=Timer::getTimeSecs();
   UInt32 expires=0;

   for (; i != iEnd; ++i)
   {
      if (!i->isWellFormed())
      {
         SharedPtr<SipMessage> failure(new SipMessage);
         mDum.makeResponse(*failure, msg, 400, "Malformed Contact");
         mDum.send(failure);
         if (!async)
         {
            database->unlockRecord(mAor);
         }
         delete(this);
         return;
      }

      expires = globalExpires;
      handler->getContactExpires(*i, mDum.getMasterProfile(), expires, returnCode);

      // Check for "Contact: *" style deregistration
      if (i->isAllContacts())
      {
         if (contactList.size() > 1 || expires != 0)
         {
            SharedPtr<SipMessage> failure(new SipMessage);
            mDum.makeResponse(*failure, msg, 400, "Invalid use of 'Contact: *'");
            mDum.send(failure);
            if (!async)
            {
               database->unlockRecord(mAor);
            }
            delete(this);
            return;
         }

         if (!async)
         {
            database->removeAor(mAor);
         }
         else
         {
            mAsyncLocalStore->removeAllContacts();
            mAsyncState = asyncStateWaitingForAcceptReject;
         }

         handler->onRemoveAll(getHandle(), msg);
         //!WARN! Must not access this object beyond this point. The client my call reject() or accept(), deleting this object.  Also, watch out for local objects that are still in scope and access this object on destruction.
         return;
      }

      ContactInstanceRecord rec;
      rec.mContact=*i;
      rec.mRegExpires=(UInt64)expires+now;

      if(i->exists(p_Instance))
      {
         rec.mInstance=i->param(p_Instance);
      }

      if(!msg.empty(h_Paths))
      {
         rec.mSipPath=msg.header(h_Paths);
      }

      rec.mLastUpdated=now;
      rec.mReceivedFrom=msg.getSource();
      rec.mPublicAddress=Helper::getClientPublicAddress(msg);

      bool hasFlow = tryFlow(rec,msg);
      
      if(!testFlowRequirements(rec, msg, hasFlow))
      {
         // We have rejected the request. Bail.
         if (!async)
         {
            database->unlockRecord(mAor);
         }
         delete(this);
         return;
      }

      // Add ContactInstanceRecord to List
      mRequestContacts.push_back(rec);

      // Check to see if this is a removal.
      if (expires == 0)
      {
         if (operation == REFRESH)
         {
            operation = REMOVE;
         }

         if (!async)
         {
            database->removeContact(mAor, rec);
         }
         else
         {
            mAsyncLocalStore->removeContact(rec);
         }
      }
      else // Otherwise, it's an addition or refresh.
      {
         RegistrationPersistenceManager::update_status_t status;
         InfoLog(<< "Adding " << mAor << " -> " << *i);
         DebugLog(<< "Contact has tuple " << rec.mReceivedFrom << " and detected public address " << rec.mPublicAddress);

         if (!async)
         {
            status = database->updateContact(mAor, rec);
         }
         else
         {
            status =  mAsyncLocalStore->updateContact(rec);
         }

         if (status == RegistrationPersistenceManager::CONTACT_CREATED)
         {
            operation = ADD;
         }
      }
   }

   // The way this works is:
   //
   //  - If no additions or removals are performed, this is a refresh
   //
   //  - If at least one contact is removed and none are added, this
   //    is a removal.
   //
   //  - If at least one contact is added, this is an addition, *even*
   //    *if* a contact was also removed.

   //for async processing, need to wait for accept()/reject().  If accepted, the modifications made here will be
   //sent to the user via asyncUpdateContacts().  The user then returns a final contact list, which is then processed
   //and sent back in the 200.
   if (async)
   {
      mAsyncState = asyncStateWaitingForAcceptReject;
   }

   switch (operation)
   {
      case REFRESH:
         handler->onRefresh(getHandle(), msg);
         //!WARN! Must not access this object beyond this point. The client my call reject() or accept(), deleting this object.  Also, watch out for local objects that are still in scope and access this object on destruction.
         return;

      case REMOVE:
         handler->onRemove(getHandle(), msg);
         //!WARN! Must not access this object beyond this point. The client my call reject() or accept(), deleting this object.  Also, watch out for local objects that are still in scope and access this object on destruction.
         return;

      case ADD:
         handler->onAdd(getHandle(), msg);
         //!WARN! Must not access this object beyond this point. The client my call reject() or accept(), deleting this object.  Also, watch out for local objects that are still in scope and access this object on destruction.
         return;

      default:
         resip_assert(0);
   }
}

void
ServerRegistration::dispatch(const DumTimeout& msg)
{
}

EncodeStream&
ServerRegistration::dump(EncodeStream& strm) const
{
   strm << "ServerRegistration " << mAor;
   return strm;
}

bool 
ServerRegistration::tryFlow(ContactInstanceRecord& rec,
                            const resip::SipMessage& msg)
{
   // .bwc. ie. Can we assure that the connection the client is using on the
   // first hop can be re-used later?
   try
   {
      // Outbound logic
      if(InteropHelper::getOutboundSupported())
      {
         resip::NameAddr& contact(rec.mContact);
         if(contact.exists(p_Instance) && contact.exists(p_regid))
         {
            if(!msg.empty(h_Paths) && (msg.header(h_Paths).back().uri().exists(p_ob) ||
                                       InteropHelper::getAssumeFirstHopSupportsOutboundEnabled()))
            {
               rec.mRegId=contact.param(p_regid);
               // Edge-proxy is directly connected to the client, and ready to 
               // send traffic down the "connection" (TCP connection, or NAT 
               // pinhole, or what-have-you).
               mDidOutbound=true;
               return true;
            }
            else if(msg.header(h_Vias).size() == 1)
            {
               rec.mRegId=contact.param(p_regid);
               // We are directly connected to the client.
               // .bwc. In the outbound case, we should fail if the connection 
               // is gone. No recovery should be attempted by the server.
               rec.mUseFlowRouting = true;
               rec.mReceivedFrom.onlyUseExistingConnection=true;
               mDidOutbound=true;
               return true;
            }
         }
      }

      // Record-Route flow token hack, or client NAT detect hack; use with caution
      if(msg.header(h_Vias).size() == 1)  // client is directly connected to this server
      {
         if(InteropHelper::getRRTokenHackEnabled() || 
            flowTokenNeededForTls(rec) || 
            flowTokenNeededForSigcomp(rec) ||
            (InteropHelper::getClientNATDetectionMode() != InteropHelper::ClientNATDetectionDisabled &&
             Helper::isClientBehindNAT(msg, InteropHelper::getClientNATDetectionMode() == InteropHelper::ClientNATDetectionPrivateToPublicOnly)))
         {
               rec.mUseFlowRouting = true;
               rec.mReceivedFrom.onlyUseExistingConnection=false;
               return true;
         }
      }
   }
   catch(resip::ParseBuffer::Exception&)
   {}
   return false;
}

bool 
ServerRegistration::testFlowRequirements(ContactInstanceRecord &rec,
                                          const resip::SipMessage& msg,
                                          bool hasFlow) const
{
   const resip::NameAddr& contact(rec.mContact);

   if(!msg.empty(h_Supporteds) &&
      msg.header(h_Supporteds).find(Token(Symbols::Outbound)) &&
      contact.exists(p_Instance) && 
      contact.exists(p_regid))
   {
      // Client has explicitly requested Outbound processing, which requires us 
      // to have a flow.
      if(!hasFlow)
      {
         SharedPtr<SipMessage> failure(new SipMessage);
         mDum.makeResponse(*failure, msg, 439);
         mDum.send(failure);
         return false;
      }
   }

   if(!hasFlow && flowTokenNeededForTls(rec))
   {
      SharedPtr<SipMessage> failure(new SipMessage);
      mDum.makeResponse(*failure, msg, 400, "Trying to use TLS with an IP-address in your Contact header won't work if you don't have a flow. Consider implementing outbound, or putting an FQDN in your contact header.");
      mDum.send(failure);
      return false;
   }

   if(!hasFlow && flowTokenNeededForSigcomp(rec))
   {
      SharedPtr<SipMessage> failure(new SipMessage);
      mDum.makeResponse(*failure, msg, 400, "Trying to use sigcomp on a connection-oriented protocol won't work if you don't have a flow. Consider implementing outbound, or using UDP/DTLS for this case.");
      mDum.send(failure);
      return false;
   }

   return true;
}

bool 
ServerRegistration::flowTokenNeededForTls(const ContactInstanceRecord &rec) const
{
   const resip::NameAddr& contact(rec.mContact);
   if(DnsUtil::isIpAddress(contact.uri().host()))
   {
      // IP address in host-part.
      if(contact.uri().scheme()=="sips")
      {
         // sips: and IP-address in contact. This will probably not work anyway.
         return true;
      }

      if(contact.uri().exists(p_transport))
      {
         TransportType type = Tuple::toTransport(contact.uri().param(p_transport));
         if(isSecure(type))
         {
            // secure transport and IP-address. Almost certainly won't work, but
            // we'll try anyway.
            return true;
         }
      }
   }
   return false;
}

bool 
ServerRegistration::flowTokenNeededForSigcomp(const ContactInstanceRecord &rec) const
{
   const resip::NameAddr& contact(rec.mContact);

   if(contact.uri().exists(p_sigcompId))
   {
      if(contact.uri().exists(p_transport))
      {
         TransportType type = Tuple::toTransport(contact.uri().param(p_transport));
         if(type == TLS || type == TCP)
         {
            // Client is using sigcomp on the first hop using a connection-
            // oriented transport. For this to work, that connection has to be
            // reused for all traffic.
            return true;
         }
      }
      else
      {
         // ?bwc? Client is using sigcomp, but we're not sure whether this is
         // over a connection-oriented transport or not.
         DebugLog(<< "Client is using sigcomp, but we're not sure whether this "
                     "is over a connection-oriented transport or not, because "
                     "the contact doesn't have a transport param in it. It is "
                     "possible this will work though, so we'll let it proceed."
                     );
      }
   }
   return false;
}


void
ServerRegistration::asyncProcessFinalOkMsg(SipMessage &msg, ContactPtrList &contacts)
{
   if (contacts.size() > 0)
   {
      ContactPtrList::iterator it(contacts.begin());
      ContactPtrList::iterator itEnd(contacts.end());

      std::auto_ptr<ContactPtrList> expired;

      UInt64 now=Timer::getTimeSecs();

      for (;it != itEnd;++it)
      {
         resip::SharedPtr<ContactInstanceRecord> rec(*it);

         if (!rec)
         {
            resip_assert(0);
            continue;
         }

         if (rec->mRegExpires <= now)
         {
            if (!expired.get())
            {
               expired = std::auto_ptr<ContactPtrList>(new ContactPtrList());
            }
            expired->push_back(rec);
            continue;
         }

         rec->mContact.param(p_expires) = UInt32(rec->mRegExpires - now);
         msg.header(h_Contacts).push_back(rec->mContact);
      }

      if (expired.get() && expired->size() > 0)
      {
         mDum.mServerRegistrationHandler->asyncRemoveExpired(getHandle(),mAor,expired);
         //!WARN! Must not access this object beyond this point. The client my call reject() or accept(), deleting this object.  Also, watch out for local objects that are still in scope and access this object on destruction.
         return;
      }
   }
}

void
ServerRegistration::processFinalOkMsg(SipMessage &msg, ContactList &contacts)
{
   //build the 200Ok and remove any expired entries.
   //the non-asynchronous behavior is to call the database directly, the async behavior is to build a
   //list of all expired entries and send it to the handler.
   if (contacts.size() > 0)
   {
      ContactList::iterator it(contacts.begin());
      ContactList::iterator itEnd(contacts.end());

      RegistrationPersistenceManager *database = mDum.mRegistrationPersistenceManager;
      UInt64 now=Timer::getTimeSecs();

      for (;it != itEnd;++it)
      {
         if (it->mRegExpires <= now)
         {
            database->removeContact(mAor,*it);
            continue;
         }
         it->mContact.param(p_expires) = UInt32(it->mRegExpires - now);
         msg.header(h_Contacts).push_back(it->mContact);
      }
   }
}

bool
ServerRegistration::asyncProvideContacts(std::auto_ptr<resip::ContactPtrList> contacts)
{
   switch (mAsyncState)
   {
      case asyncStateWaitingForInitialContactList:
      {
         resip_assert(mAsyncLocalStore.get() == 0);
         mAsyncLocalStore = resip::SharedPtr<AsyncLocalStore>(new AsyncLocalStore(contacts));
         mAsyncState = asyncStateProcessingRegistration;
         processRegistration(mRequest);
         break;
      }
      case asyncStateWaitingForAcceptReject:
      {
         resip_assert(0); //need to call accept() or reject(), wait for asyncUpdateContacts(), then call this function.
         return false;
      }
      case asyncStateAcceptedWaitingForFinalContactList:
      {
         mAsyncState = asyncStateProvidedFinalContacts;
         asyncProcessFinalContacts(contacts);
         break;
      }
      default:
      {
         resip_assert(0);
         return false;
      }
   }

   return true;
}

void
ServerRegistration::asyncProcessFinalContacts(std::auto_ptr<resip::ContactPtrList> contacts)
{
   if (contacts.get())
   {
      if (!mAsyncOkMsg.get())
      {
         resip_assert(0);
      }
      else
      {
         asyncProcessFinalOkMsg(*mAsyncOkMsg,*contacts);
      }
   }

   mAsyncState = asyncStateNone;
   mDum.send(mAsyncOkMsg);
   mAsyncOkMsg.reset();
   delete(this);
}

void
ServerRegistration::AsyncLocalStore::create(std::auto_ptr<ContactPtrList> originalContacts)
{
   mModifiedContacts = originalContacts;
   mLog = std::auto_ptr<ContactRecordTransactionLog>(new ContactRecordTransactionLog());
}

void
ServerRegistration::AsyncLocalStore::destroy(void)
{
   mModifiedContacts.reset();
   mLog.reset();
}

RegistrationPersistenceManager::update_status_t
ServerRegistration::AsyncLocalStore::updateContact(const ContactInstanceRecord &rec)
{
   if (!mModifiedContacts.get() || !mLog.get())
   {
      resip_assert(0);
      return RegistrationPersistenceManager::CONTACT_UPDATED;
   }

   ContactPtrList::iterator it(mModifiedContacts->begin());
   ContactPtrList::iterator itEnd(mModifiedContacts->end());

   resip::SharedPtr<ContactRecordTransaction> logEntry;

   // See if the contact is already present. We use URI matching rules here.
   for (; it != itEnd; ++it)
   {
      if ((*it) && **it == rec)
      {
         **it = rec;

         logEntry = resip::SharedPtr<ContactRecordTransaction>(new ContactRecordTransaction(ContactRecordTransaction::update,*it));
         mLog->push_back(logEntry);

         return RegistrationPersistenceManager::CONTACT_UPDATED;
      }
   }

   // This is a new contact, so we add it to the list.
   resip::SharedPtr<ContactInstanceRecord> newRec(new ContactInstanceRecord(rec));

   logEntry = resip::SharedPtr<ContactRecordTransaction>(new ContactRecordTransaction(ContactRecordTransaction::create,newRec));
   mLog->push_back(logEntry);

   mModifiedContacts->push_back(newRec);

   return RegistrationPersistenceManager::CONTACT_CREATED;
}

void
ServerRegistration::AsyncLocalStore::removeContact(const ContactInstanceRecord &rec)
{
   if (!mModifiedContacts.get() || !mLog.get())
   {
      resip_assert(0);
      return;
   }

   ContactPtrList::iterator it(mModifiedContacts->begin());
   ContactPtrList::iterator itEnd(mModifiedContacts->end());

   // See if the contact is present. We use URI matching rules here.
   for (; it != itEnd; ++it)
   {
      if ((*it) && **it == rec)
      {
         resip::SharedPtr<ContactRecordTransaction>
         logEntry(resip::SharedPtr<ContactRecordTransaction>
                  (new ContactRecordTransaction(ContactRecordTransaction::remove,*it)));

         mLog->push_back(logEntry);

         mModifiedContacts->erase(it);
         return;
      }
   }
}

void
ServerRegistration::AsyncLocalStore::removeAllContacts(void)
{
   if (!mModifiedContacts.get() || !mLog.get())
   {
      return;
   }

   resip::SharedPtr<ContactInstanceRecord> recNull;

   resip::SharedPtr<ContactRecordTransaction>
   logEntry(resip::SharedPtr<ContactRecordTransaction>(new ContactRecordTransaction(ContactRecordTransaction::removeAll,recNull)));

   mLog->push_back(logEntry);

   mModifiedContacts->clear();
}


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
