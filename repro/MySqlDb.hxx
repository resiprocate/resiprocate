#if !defined(RESIP_MYSQLDB_HXX)
#define RESIP_MYSQLDB_HXX 

#ifdef WIN32
#include <mysql.h>
#else 
#include <mysql/mysql.h>
#endif

#include "rutil/Data.hxx"
#include "repro/SqlDb.hxx"

namespace resip
{
  class TransactionUser;
}

namespace repro
{

class MySqlDb: public SqlDb
{
   public:
      MySqlDb(const resip::Data& dbServer, 
              const resip::Data& user, 
              const resip::Data& password, 
              const resip::Data& databaseName, 
              unsigned int port, 
              const resip::Data& customUserAuthQuery);
      
      ~MySqlDb();

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
      int query(const resip::Data& queryCommand, MYSQL_RES** result) const;
      virtual int query(const resip::Data& queryCommand) const;
      resip::Data& escapeString(const resip::Data& str, resip::Data& escapedStr) const;

      resip::Data mDBServer;
      resip::Data mDBUser;
      resip::Data mDBPassword;
      resip::Data mDBName;
      unsigned int mDBPort;
      resip::Data mCustomUserAuthQuery;

      mutable MYSQL* mConn;
      mutable MYSQL_RES* mResult[MaxTable];

      void userWhereClauseToDataStream(const Key& key, resip::DataStream& ds) const;
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
