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
     mDidOutbound(false)
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
  
  // Add all registered contacts to the message.
  RegistrationPersistenceManager *database = mDum.mRegistrationPersistenceManager;
  ContactList contacts;
  ContactList::iterator i;
  contacts = database->getContacts(mAor);
  database->unlockRecord(mAor);

  UInt64 now=Timer::getTimeMs();

  NameAddr contact;
  for (i = contacts.begin(); i != contacts.end(); i++)
  {
    if (i->mRegExpires <= now)
    {
      database->removeContact(mAor,*i);
      continue;
    }
    contact = i->mContact;
   // .bwc. p_expires is in seconds.
    contact.param(p_expires) = UInt32( ((i->mRegExpires - now)/1000) );
    ok.header(h_Contacts).push_back(contact);
  }
  
  if(mDidOutbound)
  {
     static Token outbound("outbound");
     ok.header(h_Supporteds).push_back(outbound);
  }

  SharedPtr<SipMessage> msg(static_cast<SipMessage*>(ok.clone()));
  mDum.send(msg);
  delete(this);
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
  RegistrationPersistenceManager *database = mDum.mRegistrationPersistenceManager;
  database->removeAor(mAor);
  database->addAor(mAor, mOriginalContacts);
  database->unlockRecord(mAor);

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

    enum {ADD, REMOVE, REFRESH} operation = REFRESH;

    if (!handler || !database)
    {
      // ?bwc? This is a server error; why are we sending a 4xx?
       DebugLog( << "No handler or DB - sending 405" );
       
       SharedPtr<SipMessage> failure(new SipMessage);
       mDum.makeResponse(*failure, msg, 405);
       mDum.send(failure);
       delete(this);
       return;
    }

    mAor = msg.header(h_To).uri().getAorAsUri();

   // Checks to see whether this scheme is valid, and supported.
   if( !( (mAor.scheme()=="sip" || mAor.scheme()=="sips") 
            && mDum.getMasterProfile()->isSchemeSupported(mAor.scheme()) ) )
   {
       DebugLog( << "Bad scheme in Aor" );
       
       SharedPtr<SipMessage> failure(new SipMessage);
       mDum.makeResponse(*failure, msg, 400);
       failure->header(h_StatusLine).reason() = "Bad/unsupported scheme in To: " + mAor.scheme();
       mDum.send(failure);
       delete(this);
       return;
   }
   

    database->lockRecord(mAor);

    UInt32 globalExpires = 0;
    UInt32 expires=0;
    
    if (msg.exists(h_Expires) && msg.header(h_Expires).isWellFormed())
    {
      globalExpires = msg.header(h_Expires).value();
    }
    else
    {
       globalExpires = 3600;
    }

    mOriginalContacts = database->getContacts(mAor);

    // If no conacts are present in the request, this is simply a query.
    if (!msg.exists(h_Contacts))
    {
      handler->onQuery(getHandle(), msg);
      return;
    }

    ParserContainer<NameAddr> contactList(msg.header(h_Contacts));
    ParserContainer<NameAddr>::iterator i;
    UInt64 now=Timer::getTimeMs();

    for(i = contactList.begin(); i != contactList.end(); i++)
    {
      if(!i->isWellFormed())
      {
         SharedPtr<SipMessage> failure(new SipMessage);
         mDum.makeResponse(*failure, msg, 400, "Malformed Contact");
         mDum.send(failure);
         database->unlockRecord(mAor);
         delete(this);
         return;
      }

      if (i->exists(p_expires))
      {
         expires = i->param(p_expires);
      }
      else
      {
        expires = globalExpires;
      }

      // Check for "Contact: *" style deregistration
      if (i->isAllContacts())
      {
        if (contactList.size() > 1 || expires != 0)
        {
           SharedPtr<SipMessage> failure(new SipMessage);
           mDum.makeResponse(*failure, msg, 400, "Invalid use of 'Contact: *'");
           mDum.send(failure);
           database->unlockRecord(mAor);
           delete(this);
           return;
        }

        database->removeAor(mAor);
        handler->onRemoveAll(getHandle(), msg);
        return;
      }

      ContactInstanceRecord rec;
      rec.mContact=*i;
      rec.mRegExpires=((UInt64)expires)*1000+now;

      if(i->exists(p_Instance))
      {
         rec.mInstance=i->param(p_Instance);
      }

      if(!msg.empty(h_Paths))
      {
         rec.mSipPath=msg.header(h_Paths);
      }

      rec.mLastUpdated=now;

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
            if(!i->exists(p_Instance) || !i->exists(p_regid))
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
         catch(resip::ParseBuffer::Exception& e)
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
         rec.mRegId=i->param(p_regid);
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
      
      // Check to see if this is a removal.
      if (expires == 0)
      {
        if (operation == REFRESH)
        {
          operation = REMOVE;
        }
        database->removeContact(mAor, rec);
      }
      // Otherwise, it's an addition or refresh.
      else
      {
        RegistrationPersistenceManager::update_status_t status;
        InfoLog (<< "Adding " << mAor << " -> " << *i  );
        DebugLog(<< "Contact has tuple " << rec.mReceivedFrom);
         status = database->updateContact(mAor, rec);
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
    }
}

void
ServerRegistration::dispatch(const DumTimeout& msg)
{
}

std::ostream& 
ServerRegistration::dump(std::ostream& strm) const
{
   strm << "ServerRegistration " << mAor;
   return strm;
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
