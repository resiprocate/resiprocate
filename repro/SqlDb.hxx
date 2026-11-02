#if !defined(RESIP_SQLDB_HXX)
#define RESIP_SQLDB_HXX 

#include "rutil/ConfigParse.hxx"
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
      SqlDb(const resip::ConfigParse& config);
      
      virtual bool isSane() {return mConnected;}

      virtual void eraseUser( const Key& key );
      virtual void eraseTlsPeerIdentity( const Key& key );

      virtual bool isAuthorized(const std::set<resip::Data>& peerNames, const std::set<resip::Data>& identities) const;

      // Perform a query that expects a single result/row - returns all column/field data in a vector
      virtual int singleResultQuery(const resip::Data& queryCommand, std::vector<resip::Data>& fields) const = 0;
      // As above, with a ? in queryTemplate for each of values (see fillPlaceholders)
      virtual int singleResultQuery(const resip::Data& queryTemplate, const std::vector<resip::Data>& values, std::vector<resip::Data>& fields) const = 0;

   protected:
      virtual void setConnected(bool connected) const { mConnected = connected; }
      virtual bool isConnected() const { return mConnected; }

      // Builds statement from queryTemplate, replacing each ? with the next of
      // values escaped for use inside a single quoted literal, so the template
      // has to supply the quotes (WHERE user='?').  It is one left to right
      // pass, so inserted text is never scanned again.  ?? is a literal ?.
      // Fails if a value can't be escaped or the counts don't match.  This
      // needs the connection, so it must only be called from query() with
      // mMutex held and after the connection check.
      bool fillPlaceholders(const resip::Data& queryTemplate, const std::vector<resip::Data>& values, resip::Data& statement) const;

      // Appends CustomUserAuthQuery to ds as a template for fillPlaceholders,
      // with its $user and $domain tags turned into placeholders and the values
      // added to values
      static void appendCustomUserAuthQuery(const resip::Data& customQuery,
                                            const resip::Data& user,
                                            const resip::Data& domain,
                                            resip::DataStream& ds,
                                            std::vector<resip::Data>& values);

      void userWhereClauseToDataStream(const Key& key, resip::DataStream& ds, std::vector<resip::Data>& values) const;
      void tlsPeerIdentityWhereClauseToDataStream(const Key& key, resip::DataStream& ds, std::vector<resip::Data>& values) const;

      // when multiple threads are in use with the same connection, you need to
      // mutex calls to mysql_query and mysql_store_result:
      // http://dev.mysql.com/doc/refman/5.1/en/threaded-clients.html
      mutable resip::Mutex mMutex;

      resip::Data tableName( Table table ) const;

   private:
      // Db manipulation routines
      virtual void dbEraseRecord(const Table table, 
                                 const resip::Data& key,
                                 bool isSecondaryKey=false);  // allows deleting records from a table that supports secondary keying using a secondary key
      virtual bool dbBeginTransaction(const Table table) = 0;
      virtual bool dbCommitTransaction(const Table table);
      virtual bool dbRollbackTransaction(const Table table);

      virtual int query(const resip::Data& queryCommand) const = 0;
      virtual int query(const resip::Data& queryTemplate, const std::vector<resip::Data>& values) const = 0;
      // Escapes str for use inside a single quoted literal; only used by
      // fillPlaceholders, so values reach SQL only through query()
      virtual bool escapeValue(const resip::Data& str, resip::Data& escapedStr) const = 0;

      // name of the user column in the users table
      virtual const char* userColumn() const = 0;

      mutable volatile bool mConnected;
      resip::Data mTlsPeerAuthorizationQuery;
      resip::Data mTableNamePrefix;
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

