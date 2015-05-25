#if !defined(RESIP_INMEMORYSYNCREGDB_HXX)
#define RESIP_INMEMORYSYNCREGDB_HXX

#include <map>
#include <set>
#include <list>

#include "resip/dum/RegistrationPersistenceManager.hxx"
#include "rutil/Mutex.hxx"
#include "rutil/Condition.hxx"
#include "rutil/Lock.hxx"

namespace resip
{

class InMemorySyncRegDbHandler
{
public:
   typedef enum
   {
      SyncServer,
      AllChanges
   } HandlerMode;
   InMemorySyncRegDbHandler(HandlerMode mode = SyncServer) : mMode(mode) {}
   virtual ~InMemorySyncRegDbHandler(){}
   HandlerMode getMode() { return mMode; }
   virtual void onAorModified(const resip::Uri& aor, const ContactList& contacts) = 0;
   virtual void onInitialSyncAor(unsigned int connectionId, const resip::Uri& aor, const ContactList& contacts) {}
protected:
   HandlerMode mMode;
};


/**
  Implementation of a persistence manager. This class keeps
  all registrations in memory, and is used for remote replication.

  Removed contact bindings are kept in memory with a RegExpires time
  of 0.  This is required in order to properly syncronize removed 
  contact bindings with a peer instance.  Contacts are not deleted 
  from memory until they have been removed for "removeLingerSecs".
  The removeLingerSecs parameter is passed into the contructor.
  If removeLingerSecs is set to 0, then contacts are removed from
  memory immediately and this class behaves very similar to the 
  InMemoryRegistrationDatabase class.

  The InMemorySyncRegDbHandler can be used by an external mechanism to 
  transport registration bindings to a remote peer for replication.
  See the RegSyncClient and RegSyncServer implementations in the repro
  project.
*/
class InMemorySyncRegDb : public RegistrationPersistenceManager
{
   public:

      InMemorySyncRegDb(unsigned int removeLingerSecs = 0);
      virtual ~InMemorySyncRegDb();
      
      virtual void addHandler(InMemorySyncRegDbHandler* handler);
      virtual void removeHandler(InMemorySyncRegDbHandler* handler);

      virtual void initialSync(unsigned int connectionId);

      virtual void addAor(const Uri& aor, const ContactList& contacts);
      virtual void removeAor(const Uri& aor);
      virtual bool aorIsRegistered(const Uri& aor);
      virtual bool aorIsRegistered(const Uri& aor, UInt64* maxExpires);
      
      virtual void lockRecord(const Uri& aor);
      virtual void unlockRecord(const Uri& aor);
      
      virtual update_status_t updateContact(const resip::Uri& aor,
                                             const ContactInstanceRecord& rec);
      virtual void removeContact(const Uri& aor, 
                                 const ContactInstanceRecord& rec);
      
      virtual void getContacts(const Uri& aor, ContactList& container);
      virtual void getContactsFull(const Uri& aor, ContactList& container);
   
      /// return all the AOR in the DB 
      virtual void getAors(UriList& container);
      
   protected:
      typedef std::map<Uri,ContactList *> database_map_t;
      database_map_t mDatabase;
      Mutex mDatabaseMutex;

      std::set<Uri> mLockedRecords;
      Mutex mLockedRecordsMutex;
      Condition mRecordUnlocked;

      void invokeOnAorModified(bool sync, const resip::Uri& aor, const ContactList& contacts);
      void invokeOnInitialSyncAor(unsigned int connectionId, const resip::Uri& aor, const ContactList& contacts);
      unsigned int mRemoveLingerSecs;
      typedef std::list<InMemorySyncRegDbHandler*> HandlerList;
      HandlerList mHandlers;  // use list over set to preserve add order
      Mutex mHandlerMutex;
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
