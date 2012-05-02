
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/Lock.hxx"

#include "repro/StaticRegStore.hxx"

using namespace resip;
using namespace repro;
using namespace std;


#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

StaticRegStore::StaticRegStore(AbstractDb& db):
   mDb(db)
{
   AbstractDb::Key key = mDb.firstStaticRegKey();
   while (!key.empty())
   {
      AbstractDb::StaticRegRecord rec = mDb.getStaticReg(key);

      try
      {
         Uri aor(rec.mAor);
         NameAddr contact(rec.mContact);
         NameAddrs paths;
         Data path;
         ParseBuffer pb(rec.mPath);
         const char* anchor = pb.position();
         while(!pb.eof())
         {
            pb.skipToChar(Symbols::COMMA[0]);
            pb.data(path, anchor);
            paths.push_back(NameAddr(path));
            if(!pb.eof()) pb.skipChar();  // skip over comma
            anchor = pb.position();
         }

         mStaticRegList[make_pair(aor, contact.uri())] = StaticRegRecord(aor, contact, paths);
      }
      catch(resip::ParseBuffer::Exception& e)  
      {
         // This should never happen, since the format should be verified before writing to DB
         ErrLog(<<"Failed to load a static registration due to parse error: " << e);
      }

      key = mDb.nextStaticRegKey();
   }
}


StaticRegStore::~StaticRegStore()
{
}


bool
StaticRegStore::addStaticReg(const resip::Uri& aor,
                             const resip::NameAddr& contact,
                             const resip::NameAddrs& path)
{
   Data aorData(Data::from(aor));
   Data contactData(Data::from(contact));

   // Add new Db Record
   Key addKey = buildKey(aorData, contactData);
   AbstractDb::StaticRegRecord rec;
   rec.mAor = aorData;
   rec.mContact = contactData;
   NameAddrs::const_iterator it = path.begin();
   for(; it != path.end(); it++)
   {
      if(it != path.begin())
      {
         rec.mPath += ",";
      }
      rec.mPath += Data::from(*it);
   }

   InfoLog( << "Add StaticReg: key=" << addKey);

   if(!mDb.addStaticReg(addKey, rec))
   {
      return false;
   }

   Key removeKey;
   {
      WriteLock lock(mMutex);
      std::pair<Uri, Uri> mapKey = make_pair(aor, contact.uri());
      StaticRegRecordMap::iterator it = mStaticRegList.find(mapKey);
      if(it != mStaticRegList.end())
      {
         // Exists already (Uri equalitiy for both aor and Uri portion of contact)- updatedb by removing 
         removeKey = buildKey(Data::from(it->second.mAor), Data::from(it->second.mContact));
         
         // Update Map
         it->second.mAor = aor;
         it->second.mContact = contact;
         it->second.mPath = path;
      }
      else
      {
         mStaticRegList[mapKey] = StaticRegRecord(aor, contact, path);
      }
   }

   // Do DB Work outside of local lock
   if(!removeKey.empty())
   {
      // Erase old DB record
      mDb.eraseStaticReg(removeKey);
   }

   return true;
}


void 
StaticRegStore::eraseStaticReg(const resip::Uri& aor,
                               const resip::NameAddr& contact)
{  
   Key removeKey;
   {
      WriteLock lock(mMutex);
      StaticRegRecordMap::iterator it = mStaticRegList.find(make_pair(aor, contact.uri()));
      if(it != mStaticRegList.end())
      {
         // Exists (Uri equalitiy for both aor and Uri portion of contact)
         removeKey = buildKey(Data::from(it->second.mAor), Data::from(it->second.mContact));

         // Erase from local storage
         mStaticRegList.erase(it);
      }
   }

   if(!removeKey.empty())
   {
      // Erase DB record
      mDb.eraseStaticReg(removeKey);
   }
}


StaticRegStore::Key 
StaticRegStore::buildKey(const resip::Data& aor,
                         const resip::Data& contact) const
{  
   Data pKey = aor+":"+contact;
   return pKey;
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
 */
