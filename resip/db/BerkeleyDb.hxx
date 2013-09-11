#if !defined(RESIP_BERKELEYDB_HXX)
#define RESIP_BERKELEYDB_HXX 

#ifdef WIN32
#include <db_cxx.h>
#elif HAVE_CONFIG_H
#include "config.h"
#include DB_HEADER
//#elif defined(__APPLE__) 
//#include <db42/db_cxx.h>
#else
#include <db_cxx.h>
#endif

#include "rutil/Data.hxx"
#include "repro/AbstractDb.hxx"

namespace resip
{
  class TransactionUser;
}

namespace repro
{

class BerkeleyDb: public AbstractDb
{
   public:
      BerkeleyDb();
      BerkeleyDb( const resip::Data& dbPath, const resip::Data& dbName = resip::Data::Empty );
      
      virtual ~BerkeleyDb();

      virtual bool isSane() {return mSane; }
      
   private:
      void init(const resip::Data& dbPath, const resip::Data& dbName);
      static int getSecondaryKeyCallback(Db *db, const Dbt *pkey, const Dbt *pdata, Dbt *skey);

      class TableInfo
      {
      public:
         TableInfo() : mDb(0), mCursor(0), mTransaction(0), mSecondaryDb(0), mSecondaryCursor(0) {}
         Db*    mDb;
         Dbc*   mCursor;
         DbTxn* mTransaction;
         Db*    mSecondaryDb;
         Dbc*   mSecondaryCursor;
      };

      DbEnv* mEnv;
      TableInfo mTableInfo[MaxTable];
      
      bool mSane;
      
      // Db manipulation routines
      virtual bool dbWriteRecord(const Table table, 
                                 const resip::Data& key, 
                                 const resip::Data& data);
      virtual bool dbReadRecord(const Table table, 
                                const resip::Data& key, 
                                resip::Data& data) const; // return false if not found
      virtual void dbEraseRecord(const Table table, 
                                 const resip::Data& key,
                                 bool isSecondaryKey=false);  // allows deleting records from a table that supports secondary keying using a secondary key
      virtual resip::Data dbNextKey(const Table table, 
                                    bool first=true); // return empty if no more  
      virtual bool dbNextRecord(const Table table,
                                const resip::Data& key,
                                resip::Data& data,
                                bool forUpdate,
                                bool first=false);  // return false if no more
      virtual bool dbBeginTransaction(const Table table);
      virtual bool dbCommitTransaction(const Table table);
      virtual bool dbRollbackTransaction(const Table table);
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
