#include "resip/stack/ExtensionParameter.hxx"
#include "resip/stack/InteropHelper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/ServerRegistration.hxx"
#include "resip/dum/Dialog.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/RegistrationPersistenceManager.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Timer.hxx"
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

void
ServerRegistration::accept(SipMessage& ok)
{
   ok.remove(h_Contacts);

   InfoLog( << "accepted a registration " << mAor );

   if (mDidOutbound)
   {
      static Token outbound("outbound");
      ok.header(h_Supporteds).push_back(outbound);
   }

   if (!mDum.mServerRegistrationHandler->asyncProcessing())
   {
      // Add all registered contacts to the message.
      RegistrationPersistenceManager *database = mDum.mRegistrationPersistenceManager;

      ContactList contacts;

      contacts = database->getContacts(mAor);
      database->unlockRecord(mAor);

      //removes expired entries from the ok msg as well as calls the database to remove expired contacts.
      processFinalOkMsg(ok,contacts);

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
            assert(0);
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
            assert(0);
            return;
         }
         //This register was accepted, but still need to apply the changes made by this register and then
         //receive a final contact list before sending the 200.
         mAsyncState = asyncStateAcceptedWaitingForFinalContactList;

         std::auto_ptr<ContactRecordTransactionLog> log;
         std::auto_ptr<ContactPtrList> modifiedContacts;

         mAsyncLocalStore->releaseLog(log,modifiedContacts);

         mDum.mServerRegistrationHandler->asyncUpdateContacts(getHandle(),mAor,modifiedContacts,log);

         mAsyncLocalStore->destroy(); //drop ownership of resources in the local store.

         mAsyncOkMsg = SharedPtr<SipMessage>(static_cast<SipMessage*>(ok.clone()));
      }
   }
}

void
ServerRegistration::accept(int statusCode)
{
   SipMessage success;
   mDum.makeResponse(success, mRequest, statusCode);
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
   
   assert(msg.isRequest());
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

    mAor = msg.header(h_To).uri().getAorAsUri();

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
      handler->asyncGetContacts(getHandle(),mAor);
      mAsyncState = asyncStateWaitingForInitialContactList;
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
      *mOriginalContacts = database->getContacts(mAor);
   }

   // If no contacts are present in the request, this is simply a query.
    if (!msg.exists(h_Contacts))
    {
      if (async)
      {
         mAsyncState = asyncStateQueryOnly;
      }
      handler->onQuery(getHandle(), msg);
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
      handler->getContactExpires(*i,mDum.getMasterProfile(),expires,returnCode);       

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

      bool supportsOutbound = processOutbound(*i,rec,msg);

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
         DebugLog(<< "Contact has tuple " << rec.mReceivedFrom);

         if (!async)
            status = database->updateContact(mAor, rec);
         else
            status =  mAsyncLocalStore->updateContact(rec);

         if (status == RegistrationPersistenceManager::CONTACT_CREATED)
         {
            operation = ADD;
         }
      }

      // !bwc! If we perform outbound processing for any Contact, we need to
      // set this to true.
      mDidOutbound |= supportsOutbound;
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
         break;

      case REMOVE:
         handler->onRemove(getHandle(), msg);
         break;

      case ADD:
         handler->onAdd(getHandle(), msg);
         break;

      default:
         assert(0);
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
ServerRegistration::processOutbound(resip::NameAddr &naddr, ContactInstanceRecord &rec, const resip::SipMessage &msg)
{
      // .bwc. If, in the end, this is true, it means all necessary conditions
      // for outbound support have been met.
      bool supportsOutbound=InteropHelper::getOutboundSupported();

      // .bwc. We only store flow information if we have a direct flow to the
      // endpoint. We do not create a flow if there is an edge-proxy, because if
      // our connection to the edge proxy fails, the flow from the edge-proxy to
      // the endpoint is still good, so we should not discard the registration.
      bool haveDirectFlow=true;

      if(supportsOutbound)
      {
         try
         {
         if (!naddr.exists(p_Instance) || !naddr.exists(p_regid))
            {
               DebugLog(<<"instance or reg-id missing");
               supportsOutbound=false;
            }

            if(!msg.empty(h_Paths))
            {
               haveDirectFlow=false;
               if(!msg.header(h_Paths).back().exists(p_ob))
               {
                  DebugLog(<<"last Path doesn't have ob");
                  supportsOutbound=false;
               }
            }
            else if(msg.header(h_Vias).size() > 1)
            {
               DebugLog(<<"more than one Via, and no Path");
               supportsOutbound=false;
            }
         }
         catch(resip::ParseBuffer::Exception&)
         {
            supportsOutbound=false;
         }
      }
      else
      {
         DebugLog(<<"outbound support disabled");
      }

      // .bwc. The outbound processing
      if(supportsOutbound)
      {
      rec.mRegId=naddr.param(p_regid);
         if(haveDirectFlow)
         {
            // .bwc. We NEVER record the source if outbound is not being used.
            // There is nothing we can do with this info in the non-outbound
            // case that doesn't flagrantly violate spec (yet).
            // (Example: If we remember this info with the intent of sending
            // incoming stuff to this source directly, we end up being forced
            // to record-route with a flow token to give in-dialog stuff a
            // chance of working. Once we have record-routed, we have set this
            // decision in stone for the rest of the dialog, preventing target
            // refresh requests from working. If record-route was mutable, 
            // maybe it would be okay to do this, but until this is officially
            // allowed, we shouldn't touch it.)
            rec.mReceivedFrom=msg.getSource();
            // .bwc. In the outbound case, we should fail if the connection is
            // gone. No recovery should be attempted by the server.
            rec.mReceivedFrom.onlyUseExistingConnection=true;
            DebugLog(<<"Set rec.mReceivedFrom: " << rec.mReceivedFrom);
         }
      }
      else if(InteropHelper::getRRTokenHackEnabled())
      {
         // .bwc. If we are going to do the broken thing, and record-route with
         // flow-tokens every time, we enable it here. Keep in mind, this will
         // either break target-refreshes (if we have a direct connection to the
         // endpoint), or we have a good chance of inadvertently including a 
         // proxy that didn't record-route in the dialog.
         // !bwc! TODO remove this once mid-dialog connection reuse is handled
         // by most endpoints.
         rec.mReceivedFrom=msg.getSource();
         rec.mReceivedFrom.onlyUseExistingConnection=false;
         DebugLog(<<"Set rec.mReceivedFrom: " << rec.mReceivedFrom);
      }
      
   return supportsOutbound;
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
            assert(0);
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
         assert(mAsyncLocalStore.get() == 0);
         mAsyncLocalStore = resip::SharedPtr<AsyncLocalStore>(new AsyncLocalStore(contacts));
         mAsyncState = asyncStateProcessingRegistration;
         processRegistration(mRequest);
         break;
      }
      case asyncStateWaitingForAcceptReject:
      {
         assert(0); //need to call accept() or reject(), wait for asyncUpdateContacts(), then call this function.
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
         assert(0);
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
         assert(0);
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
   if (originalContacts.get())
   {
      mLog->resize(originalContacts->size());
   }
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
      assert(0);
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
      assert(0);
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
