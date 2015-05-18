#if !defined(RESIP_SQLDB_HXX)
#define RESIP_SQLDB_HXX 

#include "rutil/Data.hxx"
#include "repro/AbstractDb.hxx"

namespace resip
{
  class TransactionUser;
}

namespace repro
{

class SqlDb: public AbstractDb
{
   public:
      SqlDb();
      
      virtual bool isSane() {return mConnected;}

      virtual void eraseUser( const Key& key );

      // Perform a query that expects a single result/row - returns all column/field data in a vector
      virtual int singleResultQuery(const resip::Data& queryCommand, std::vector<resip::Data>& fields) const = 0;

   protected:
      virtual void setConnected(bool connected) const { mConnected = connected; }
      virtual bool isConnected() const { return mConnected; }

      // when multiple threads are in use with the same connection, you need to
      // mutex calls to mysql_query and mysql_store_result:
      // http://dev.mysql.com/doc/refman/5.1/en/threaded-clients.html
      mutable resip::Mutex mMutex;

      const char* tableName( Table table ) const;
      void getUserAndDomainFromKey(const AbstractDb::Key& key, resip::Data& user, resip::Data& domain) const;

   private:
      // Db manipulation routines
      virtual void dbEraseRecord(const Table table, 
                                 const resip::Data& key,
                                 bool isSecondaryKey=false);  // allows deleting records from a table that supports secondary keying using a secondary key
      virtual bool dbBeginTransaction(const Table table) = 0;
      virtual bool dbCommitTransaction(const Table table);
      virtual bool dbRollbackTransaction(const Table table);

      virtual int query(const resip::Data& queryCommand) const = 0;
      virtual resip::Data& escapeString(const resip::Data& str, resip::Data& escapedStr) const = 0;

      mutable volatile bool mConnected;

      virtual void userWhereClauseToDataStream(const Key& key, resip::DataStream& ds) const = 0;
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

