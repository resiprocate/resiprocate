#include "resip/dum/InMemorySyncRegDb.hxx"
#include "rutil/Timer.hxx"
#include "rutil/Logger.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

class RemoveIfRequired
{
protected:
    UInt64 mNow;
    unsigned int mRemoveLingerSecs;
public:
    RemoveIfRequired(UInt64& now, unsigned int removeLingerSecs) :
       mNow(now),
       mRemoveLingerSecs(removeLingerSecs) {}
    bool operator () (const ContactInstanceRecord& rec)
    {
       return mustRemove(rec);
    }
    bool mustRemove(const ContactInstanceRecord& rec)
    {
       if((rec.mRegExpires <= mNow) && ((mNow - rec.mLastUpdated) > mRemoveLingerSecs)) 
       {
          DebugLog(<< "ContactInstanceRecord removed after linger: " << rec.mContact);
          return true;
       }
      return false;
    }
};

/* Solaris with libCstd seems to choke on the use of an
   object (such as RemoveIfRequired) as a predicate for remove_if.
   Therefore, this wrapper function implements a workaround,
   iterating the list explicitly and using erase(). */
void
contactsRemoveIfRequired(ContactList& contacts, UInt64& now,
   unsigned int removeLingerSecs)
{
   RemoveIfRequired rei(now, removeLingerSecs);
#ifdef __SUNPRO_CC
   for(ContactList::iterator i = contacts.begin(); i != contacts.end(); )
   {
      if(rei.mustRemove(*i))
         i = contacts.erase(i);
      else
         ++i;
   }
#else
   contacts.remove_if(rei);
#endif
}

InMemorySyncRegDb::InMemorySyncRegDb(unsigned int removeLingerSecs) : 
   mRemoveLingerSecs(removeLingerSecs),
   mHandler(0)
{
}

InMemorySyncRegDb::~InMemorySyncRegDb()
{
   for( database_map_t::const_iterator it = mDatabase.begin();
        it != mDatabase.end(); it++)
   {
      delete it->second;
   }
   mDatabase.clear();
}

void 
InMemorySyncRegDb::initialSync(unsigned int connectionId)
{
   Lock g(mDatabaseMutex);
   UInt64 now = Timer::getTimeSecs();
   for(database_map_t::iterator it = mDatabase.begin(); it != mDatabase.end(); it++)
   {
      if(it->second)
      {
         ContactList& contacts = *(it->second);
         if(mRemoveLingerSecs > 0) 
         {
            contactsRemoveIfRequired(contacts, now, mRemoveLingerSecs);
         }
         if(mHandler) mHandler->onInitialSyncAor(connectionId, it->first, contacts);        
      }
   }
}

void 
InMemorySyncRegDb::addAor(const Uri& aor,
                          const ContactList& contacts)
{
   Lock g(mDatabaseMutex);
   database_map_t::iterator it = mDatabase.find(aor);
   if(it != mDatabase.end())
   {
       if(it->second)
       {
           *(it->second) = contacts;
       }
       else
       {
           it->second = new ContactList(contacts);
       }
   }
   else
   {
       mDatabase[aor] = new ContactList(contacts);
   }
   if(mHandler) mHandler->onAorModified(aor, contacts);
}

void 
InMemorySyncRegDb::removeAor(const Uri& aor)
{
  database_map_t::iterator i;

  Lock g(mDatabaseMutex);
  i = mDatabase.find(aor);
  //DebugLog (<< "Removing registration bindings " << aor);
  if (i != mDatabase.end())
  {
     if (i->second)
     {
        if(mRemoveLingerSecs > 0)
        {
           ContactList& contacts = *(i->second);
           UInt64 now = Timer::getTimeSecs();
           for(ContactList::iterator it = contacts.begin(); it != contacts.end(); it++)
           {
              // Don't delete record - set expires to 0
              it->mRegExpires = 0;
              it->mLastUpdated = now;
           }
           if(mHandler) mHandler->onAorModified(aor, contacts);
        }
        else
        {
           delete i->second;
           // Setting this to 0 causes it to be removed when we unlock the AOR.
           i->second = 0;
           ContactList emptyList;
           if(mHandler) mHandler->onAorModified(aor, emptyList);
        }
     }
  }
}

void
InMemorySyncRegDb::getAors(InMemorySyncRegDb::UriList& container)
{
   container.clear();
   Lock g(mDatabaseMutex);
   for( database_map_t::const_iterator it = mDatabase.begin();
        it != mDatabase.end(); it++)
   {
      container.push_back(it->first);
   }
}

bool 
InMemorySyncRegDb::aorIsRegistered(const Uri& aor)
{
   Lock g(mDatabaseMutex);
   database_map_t::iterator i = mDatabase.find(aor);
   if (i != mDatabase.end() && i->second == 0)
   {
      if(mRemoveLingerSecs > 0)
      {
         ContactList& contacts = *(i->second);
         UInt64 now = Timer::getTimeSecs();
         for(ContactList::iterator it = contacts.begin(); it != contacts.end(); it++)
         {
            if(it->mRegExpires > now)
            {
               return true;
            }
         }
      }
      else
      {
          return true;
      }
   }
   return false;
}

