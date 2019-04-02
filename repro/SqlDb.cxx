#include <cassert>
#include <fcntl.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "rutil/ResipAssert.h"
#include "rutil/Data.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"

#include "repro/AbstractDb.hxx"
#include "repro/SqlDb.hxx"


using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

SqlDb::SqlDb(const resip::ConfigParse& config) : mConnected(false)
{
   mTlsPeerAuthorizationQuery = config.getConfigData("CustomTlsAuthQuery", "");
   mTableNamePrefix = config.getConfigData("TableNamePrefix", "");
}

void 
SqlDb::eraseUser(const AbstractDb::Key& key )
{ 
   Data command;
   {
      DataStream ds(command);
      ds << "DELETE FROM " << tableName(UserTable) << " ";
      userWhereClauseToDataStream(key, ds);
   }
   query(command);
}

void
SqlDb::eraseTlsPeerIdentity(const AbstractDb::Key& key )
{
   Data command;
   {
      DataStream ds(command);
      ds << "DELETE FROM " << tableName(TlsPeerIdentityTable) << " ";
      tlsPeerIdentityWhereClauseToDataStream(key, ds);
   }
   query(command);
}

void
SqlDb::setToData(const std::set<resip::Data>& items, resip::Data& result, const resip::Data& sep, const char quote) const
{
   DataStream ds(result);
   std::set<resip::Data>::const_iterator it = items.begin();
   while(it != items.end())
   {
      if(it != items.begin())
      {
         ds << sep;
      }
      if(quote != 0)
      {
         ds << quote << *it << quote;
      }
      else
      {
         ds << *it;
      }
      it++;
   }
}

bool
SqlDb::isAuthorized(const std::set<resip::Data>& peerNames, const std::set<resip::Data>& identities) const
{
   std::vector<Data> ret;

   Data peerNameSet;
   setToData(peerNames, peerNameSet);
   StackLog(<<"peerNameSet = " << peerNameSet);

   Data identitySet;
   setToData(identities, identitySet);
   StackLog(<<"identitySet = " << identitySet);

   Data command;
   if(mTlsPeerAuthorizationQuery.empty())
   {
      DataStream ds(command);
      ds << "SELECT count(1) FROM " << tableName(TlsPeerIdentityTable) << " WHERE peerName IN (" << peerNameSet << ") AND ";
      ds << "authorizedIdentity IN (" << identitySet << ");";
   }
   else
   {
      command = mTlsPeerAuthorizationQuery;
      command.replace("$peerNames", peerNameSet);
      command.replace("$identities", identitySet);
   }

   if(singleResultQuery(command, ret) != 0 || ret.size() == 0)
   {
      return false;
   }
   int count = ret.front().convertInt();

   DebugLog( << "Count is " << count);

   return count > 0;
}

void 
SqlDb::dbEraseRecord(const Table table, 
                       const resip::Data& pKey,
                       bool isSecondaryKey) // allows deleting records from a table that supports secondary keying using a secondary key
{ 
   Data command;
   {
      DataStream ds(command);
      Data escapedKey;
      ds << "DELETE FROM " << tableName(table);
      if(isSecondaryKey)
      {
         ds << " WHERE attr2='" << escapeString(pKey, escapedKey) << "'";
      }
      else
      {
         ds << " WHERE attr='" << escapeString(pKey, escapedKey) << "'";
      }
   }   
   query(command);
}

bool 
SqlDb::dbCommitTransaction(const Table table)
{
   Data command("COMMIT");
   return query(command) == 0;
}

bool 
SqlDb::dbRollbackTransaction(const Table table)
{
   Data command("ROLLBACK");
   return query(command) == 0;
}

static const char userTable[] = "users";
static const char tlsPeerIdentityTable[] = "tlsPeerIdentity";
static const char routesavp[] = "routesavp";
static const char aclsavp[] = "aclsavp";
static const char configsavp[] = "configsavp";
static const char staticregsavp[] = "staticregsavp";
static const char filtersavp[] = "filtersavp";
static const char siloavp[] = "siloavp";

Data
SqlDb::tableName(Table table) const
{
   switch (table)
   {
      case UserTable:
         return mTableNamePrefix + userTable;
      case TlsPeerIdentityTable:
         return mTableNamePrefix + tlsPeerIdentityTable;
      case RouteTable:
         return mTableNamePrefix + routesavp;
      case AclTable:
         return mTableNamePrefix + aclsavp;
      case ConfigTable:
         return mTableNamePrefix + configsavp;
      case StaticRegTable:
         return mTableNamePrefix + staticregsavp;
      case FilterTable:
         return mTableNamePrefix + filtersavp;
      case SiloTable:
         return mTableNamePrefix + siloavp;
      default:
         resip_assert(0);
   }
   return 0;
}

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

