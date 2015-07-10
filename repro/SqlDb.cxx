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

SqlDb::SqlDb() : mConnected(false)
{
}

void 
SqlDb::eraseUser(const AbstractDb::Key& key )
{ 
   Data command;
   {
      DataStream ds(command);
      ds << "DELETE FROM users ";
      userWhereClauseToDataStream(key, ds);
   }
   query(command);
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

static const char usersavp[] = "usersavp";
static const char routesavp[] = "routesavp";
static const char aclsavp[] = "aclsavp";
static const char configsavp[] = "configsavp";
static const char staticregsavp[] = "staticregsavp";
static const char filtersavp[] = "filtersavp";
static const char siloavp[] = "siloavp";

const char*
SqlDb::tableName(Table table) const
{
   switch (table)
   {
      case UserTable:
         resip_assert(false);  // usersavp is not used!
         return usersavp;
      case RouteTable:
         return routesavp;
      case AclTable:
         return aclsavp; 
      case ConfigTable:
         return configsavp;
      case StaticRegTable:
         return staticregsavp;
      case FilterTable:
         return filtersavp;
      case SiloTable:
         return siloavp;
      default:
         resip_assert(0);
   }
   return 0;
}

void
SqlDb::getUserAndDomainFromKey(const Key& key, Data& user, Data& domain) const
{
   ParseBuffer pb(key);
   const char* start = pb.position();
   pb.skipToOneOf("@");
   pb.data(user, start);
   const char* anchor = pb.skipChar();
   pb.skipToEnd();
   pb.data(domain, anchor);
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

