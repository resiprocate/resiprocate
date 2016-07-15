#if !defined(RESIP_POSTGRESQLDB_HXX)
#define RESIP_POSTGRESQLDB_HXX 

#include <libpq-fe.h>

#include "rutil/Data.hxx"
#include "repro/SqlDb.hxx"

namespace resip
{
  class TransactionUser;
}

namespace repro
{

class PostgreSqlDb: public SqlDb
{
   public:
      PostgreSqlDb(const resip::Data& dbConnInfo,
              const resip::Data& dbServer,
              const resip::Data& user, 
              const resip::Data& password, 
              const resip::Data& databaseName, 
              unsigned int port, 
              const resip::Data& customUserAuthQuery);

      ~PostgreSqlDb();
      
      virtual bool addUser( const Key& key, const UserRecord& rec );
      virtual UserRecord getUser( const Key& key ) const;
      virtual resip::Data getUserAuthInfo(  const Key& key ) const;
      virtual Key firstUserKey();// return empty if no more
      virtual Key nextUserKey(); // return empty if no more 

      // Perform a query that expects a single result/row - returns all column/field data in a vector
      virtual int singleResultQuery(const resip::Data& queryCommand, std::vector<resip::Data>& fields) const;

   private:
      // Db manipulation routines
      virtual bool dbWriteRecord(const Table table, 
                                 const resip::Data& key, 
                                 const resip::Data& data);
      virtual bool dbReadRecord(const Table table, 
                                const resip::Data& key, 
                                resip::Data& data) const; // return false if not found
      virtual resip::Data dbNextKey(const Table table, 
                                    bool first=true); // return empty if no more
      virtual bool dbNextRecord(const Table table,
                                const resip::Data& key,
                                resip::Data& data,
                                bool forUpdate, // specifying to add SELECT ... FOR UPDATE so the rows are locked
                                bool first=false);  // return false if no more
      virtual bool dbBeginTransaction(const Table table);

      void initialize() const;
      void disconnectFromDatabase() const;
      int connectToDatabase() const;
      int query(const resip::Data& queryCommand, PGresult** result) const;
      virtual int query(const resip::Data& queryCommand) const;
      resip::Data& escapeString(const resip::Data& str, resip::Data& escapedStr) const;

      resip::Data mDBConnInfo;
      resip::Data mDBServer;
      resip::Data mDBUser;
      resip::Data mDBPassword;
      resip::Data mDBName;
      unsigned int mDBPort;
      resip::Data mCustomUserAuthQuery;

      mutable PGconn* mConn;
      mutable PGresult* mResult[MaxTable];
      mutable int mRow[MaxTable];

      void userWhereClauseToDataStream(const Key& key, resip::DataStream& ds) const;
};

}
#endif  

/* ====================================================================
 * 
 * Copyright (c) 2015 Daniel Pocock.  All rights reserved.
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * ====================================================================
 */

