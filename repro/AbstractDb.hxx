#if !defined(RESIP_ABSTRACTDB_HXX)
#define RESIP_ABSTRACTDB_HXX 

#include "rutil/Data.hxx"
#include "rutil/Fifo.hxx"
#include "resip/stack/Message.hxx"
#include <vector>

namespace resip
{
  class TransactionUser;
}

namespace repro
{

class AbstractDb
{
   public:
      AbstractDb();
      virtual ~AbstractDb();
      
      class UserRecord
      {
         public:
            resip::Data user;
            resip::Data domain;
            resip::Data realm;
            resip::Data passwordHash;
            resip::Data passwordHashAlt;
            resip::Data name;
            resip::Data email;
            resip::Data forwardAddress;
      };

      class RouteRecord
      {
         public:
            resip::Data mMethod;
            resip::Data mEvent;
            resip::Data mMatchingPattern;
            resip::Data mRewriteExpression;
            short mOrder;
            //bool mLocalUserOnly;
      };

      class AclRecord
      {
         public:
            resip::Data mTlsPeerName;
            resip::Data mAddress;
            short mMask;            
            short mPort;
            short mFamily;
            short mTransport;
      };

      class ConfigRecord
      {
         public:
            resip::Data mDomain;
            short mTlsPort;
      };

      class StaticRegRecord
      {
         public:
            resip::Data mAor;
            resip::Data mContact;
            resip::Data mPath;
      };

      class FilterRecord
      {
         public:
            resip::Data mCondition1Header;
            resip::Data mCondition1Regex;
            resip::Data mCondition2Header;
            resip::Data mCondition2Regex;
            resip::Data mMethod;
            resip::Data mEvent;
            short mAction;  // 0 - Accept, 1 - Reject, 2 - SQL Query
            resip::Data mActionData;
            short mOrder;
      };

      class SiloRecord
      {
         public:
            resip::Data mDestUri;
            resip::Data mSourceUri;
            UInt64 mOriginalSentTime;
            resip::Data mTid;
            resip::Data mMimeType;
            resip::Data mMessageBody;
      };

      typedef resip::Data Key;
      typedef std::vector<RouteRecord> RouteRecordList;
      typedef std::vector<AclRecord> AclRecordList;
      typedef std::vector<ConfigRecord> ConfigRecordList;
      typedef std::vector<StaticRegRecord> StaticRegRecordList;
      typedef std::vector<FilterRecord> FilterRecordList;
      typedef std::vector<SiloRecord> SiloRecordList;

      virtual bool isSane() = 0;

      // functions for User Records 
      virtual bool addUser(const Key& key, const UserRecord& rec);
      virtual void eraseUser(const Key& key);
      virtual UserRecord getUser(const Key& key) const;
      virtual resip::Data getUserAuthInfo(const Key& key) const;
      virtual Key firstUserKey();// return empty if no more
      virtual Key nextUserKey(); // return empty if no more 
         
      // functions for Route Records
      virtual bool addRoute(const Key& key, const RouteRecord& rec);
      virtual void eraseRoute(const Key& key);
      virtual RouteRecord getRoute(const Key& key) const;
      virtual RouteRecordList getAllRoutes();
      virtual Key firstRouteKey();// return empty if no more
      virtual Key nextRouteKey(); // return empty if no more 

      // functions for Acl Records
      virtual bool addAcl(const Key& key, const AclRecord& rec);
      virtual void eraseAcl(const Key& key);
      virtual AclRecordList getAllAcls();
      virtual AclRecord getAcl(const Key& key) const;
      virtual Key firstAclKey();// return empty if no more
      virtual Key nextAclKey(); // return empty if no more 

      // functions for Config Records
      virtual bool addConfig(const Key& key, const ConfigRecord& rec);
      virtual void eraseConfig(const Key& key);
      virtual ConfigRecordList getAllConfigs();
      virtual ConfigRecord getConfig(const Key& key) const;
      virtual Key firstConfigKey();// return empty if no more
      virtual Key nextConfigKey(); // return empty if no more 

      // functions for StaticReg Records
      virtual bool addStaticReg(const Key& key, const StaticRegRecord& rec);
      virtual void eraseStaticReg(const Key& key );
      virtual StaticRegRecordList getAllStaticRegs();
      virtual StaticRegRecord getStaticReg( const Key& key) const;
      virtual Key firstStaticRegKey();// return empty if no more
      virtual Key nextStaticRegKey(); // return empty if no more 

      // functions for Filter Records
      virtual bool addFilter(const Key& key, const FilterRecord& rec);
      virtual void eraseFilter(const Key& key);
      virtual FilterRecord getFilter(const Key& key) const;
      virtual FilterRecordList getAllFilters();
      virtual Key firstFilterKey();// return empty if no more
      virtual Key nextFilterKey(); // return empty if no more 

      // functions for Silo Records
      virtual bool addToSilo(const Key& key, const SiloRecord& rec);
      virtual bool getSiloRecords(const Key& skey, SiloRecordList& recordList); 
      virtual void eraseSiloRecord(const Key& key);
      virtual void cleanupExpiredSiloRecords(UInt64 now, unsigned long expirationTime);

   protected:
      typedef enum 
      {
         UserTable=0,
         RouteTable,
         AclTable,
         ConfigTable,
         StaticRegTable,
         FilterTable,
         SiloTable,
         MaxTable  // This one MUST be last 
      } Table;

      virtual int getSecondaryKey(const Table table, 
                                  const Key& key, 
                                  const resip::Data& data, 
                                  void** secondaryKey, 
                                  unsigned int* secondaryKeyLen);

      // Db manipulation routines
      virtual bool dbWriteRecord(const Table table, 
                                 const resip::Data& key, 
                                 const resip::Data& data) =0;
      /// return false if not found     
      virtual bool dbReadRecord(const Table table, 
                                const resip::Data& key, 
                                resip::Data& data) const =0;
      virtual void dbEraseRecord(const Table table, 
                                 const resip::Data& key,
                                 bool isSecondaryKey=false) =0;  // allows deleting records from a table that supports secondary keying using a secondary key
      virtual resip::Data dbFirstKey(const Table table);
      virtual resip::Data dbNextKey(const Table table,
                                    bool first=false) = 0; // return empty if no more

      // Methods for tables that allow duplicate keys
      virtual bool dbFirstRecord(const Table table,
                                 const resip::Data& key,
                                 resip::Data& data,
                                 bool forUpdate); 
      virtual bool dbNextRecord(const Table table,
                                const resip::Data& key,
                                resip::Data& data,
                                bool forUpdate,
                                bool first=false) = 0; // return false if no more
      virtual bool dbBeginTransaction(const Table table) = 0;
      virtual bool dbCommitTransaction(const Table table) = 0;
      virtual bool dbRollbackTransaction(const Table table) = 0;

      virtual void encodeUser(const UserRecord& rec, resip::Data& buffer);
      virtual void encodeRoute(const RouteRecord& rec, resip::Data& buffer);
      virtual void encodeFilter(const FilterRecord& rec, resip::Data& buffer);
      virtual void decodeSiloRecord(resip::Data& data, SiloRecord& rec);
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
 */
