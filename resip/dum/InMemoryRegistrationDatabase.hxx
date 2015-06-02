#if !defined(RESIP_INMEMORYREGISTRATIONDATABASE_HXX)
#define RESIP_INMEMORYREGISTRATIONDATABASE_HXX

#include <map>
#include <set>

#include "resip/dum/RegistrationPersistenceManager.hxx"
#include "rutil/Mutex.hxx"
#include "rutil/Condition.hxx"
#include "rutil/Lock.hxx"

namespace resip
{

/**
  Trivial implementation of a persistence manager. This class keeps
  all registrations in memory, and has no schemes for disk storage
  or replication of any kind. It's good for testing, but probably
  inappropriate for any commercially deployable products.
*/
class InMemoryRegistrationDatabase : public RegistrationPersistenceManager
{
   public:

      /**
       * @param checkExpiry if set, then the methods aorIsRegistered() and
       *                    getContacts() will check that contacts are
       *                    not expired before returning an answer.
       */
      InMemoryRegistrationDatabase(bool checkExpiry = false);
      virtual ~InMemoryRegistrationDatabase();
      
      virtual void addAor(const Uri& aor, const ContactList& contacts);
      virtual void removeAor(const Uri& aor);
      virtual bool aorIsRegistered(const Uri& aor);
      
      virtual void lockRecord(const Uri& aor);
      virtual void unlockRecord(const Uri& aor);
      
      virtual update_status_t updateContact(const resip::Uri& aor,
                                             const ContactInstanceRecord& rec);
      virtual void removeContact(const Uri& aor, 
                                 const ContactInstanceRecord& rec);
      
      virtual void getContacts(const Uri& aor, ContactList& container);
   
      /// return all the AOR in the DB 
      virtual void getAors(UriList& container);
      
   protected:
      typedef std::map<Uri,ContactList *> database_map_t;
      database_map_t mDatabase;
      Mutex mDatabaseMutex;
      
      std::set<Uri> mLockedRecords;
      Mutex mLockedRecordsMutex;
      Condition mRecordUnlocked;

      bool mCheckExpiry;

      /**
       * Find aor in mDatabase
       * Before returning the iterator pointing to aor,
       * delete all expired contacts
       */
      database_map_t::iterator findNotExpired(const Uri& aor);
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
