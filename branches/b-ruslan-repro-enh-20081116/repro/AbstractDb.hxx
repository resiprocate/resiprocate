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

      typedef resip::Data Key;
      typedef std::vector<RouteRecord> RouteRecordList;
      typedef std::vector<AclRecord> AclRecordList;
      typedef std::vector<ConfigRecord> ConfigRecordList;

      // functions for User Records 
      virtual void addUser( const Key& key, const UserRecord& rec );
      virtual void eraseUser( const Key& key );
      virtual void writeUser( const Key& oldkey, const Key& newkey, const UserRecord& rec );
      virtual UserRecord getUser( const Key& key ) const;
      virtual resip::Data getUserAuthInfo(  const Key& key ) const;
      virtual Key firstUserKey();// return empty if no more
      virtual Key nextUserKey(); // return empty if no more 
         
      // functions for Route Records
      virtual void addRoute( const Key& key, const RouteRecord& rec );
      virtual void eraseRoute(  const Key& key );
      virtual void writeRoute( const Key& oldkey, const Key& newkey, const RouteRecord& rec );
      virtual RouteRecord getRoute( const Key& key) const;
      virtual RouteRecordList getAllRoutes();
      virtual Key firstRouteKey();// return empty if no more
      virtual Key nextRouteKey(); // return empty if no more 

      // functions for Acl Records
      virtual void addAcl( const Key& key, const AclRecord& rec );
      virtual void eraseAcl(  const Key& key );
      virtual AclRecordList getAllAcls();
      virtual AclRecord getAcl( const Key& key) const;
      virtual Key firstAclKey();// return empty if no more
      virtual Key nextAclKey(); // return empty if no more 

      // functions for Config Records
      virtual void addConfig( const Key& key, const ConfigRecord& rec );
      virtual void eraseConfig(  const Key& key );
      virtual ConfigRecordList getAllConfigs();
      virtual ConfigRecord getConfig( const Key& key) const;
      virtual Key firstConfigKey();// return empty if no more
      virtual Key nextConfigKey(); // return empty if no more 

 
   protected:
      typedef enum 
      {
         UserTable=0,
         RouteTable,
         AclTable,
         ConfigTable,
         MaxTable  // This one MUST be last 
      } Table;
      
      // Db manipulation routines
      virtual void dbWriteRecord( const Table table, 
                                  const resip::Data& key, 
                                  const resip::Data& data ) =0;
      /// return false if not found     
      virtual bool dbReadRecord( const Table table, 
                                 const resip::Data& key, 
                                 resip::Data& data ) const =0;
      virtual void dbEraseRecord(const Table table, 
                                 const resip::Data& key ) =0;
      virtual resip::Data dbFirstKey(const Table table);
      virtual resip::Data dbNextKey(const Table table, 
                                    bool first=false) =0; // return empty if no more  
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
