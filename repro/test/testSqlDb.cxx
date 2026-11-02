// Tests for the SQL backends (MySqlDb, PostgreSqlDb): values taken from SIP
// requests, TLS certificates and the web admin must reach the database as
// data, never as SQL, and a missing or dropped connection must fail cleanly.
//
// The checks that need a server run against each backend that is compiled in
// and has a server configured through the environment:
//
//    REPRO_TEST_MYSQL_HOST, REPRO_TEST_MYSQL_PORT, REPRO_TEST_MYSQL_USER,
//    REPRO_TEST_MYSQL_PASSWORD, REPRO_TEST_MYSQL_DB
//    REPRO_TEST_PGSQL_HOST, REPRO_TEST_PGSQL_PORT, REPRO_TEST_PGSQL_USER,
//    REPRO_TEST_PGSQL_PASSWORD, REPRO_TEST_PGSQL_DB
//
// The database must already hold the repro schema (create_mysql_reprodb.sql
// or create_postgresql_reprodb.sql).  The test only touches rows it creates.
// The no connection checks need no server and always run.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <set>
#include <thread>
#include <vector>

#include "rutil/ConfigParse.hxx"
#include "rutil/Data.hxx"
#include "rutil/Log.hxx"
#include "repro/AbstractDb.hxx"
#include "repro/SqlDb.hxx"
#include "repro/UserStore.hxx"
#include "repro/TlsPeerIdentityStore.hxx"
#ifdef USE_MYSQL
#include "repro/MySqlDb.hxx"
#endif
#ifdef USE_POSTGRESQL
#include "repro/PostgreSqlDb.hxx"
#endif

using namespace resip;
using namespace repro;

