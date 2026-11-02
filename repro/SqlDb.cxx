#include <cassert>
#include <cstring>
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
#include "repro/UserStore.hxx"
#include "repro/TlsPeerIdentityStore.hxx"


using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

namespace
{

// A tag in an operator supplied custom query and the values it stands for
struct CustomQueryTag
{
   CustomQueryTag(const char* n, const std::vector<Data>& v, bool q) : name(n), values(&v), quoted(q) {}
   Data name;
   const std::vector<Data>* values;
   bool quoted;   // write a quoted list ('?','?'), otherwise the query has the quotes round the tag
};

// Copies customQuery to ds as a template for SqlDb::fillPlaceholders.  The
// query is scanned once: each tag becomes a placeholder per value, the values
// are added in the order the tags appear, and a literal ? is written as ??.
// Nothing taken from the values is ever written here, so a value that happens
// to contain a tag name can't be expanded again.
void
expandCustomQuery(const Data& customQuery,
                  const std::vector<CustomQueryTag>& tags,
                  DataStream& ds,
                  std::vector<Data>& values)
{
   const char* p = customQuery.data();
   const char* end = p + customQuery.size();
   const char* run = p;
   while(p < end)
   {
      if(*p == '?')
      {
         ds.write(run, p - run);
         ds << "??";
         run = ++p;
         continue;
      }
      if(*p == '$')
      {
         std::vector<CustomQueryTag>::const_iterator tag = tags.begin();
         for(; tag != tags.end(); ++tag)
         {
            if((Data::size_type)(end - p) >= tag->name.size() &&
               memcmp(p, tag->name.data(), tag->name.size()) == 0)
            {
               break;
            }
         }
         if(tag != tags.end())
         {
            ds.write(run, p - run);
            for(std::vector<Data>::size_type i = 0; i < tag->values->size(); i++)
            {
               if(i > 0)
               {
                  ds << ",";
               }
               ds << (tag->quoted ? "'?'" : "?");
            }
            values.insert(values.end(), tag->values->begin(), tag->values->end());
            p += tag->name.size();
            run = p;
            continue;
         }
      }
      p++;
   }
   ds.write(run, end - run);
}

}

SqlDb::SqlDb(const resip::ConfigParse& config) : mConnected(false)
{
   mTlsPeerAuthorizationQuery = config.getConfigData("CustomTlsAuthQuery", "");
   mTableNamePrefix = config.getConfigData("TableNamePrefix", "");
}

void 
SqlDb::eraseUser(const AbstractDb::Key& key )
{ 
   Data command;
   std::vector<Data> values;
   {
      DataStream ds(command);
      ds << "DELETE FROM " << tableName(UserTable) << " ";
      userWhereClauseToDataStream(key, ds, values);
   }
   query(command, values);
}

void
SqlDb::eraseTlsPeerIdentity(const AbstractDb::Key& key )
{
   Data command;
   std::vector<Data> values;
   {
      DataStream ds(command);
      ds << "DELETE FROM " << tableName(TlsPeerIdentityTable) << " ";
      tlsPeerIdentityWhereClauseToDataStream(key, ds, values);
   }
   query(command, values);
}

bool
SqlDb::fillPlaceholders(const Data& queryTemplate, const std::vector<Data>& values, Data& statement) const
{
   statement.clear();
   std::vector<Data>::const_iterator value = values.begin();
   const char* p = queryTemplate.data();
   const char* end = p + queryTemplate.size();
   while(p < end)
   {
      const char* q = static_cast<const char*>(memchr(p, '?', end - p));
      if(q == 0)
      {
         statement.append(p, (Data::size_type)(end - p));
         break;
      }
      statement.append(p, (Data::size_type)(q - p));
      if(q + 1 < end && q[1] == '?')
      {
         statement += '?';
         p = q + 2;
         continue;
      }
      if(value == values.end())
      {
         ErrLog(<< "SQL template has more placeholders than values: " << queryTemplate);
         statement.clear();
         return false;
      }
      Data escaped;
      if(!escapeValue(*value++, escaped))
      {
         statement.clear();
         return false;
      }
      statement += escaped;
      p = q + 1;
   }
   if(value != values.end())
   {
      ErrLog(<< "SQL template has fewer placeholders than values: " << queryTemplate);
      statement.clear();
      return false;
   }
   return true;
}

void
SqlDb::appendCustomUserAuthQuery(const Data& customQuery,
                                 const Data& user,
                                 const Data& domain,
                                 DataStream& ds,
                                 std::vector<Data>& values)
{
   const std::vector<Data> userValue(1, user);
   const std::vector<Data> domainValue(1, domain);
   std::vector<CustomQueryTag> tags;
   tags.push_back(CustomQueryTag("$user", userValue, false));
   tags.push_back(CustomQueryTag("$domain", domainValue, false));
   expandCustomQuery(customQuery, tags, ds, values);
}

void
SqlDb::userWhereClauseToDataStream(const Key& key, DataStream& ds, std::vector<Data>& values) const
{
   Data user;
   Data domain;
   UserStore::getUserAndDomainFromKey(key, user, domain);
   ds << " WHERE " << userColumn() << "='?' AND domain='?'";
   values.push_back(user);
   values.push_back(domain);
}

void
SqlDb::tlsPeerIdentityWhereClauseToDataStream(const Key& key, DataStream& ds, std::vector<Data>& values) const
{
   Data peerName;
   Data authorizedIdentity;
   TlsPeerIdentityStore::getTlsPeerIdentityFromKey(key, peerName, authorizedIdentity);
   ds << " WHERE peerName='?' AND authorizedIdentity='?'";
   values.push_back(peerName);
   values.push_back(authorizedIdentity);
}

bool
SqlDb::isAuthorized(const std::set<resip::Data>& peerNames, const std::set<resip::Data>& identities) const
{
   std::vector<Data> ret;

   // The default query uses the same tags as CustomTlsAuthQuery, so both go
   // through the same substitution
   Data tlsAuthQuery(mTlsPeerAuthorizationQuery);
   if(tlsAuthQuery.empty())
   {
      DataStream ds(tlsAuthQuery);
      ds << "SELECT count(1) FROM " << tableName(TlsPeerIdentityTable)
         << " WHERE peerName IN ($peerNames) AND authorizedIdentity IN ($identities);";
   }

   const std::vector<Data> peerNameValues(peerNames.begin(), peerNames.end());
   const std::vector<Data> identityValues(identities.begin(), identities.end());
   std::vector<CustomQueryTag> tags;
   tags.push_back(CustomQueryTag("$peerNames", peerNameValues, true));
   tags.push_back(CustomQueryTag("$identities", identityValues, true));

   Data queryTemplate;
   std::vector<Data> values;
   {
      DataStream ds(queryTemplate);
      expandCustomQuery(tlsAuthQuery, tags, ds, values);
   }

   if(singleResultQuery(queryTemplate, values, ret) != 0 || ret.size() == 0)
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