void
InMemorySyncRegDb::lockRecord(const Uri& aor)
{
   Lock g2(mLockedRecordsMutex);

   DebugLog(<< "InMemorySyncRegDb::lockRecord:  aor=" << aor << " threadid=" << ThreadIf::selfId());

   {
      Lock g1(mDatabaseMutex);
      // This forces insertion if the record does not yet exist.
      mDatabase[aor];
   }

   while (mLockedRecords.count(aor))
   {
      mRecordUnlocked.wait(mLockedRecordsMutex);
   }

   mLockedRecords.insert(aor);
}

void
InMemorySyncRegDb::unlockRecord(const Uri& aor)
{
   Lock g2(mLockedRecordsMutex);

   DebugLog(<< "InMemorySyncRegDb::unlockRecord:  aor=" << aor << " threadid=" << ThreadIf::selfId());

   {
      Lock g1(mDatabaseMutex);
      // If the pointer is null, we remove the record from the map.
      database_map_t::iterator i = mDatabase.find(aor);

      // The record must have been inserted when we locked it in the first place
      assert (i != mDatabase.end());

      if (i->second == 0)
      {
         mDatabase.erase(i);
      }
   }

   mLockedRecords.erase(aor);
   mRecordUnlocked.broadcast();
}

RegistrationPersistenceManager::update_status_t 
InMemorySyncRegDb::updateContact(const resip::Uri& aor, 
                                 const ContactInstanceRecord& rec) 
{
   ContactList *contactList = 0;

   {
      Lock g(mDatabaseMutex);

      database_map_t::iterator i;
      i = mDatabase.find(aor);
      if (i == mDatabase.end() || i->second == 0)
      {
         contactList = new ContactList();
         mDatabase[aor] = contactList;
      }
      else
      {
         contactList = i->second;
      }
   }
   
   assert(contactList);

   ContactList::iterator j;

   // See if the contact is already present. We use URI matching rules here.
   for (j = contactList->begin(); j != contactList->end(); j++)
   {
      if (*j == rec)
      {
         update_status_t status = CONTACT_UPDATED;
         if(mRemoveLingerSecs > 0 && j->mRegExpires == 0)
         {
            // If records linger, then check if updating a lingering record, if so
            // modify status to CREATED so that ServerRegistration will properly generate
            // an onAdd callback, instead of onRefresh.
            // When contacts linger, their expires time is set to 0
            status = CONTACT_CREATED;
         }
         *j=rec;
         if(mHandler && !rec.mSyncContact) mHandler->onAorModified(aor, *contactList);
         return status;
      }
   }

   // This is a new contact, so we add it to the list.
   contactList->push_back(rec);
   if(mHandler && !rec.mSyncContact) mHandler->onAorModified(aor, *contactList);
   return CONTACT_CREATED;
}

void 
InMemorySyncRegDb::removeContact(const Uri& aor, 
                                 const ContactInstanceRecord& rec)
{
   ContactList *contactList = 0;

   {
      Lock g(mDatabaseMutex);

      database_map_t::iterator i;
      i = mDatabase.find(aor);
      if (i == mDatabase.end() || i->second == 0)
      {
         return;
      }
      contactList = i->second;
   }

   ContactList::iterator j;

   // See if the contact is present. We use URI matching rules here.
   for (j = contactList->begin(); j != contactList->end(); j++)
   {
      if (*j == rec)
      {
         if(mRemoveLingerSecs > 0)
         {
            j->mRegExpires = 0;
            j->mLastUpdated = Timer::getTimeSecs();
            if(mHandler && !rec.mSyncContact) mHandler->onAorModified(aor, *contactList);
         }
         else
         {
            contactList->erase(j);
            if (contactList->empty())
            {
               removeAor(aor);
            }
            else
            {
               if(mHandler && !rec.mSyncContact) mHandler->onAorModified(aor, *contactList);
            }
         }
         return;
      }
   }
}

void
InMemorySyncRegDb::getContacts(const Uri& aor, ContactList& container)
{
   Lock g(mDatabaseMutex);
   database_map_t::iterator i = mDatabase.find(aor);
   if (i == mDatabase.end() || i->second == 0)
   {
      container.clear();
      return;
   }
   if(mRemoveLingerSecs > 0)
   {
      ContactList& contacts = *(i->second);
      UInt64 now = Timer::getTimeSecs();
      contactsRemoveIfRequired(contacts, now, mRemoveLingerSecs);
      container.clear();
      for(ContactList::iterator it = contacts.begin(); it != contacts.end(); it++)
      {
         if(it->mRegExpires > now)
         {
             container.push_back(*it);
         }
      }
   }
   else
   {
      container = *(i->second);
   }
}

void
InMemorySyncRegDb::getContactsFull(const Uri& aor, ContactList& container)
{
   Lock g(mDatabaseMutex);
   database_map_t::iterator i = mDatabase.find(aor);
   if (i == mDatabase.end() || i->second == 0)
   {
      container.clear();
      return;
   }
   ContactList& contacts = *(i->second);
   if(mRemoveLingerSecs > 0)
   {
      UInt64 now = Timer::getTimeSecs();
      contactsRemoveIfRequired(contacts, now, mRemoveLingerSecs);
   }
   container = contacts;
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