namespace
{

int gFailures = 0;
const char* gBackend = "";

#define CHECK(cond) \
   do { \
      if(!(cond)) \
      { \
         gFailures++; \
         std::cerr << __FILE__ << ":" << __LINE__ << ": [" << gBackend << "] check failed: " #cond << std::endl; \
      } \
   } while(0)

class TestConfig : public ConfigParse
{
   public:
      void printHelpText(int, char**) override {}
};

struct DbSettings
{
   Data host;
   unsigned int port;
   Data user;
   Data password;
   Data name;
};

typedef SqlDb* (*CreateDb)(const ConfigParse& config, const DbSettings& settings, const Data& customUserAuthQuery);

struct Backend
{
   const char* name;
   const char* envPrefix;
   unsigned int defaultPort;
   CreateDb create;
   const char* userColumn;          // the user column in the users table
   const char* dropConnectionQuery; // ends this session from the server side
};

Data
getEnv(const Data& name, const Data& defaultValue = Data::Empty)
{
#ifdef _MSC_VER
   size_t requiredSize = 0;
   getenv_s(&requiredSize, nullptr, 0, name.c_str());
   if(requiredSize == 0)
   {
      return defaultValue;
   }
   std::vector<char> value(requiredSize);
   getenv_s(&requiredSize, value.data(), value.size(), name.c_str());
   return Data(value.data());
#else
   const char* value = getenv(name.c_str());
   return value ? Data(value) : defaultValue;
#endif
}

std::set<Data>
dataSet(std::initializer_list<const char*> items)
{
   std::set<Data> ret;
   for(const char* item : items)
   {
      ret.insert(Data(item));
   }
   return ret;
}

// Values with everything that has meaning to SQL or to the placeholder and
// tag substitution: quotes, backslashes, ? and tag names
const Data kDomain("ex'am\\ple?.com");
const Data kUser("o'brien\\?$domain");

void
testNoConnection(const Backend& backend)
{
   // Nothing listens on port 1, so the connect in the constructor and every
   // reconnect fails.  Each call must report failure instead of crashing.
   TestConfig config;
   DbSettings settings = { "127.0.0.1", 1, "repro", "repro", "repro" };
   std::unique_ptr<SqlDb> db(backend.create(config, settings, Data::Empty));

   const AbstractDb::Key userKey = UserStore::buildKey(kUser, kDomain);
   CHECK(db->getUserAuthInfo(userKey).empty());
   CHECK(db->getUser(userKey).user.empty());

   AbstractDb::UserRecord rec;
   rec.user = kUser;
   rec.domain = kDomain;
   CHECK(!db->addUser(userKey, rec));
   db->eraseUser(userKey);

   CHECK(!db->isAuthorized(dataSet({"peer'"}), dataSet({"sip:a@b'"})));

   AbstractDb::RouteRecord route;
   route.mMethod = "INVITE";
   CHECK(!db->addRoute("route'key", route));
   CHECK(db->getRoute("route'key").mMethod.empty());
   db->eraseRoute("route'key");
}

void
testUsers(SqlDb& db)
{
   const AbstractDb::Key key = UserStore::buildKey(kUser, kDomain);
   db.eraseUser(key);

   AbstractDb::UserRecord rec;
   rec.user = kUser;
   rec.domain = kDomain;
   rec.realm = "re'alm?";
   rec.passwordHash = "hash'1\\?";
   rec.passwordHashAlt = "alt\"2--";
   rec.name = "Name $user ? '";
   rec.email = "e'mail?@example.com";
   rec.forwardAddress = "sip:f'w@example.com;a=b?";
   CHECK(db.addUser(key, rec));

   AbstractDb::UserRecord got = db.getUser(key);
   CHECK(got.user == rec.user);
   CHECK(got.domain == rec.domain);
   CHECK(got.realm == rec.realm);
   CHECK(got.passwordHash == rec.passwordHash);
   CHECK(got.passwordHashAlt == rec.passwordHashAlt);
   CHECK(got.name == rec.name);
   CHECK(got.email == rec.email);
   CHECK(got.forwardAddress == rec.forwardAddress);
   CHECK(db.getUserAuthInfo(key) == rec.passwordHash);

   // Adding again updates the row
   rec.passwordHash = "hash'2";
   CHECK(db.addUser(key, rec));
   CHECK(db.getUserAuthInfo(key) == rec.passwordHash);

   // Digest usernames that would match every row if they reached the SQL
   CHECK(db.getUserAuthInfo(UserStore::buildKey("x' OR '1'='1", kDomain)).empty());
   CHECK(db.getUserAuthInfo(UserStore::buildKey("x\\' OR 1=1 -- ", kDomain)).empty());
   CHECK(db.getUserAuthInfo(UserStore::buildKey("x' OR 1=1 -- ", kDomain)).empty());
   // The $domain in the username must stay literal
   CHECK(db.getUserAuthInfo(UserStore::buildKey("o'brien\\?" + kDomain, kDomain)).empty());

   db.eraseUser(key);
   CHECK(db.getUserAuthInfo(key).empty());
   CHECK(db.getUser(key).user.empty());
}

void
testCustomUserAuthQuery(const Backend& backend, const DbSettings& settings)
{
   // The custom query finds users stored under "alt." + domain, so a match
   // can only come from it.  The '?' = '?' checks that a literal ? survives.
   Data customQuery;
   {
      DataStream ds(customQuery);
      ds << "SELECT passwordHashAlt FROM users WHERE " << backend.userColumn
         << " = '$user' AND domain = CONCAT('alt.', '$domain') AND '?' = '?'";
   }
   TestConfig config;
   std::unique_ptr<SqlDb> db(backend.create(config, settings, customQuery));

   const Data altDomain = "alt." + kDomain;
   const AbstractDb::Key altKey = UserStore::buildKey(kUser, altDomain);
   db->eraseUser(altKey);

   AbstractDb::UserRecord rec;
   rec.user = kUser;
   rec.domain = altDomain;
   rec.passwordHash = "builtin";
   rec.passwordHashAlt = "custom'hash?";
   CHECK(db->addUser(altKey, rec));

   CHECK(db->getUserAuthInfo(UserStore::buildKey(kUser, kDomain)) == rec.passwordHashAlt);
   CHECK(db->getUserAuthInfo(UserStore::buildKey("x' OR '1'='1", kDomain)).empty());

   db->eraseUser(altKey);
   CHECK(db->getUserAuthInfo(UserStore::buildKey(kUser, kDomain)).empty());
}

void
testTlsPeerIdentities(SqlDb& db)
{
   // A peer name holding the other tag's name, as a certificate CN could
   const Data peer("cn'x$identities\\?");
   const Data identity("sip:a'b@example.com?x");
   const AbstractDb::Key key = TlsPeerIdentityStore::buildKey(peer, identity);
   db.eraseTlsPeerIdentity(key);

   AbstractDb::TlsPeerIdentityRecord rec;
   rec.peerName = peer;
   rec.authorizedIdentity = identity;
   CHECK(db.addTlsPeerIdentity(key, rec));

   AbstractDb::TlsPeerIdentityRecord got = db.getTlsPeerIdentity(key);
   CHECK(got.peerName == peer);
   CHECK(got.authorizedIdentity == identity);

   std::set<Data> peers;
   peers.insert(peer);
   peers.insert("other.example.com");
   std::set<Data> identities;
   identities.insert(identity);
   identities.insert("sip:nobody@example.com");
   CHECK(db.isAuthorized(peers, identities));

   std::set<Data> onePeer;
   onePeer.insert(peer);
   std::set<Data> oneIdentity;
   oneIdentity.insert(identity);
   CHECK(!db.isAuthorized(onePeer, dataSet({"x' OR '1'='1"})));
   CHECK(!db.isAuthorized(onePeer, dataSet({"x') OR 1=1 OR ('"})));
   CHECK(!db.isAuthorized(dataSet({"x') OR 1=1 OR ('"}), oneIdentity));
   CHECK(!db.isAuthorized(dataSet({"cn'x"}), oneIdentity));

   db.eraseTlsPeerIdentity(key);
   CHECK(!db.isAuthorized(onePeer, oneIdentity));
}

void
testCustomTlsAuthQuery(const Backend& backend, const DbSettings& settings)
{
   // An older repro.config example put the list tag inside quotes.  Those
   // quotes are dropped, so this still matches and still can't be broken out of.
   TestConfig config;
   config.insertConfigValue("CustomTlsAuthQuery",
      "SELECT count(1) FROM tlsPeerIdentity WHERE peerName IN ('$peerNames') "
      "AND authorizedIdentity IN ($identities) AND '?' = '?'");
   std::unique_ptr<SqlDb> db(backend.create(config, settings, Data::Empty));

   const Data peer("peer'$identities");
   const Data identity("sip:c'd@example.com");
   const AbstractDb::Key key = TlsPeerIdentityStore::buildKey(peer, identity);
   db->eraseTlsPeerIdentity(key);

   AbstractDb::TlsPeerIdentityRecord rec;
   rec.peerName = peer;
   rec.authorizedIdentity = identity;
   CHECK(db->addTlsPeerIdentity(key, rec));

   std::set<Data> onePeer;
   onePeer.insert(peer);
   std::set<Data> oneIdentity;
   oneIdentity.insert(identity);
   CHECK(db->isAuthorized(onePeer, oneIdentity));
   CHECK(!db->isAuthorized(dataSet({"x') OR 1=1 OR ('"}), oneIdentity));
   CHECK(!db->isAuthorized(onePeer, dataSet({"x') OR 1=1 OR ('"})));

   db->eraseTlsPeerIdentity(key);
   CHECK(!db->isAuthorized(onePeer, oneIdentity));
}

void
testGenericRecords(SqlDb& db)
{
   // Routes use the primary key only
   const AbstractDb::Key routeKey("route'key\\?$user");
   db.eraseRoute(routeKey);
   AbstractDb::RouteRecord route;
   route.mMethod = "INVITE";
   route.mEvent = "ev'ent";
   route.mMatchingPattern = "^sip:(.*)@example.com?$";
   route.mRewriteExpression = "sip:$1@'gw'";
   route.mOrder = 7;
   CHECK(db.addRoute(routeKey, route));
   AbstractDb::RouteRecord gotRoute = db.getRoute(routeKey);
   CHECK(gotRoute.mMethod == route.mMethod);
   CHECK(gotRoute.mEvent == route.mEvent);
   CHECK(gotRoute.mMatchingPattern == route.mMatchingPattern);
   CHECK(gotRoute.mRewriteExpression == route.mRewriteExpression);
   CHECK(gotRoute.mOrder == route.mOrder);
   CHECK(db.getRoute("route' OR '1'='1").mMethod.empty());
   db.eraseRoute(routeKey);
   CHECK(db.getRoute(routeKey).mMethod.empty());

   // The message silo is also looked up by its secondary key, the
   // destination URI of a MESSAGE from the network
   const Data destUri("sip:o'brien\\?@example.com");
   const AbstractDb::Key siloKey("silo'key?");
   db.eraseSiloRecord(siloKey);
   AbstractDb::SiloRecord silo;
   silo.mDestUri = destUri;
   silo.mSourceUri = "sip:src'@example.com";
   silo.mOriginalSentTime = 1234;
   silo.mTid = "tid'?";
   silo.mMimeType = "text/plain";
   silo.mMessageBody = "it's a message?";
   CHECK(db.addToSilo(siloKey, silo));

   AbstractDb::SiloRecordList silos;
   db.getSiloRecords(destUri, silos);
   CHECK(silos.size() == 1);
   if(silos.size() == 1)
   {
      CHECK(silos[0].mDestUri == destUri);
      CHECK(silos[0].mMessageBody == silo.mMessageBody);
   }
   silos.clear();
   db.getSiloRecords("x' OR '1'='1", silos);
   CHECK(silos.empty());

   db.eraseSiloRecord(siloKey);
   silos.clear();
   db.getSiloRecords(destUri, silos);
   CHECK(silos.empty());
}

void
testThreadsAndReconnect(const Backend& backend, SqlDb& db)
{
   // The auth lookups run on several threads that share one connection.  One
   // more thread keeps having the server drop that connection, so the others
   // go through the reconnect path while they escape and query.
   const AbstractDb::Key key = UserStore::buildKey(kUser, kDomain);
   AbstractDb::UserRecord rec;
   rec.user = kUser;
   rec.domain = kDomain;
   rec.passwordHash = "thread'hash";
   CHECK(db.addUser(key, rec));

   const int kThreads = 4;
   const int kLookups = 200;
   std::atomic<int> wrong(0);
   std::atomic<bool> done(false);

   std::thread dropper([&]()
   {
      while(!done)
      {
         std::vector<Data> fields;
         db.singleResultQuery(backend.dropConnectionQuery, fields);
         std::this_thread::sleep_for(std::chrono::milliseconds(5));
      }
   });

   std::vector<std::thread> workers;
   for(int t = 0; t < kThreads; t++)
   {
      workers.emplace_back([&]()
      {
         for(int i = 0; i < kLookups; i++)
         {
            // A lookup that lands right after a drop may fail; it must
            // never return another value
            Data hash = db.getUserAuthInfo(key);
            if(!hash.empty() && hash != rec.passwordHash)
            {
               wrong++;
            }
            if(!db.getUserAuthInfo(UserStore::buildKey("x' OR '1'='1", kDomain)).empty())
            {
               wrong++;
            }
         }
      });
   }
   for(std::thread& worker : workers)
   {
      worker.join();
   }
   done = true;
   dropper.join();
   CHECK(wrong == 0);

   // Once the dropping stops the connection comes back
   CHECK(db.getUserAuthInfo(key) == rec.passwordHash);
   db.eraseUser(key);
}

#ifdef USE_MYSQL
SqlDb*
createMySql(const ConfigParse& config, const DbSettings& settings, const Data& customUserAuthQuery)
{
   return new MySqlDb(config, settings.host, settings.user, settings.password,
                      settings.name, settings.port, customUserAuthQuery);
}
#endif

#ifdef USE_POSTGRESQL
SqlDb*
createPostgreSql(const ConfigParse& config, const DbSettings& settings, const Data& customUserAuthQuery)
{
   return new PostgreSqlDb(config, Data::Empty, settings.host, settings.user, settings.password,
                           settings.name, settings.port, customUserAuthQuery);
}
#endif

}

