#include "resiprocate/SipMessage.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/ServerRegistration.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/RegistrationHandler.hxx"
#include "resiprocate/dum/RegistrationPersistenceManager.hxx"
#include "resiprocate/dum/InternalServerRegistrationMessage.hxx"
#include "resiprocate/os/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

ServerRegistrationHandle 
ServerRegistration::getHandle()
{
   return ServerRegistrationHandle(mDum, getBaseHandle().getId());
}

ServerRegistration::ServerRegistration(DialogUsageManager& dum,  DialogSet& dialogSet, const SipMessage& request)
   : NonDialogUsage(dum, dialogSet),
     mRequest(request)
{}

ServerRegistration::~ServerRegistration()
{
   mDialogSet.mServerRegistration = 0;
}

void 
ServerRegistration::acceptAsync(const SipMessage& ok)
{
   InfoLog(<< "Accept server registration request Async");
   mDum.post(new InternalServerRegistrationMessage_Accept(getHandle(), ok));
}

void 
ServerRegistration::acceptAsync(int statusCode)
{
   InfoLog(<< "Accept server registration request Async: " << statusCode);
   mDum.post(new InternalServerRegistrationMessage_Accept(getHandle(), statusCode));
}

void 
ServerRegistration::rejectAsync(int statusCode)
{
   InfoLog(<< "Reject server registration request Async");
   mDum.post(new InternalServerRegistrationMessage_Reject(getHandle(), statusCode));
}

void 
ServerRegistration::endAsync()
{
   InfoLog(<< "End server registration Async");
   mDum.post(new InternalServerRegistrationMessage_End(getHandle()));
}

void 
ServerRegistration::dispatchAsync(const SipMessage& msg)
{
   InfoLog(<< "Dispatch server registration msg Async");
   mDum.post(new InternalServerRegistrationMessage_DispatchSipMsg(getHandle(), msg));
}

void 
ServerRegistration::dispatchAsync(const DumTimeout& timer)
{
   InfoLog(<< "Dispatch server registration msg timer Async: " << timer);
   mDum.post(new InternalServerRegistrationMessage_DispatchTimeoutMsg(getHandle(), timer));
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
  RegistrationPersistenceManager::ContactPairList contacts;
  RegistrationPersistenceManager::ContactPairList::iterator i;
  contacts = database->getContacts(mAor);
  database->unlockRecord(mAor);

  time_t now;
  time(&now);

  NameAddr contact;
  for (i = contacts.begin(); i != contacts.end(); i++)
  {
    if (i->second - now <= 0)
    {
      continue;
    }
    contact.uri() = i->first;
    contact.param(p_expires) = i->second - now;
    ok.header(h_Contacts).push_back(contact);
  }

  mDum.send(ok);
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

  SipMessage failure;
  mDum.makeResponse(failure, mRequest, statusCode);
  failure.remove(h_Contacts);
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
       DebugLog( << "No handler or DB - sending 405" );
       
       SipMessage failure;
       mDum.makeResponse(failure, msg, 405);
       mDum.send(failure);
       delete(this);
       return;
    }

    mAor = msg.header(h_To).uri();

    database->lockRecord(mAor);

    int globalExpires = 0;

    if (msg.exists(h_Expires))
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
    int expires;
    time_t now;
    time(&now);

    for(i = contactList.begin(); i != contactList.end(); i++)
    {
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
          SipMessage failure;
          mDum.makeResponse(failure, msg, 400, "Invalid use of 'Contact: *'");
          mDum.send(failure);
          database->unlockRecord(mAor);
          delete(this);
          return;
        }

        database->removeAor(mAor);
        handler->onRemoveAll(getHandle(), msg);
        return;
      }

      // Check to see if this is a removal.
      if (expires == 0)
      {
        if (operation == REFRESH)
        {
          operation = REMOVE;
        }
        database->removeContact(mAor, i->uri());
      }
      // Otherwise, it's an addition or refresh.
      else
      {
        RegistrationPersistenceManager::update_status_t status;
        status = database->updateContact(mAor, i->uri(), now + expires);
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