int
main(int argc, char* argv[])
{
   // CRIT by default: the no connection and reconnect checks log errors on purpose
   Log::initialize(Data("cout"), getEnv("REPRO_TEST_LOG_LEVEL", "CRIT"), Data(argv[0]));

   std::vector<Backend> backends;
#ifdef USE_MYSQL
   backends.push_back({ "MySQL", "REPRO_TEST_MYSQL_", 3306, createMySql, "user", "KILL CONNECTION_ID()" });
#endif
#ifdef USE_POSTGRESQL
   backends.push_back({ "PostgreSQL", "REPRO_TEST_PGSQL_", 5432, createPostgreSql, "username",
                        "SELECT pg_terminate_backend(pg_backend_pid())" });
#endif

   if(backends.empty())
   {
      std::cout << "testSqlDb: built without MySQL or PostgreSQL support, nothing to test" << std::endl;
      return 0;
   }

   for(const Backend& backend : backends)
   {
      gBackend = backend.name;
      testNoConnection(backend);

      const Data prefix(backend.envPrefix);
      DbSettings settings;
      settings.host = getEnv(prefix + "HOST");
      if(settings.host.empty())
      {
         std::cout << "testSqlDb: " << backend.name << ": " << prefix << "HOST not set, skipping the server tests" << std::endl;
         continue;
      }
      settings.port = (unsigned int)getEnv(prefix + "PORT", Data((uint32_t)backend.defaultPort)).convertUnsignedLong();
      settings.user = getEnv(prefix + "USER", "repro");
      settings.password = getEnv(prefix + "PASSWORD");
      settings.name = getEnv(prefix + "DB", "repro");

      TestConfig config;
      std::unique_ptr<SqlDb> db(backend.create(config, settings, Data::Empty));
      testUsers(*db);
      testCustomUserAuthQuery(backend, settings);
      testTlsPeerIdentities(*db);
      testCustomTlsAuthQuery(backend, settings);
      testGenericRecords(*db);
      testThreadsAndReconnect(backend, *db);
      std::cout << "testSqlDb: " << backend.name << ": server tests ran" << std::endl;
   }

   if(gFailures != 0)
   {
      std::cerr << "testSqlDb: " << gFailures << " check(s) failed" << std::endl;
      return 1;
   }
   std::cout << "testSqlDb: all checks passed" << std::endl;
   return 0;
}


/* ====================================================================
 * 
 * Copyright (c) 2026, SIP Spectrum, Inc. https://sipspectrum.com
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
