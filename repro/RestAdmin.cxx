#include "rutil/ResipAssert.h"

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include <chrono>
#include <sstream>

#include "cajun/json/elements.h"
#include "cajun/json/writer.h"

#include "resip/dum/RegistrationPersistenceManager.hxx"
#include "resip/dum/PublicationPersistenceManager.hxx"
#include "resip/stack/GenericPidfContents.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/Symbols.hxx"
#include "resip/stack/Tuple.hxx"
#include "rutil/Data.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Lock.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/Socket.hxx"
#include "rutil/Timer.hxx"
#include "rutil/TransportType.hxx"

#include "repro/AclStore.hxx"
#include "repro/FilterStore.hxx"
#include "repro/Proxy.hxx"
#include "repro/RestAdmin.hxx"
#include "repro/RouteStore.hxx"
#include "repro/Store.hxx"
#include "repro/UserStore.hxx"
#include "repro/WebAdmin.hxx"

#ifdef USE_SSL
#include "resip/stack/ssl/Security.hxx"
#endif

using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

namespace
{
// Small helper: render a cajun element to a resip::Data.
Data writeJson(const json::UnknownElement& elem)
{
   std::ostringstream oss;
   json::Writer::Write(elem, oss);
   std::string s = oss.str();
   return Data(s.c_str(), s.size());
}

// Build a standard success envelope: {"data": <payload>}
Data successEnvelope(const json::UnknownElement& payload)
{
   json::Object obj;
   obj["data"] = payload;
   return writeJson(obj);
}

// Build an empty success: {"status":"ok"}
Data okEnvelope()
{
   json::Object obj;
   obj["status"] = json::String("ok");
   return writeJson(obj);
}

// Build an error envelope: {"error":{"code":N,"message":"..."}}
Data errorEnvelope(int code, const Data& message)
{
   json::Object err;
   err["code"] = json::Number(code);
   err["message"] = json::String(message.c_str());
   json::Object obj;
   obj["error"] = err;
   return writeJson(obj);
}

// Small helper: turn an unsigned int into a JSON string key.
// Used for maps keyed by SIP response codes (JSON object keys must be strings).
std::string codeToKey(unsigned int code)
{
   std::ostringstream oss;
   oss << code;
   return oss.str();
}

// SIP method types tracked by the StatisticsMessage per-method arrays.
// We enumerate them explicitly so we can map each array slot to a
// human-readable name without depending on a Method<->String helper that
// might not be available in all resip builds.
struct MethodNameEntry
{
   resip::MethodTypes type;
   const char*        name;
};
const MethodNameEntry kTrackedMethods[] =
{
   { resip::INVITE,    "INVITE"    },
   { resip::ACK,       "ACK"       },
   { resip::BYE,       "BYE"       },
   { resip::CANCEL,    "CANCEL"    },
   { resip::MESSAGE,   "MESSAGE"   },
   { resip::OPTIONS,   "OPTIONS"   },
   { resip::REGISTER,  "REGISTER"  },
   { resip::PUBLISH,   "PUBLISH"   },
   { resip::SUBSCRIBE, "SUBSCRIBE" },
   { resip::NOTIFY,    "NOTIFY"    },
   { resip::REFER,     "REFER"     },
   { resip::INFO,      "INFO"      },
   { resip::PRACK,     "PRACK"     },
   { resip::SERVICE,   "SERVICE"   },
   { resip::UPDATE,    "UPDATE"    },
};
const size_t kNumTrackedMethods = sizeof(kTrackedMethods) / sizeof(kTrackedMethods[0]);

// Build a sparse JSON object from a responsesByCode-style array.
// Only includes codes whose count is non-zero.
json::Object responseCodeMap(const unsigned int* codeArr,
                             unsigned int maxCode)
{
   json::Object obj;
   for (unsigned int c = 0; c < maxCode; ++c)
   {
      if (codeArr[c] != 0)
      {
         obj[codeToKey(c)] = json::Number(codeArr[c]);
      }
   }
   return obj;
}

// Build a sparse JSON object keyed by method name for a *ByMethod array.
// Only includes methods whose count is non-zero.
json::Object methodMap(const unsigned int* methodArr)
{
   json::Object obj;
   for (size_t i = 0; i < kNumTrackedMethods; ++i)
   {
      unsigned int v = methodArr[kTrackedMethods[i].type];
      if (v != 0)
      {
         obj[kTrackedMethods[i].name] = json::Number(v);
      }
   }
   return obj;
}

// Build a nested sparse JSON object for a *ByMethodByCode 2-D array.
// Outer keys are method names, inner keys are response codes.
// Methods with no non-zero codes are omitted entirely.
json::Object methodCodeMap(const unsigned int (*arr)[resip::StatisticsMessage::Payload::MaxCode])
{
   json::Object obj;
   for (size_t i = 0; i < kNumTrackedMethods; ++i)
   {
      resip::MethodTypes m = kTrackedMethods[i].type;
      json::Object inner = responseCodeMap(
            arr[m], resip::StatisticsMessage::Payload::MaxCode);
      // Only include the method if it has any non-zero codes.
      if (!inner.Empty())
      {
         obj[kTrackedMethods[i].name] = inner;
      }
   }
   return obj;
}

// Turn a StatisticsMessage::Payload into a json::Object with one entry per
// counter / sparse sub-object per array. Only response codes and methods
// that have non-zero counts are included, to keep payload size bounded.
json::Object payloadToJson(const resip::StatisticsMessage::Payload& p)
{
   json::Object obj;

   // --- Scalar counters (mirror exactly) ---
   obj["tuFifoSize"]               = json::Number(p.tuFifoSize);
   obj["transportFifoSizeSum"]     = json::Number(p.transportFifoSizeSum);
   obj["transactionFifoSize"]      = json::Number(p.transactionFifoSize);
   obj["activeTimers"]             = json::Number(p.activeTimers);
   obj["openTcpConnections"]       = json::Number(p.openTcpConnections);
   obj["activeClientTransactions"] = json::Number(p.activeClientTransactions);
   obj["activeServerTransactions"] = json::Number(p.activeServerTransactions);
   obj["pendingDnsQueries"]        = json::Number(p.pendingDnsQueries);

   obj["requestsSent"]             = json::Number(p.requestsSent);
   obj["responsesSent"]            = json::Number(p.responsesSent);
   obj["requestsRetransmitted"]    = json::Number(p.requestsRetransmitted);
   obj["responsesRetransmitted"]   = json::Number(p.responsesRetransmitted);
   obj["requestsReceived"]         = json::Number(p.requestsReceived);
   obj["responsesReceived"]        = json::Number(p.responsesReceived);

   // --- Sparse response-code histogram ---
   obj["responsesByCode"] =
      responseCodeMap(p.responsesByCode,
                      resip::StatisticsMessage::Payload::MaxCode);

   // --- Per-method counters (sparse, keyed by method name) ---
   obj["requestsSentByMethod"]           = methodMap(p.requestsSentByMethod);
   obj["requestsRetransmittedByMethod"]  = methodMap(p.requestsRetransmittedByMethod);
   obj["requestsReceivedByMethod"]       = methodMap(p.requestsReceivedByMethod);
   obj["responsesSentByMethod"]          = methodMap(p.responsesSentByMethod);
   obj["responsesRetransmittedByMethod"] = methodMap(p.responsesRetransmittedByMethod);
   obj["responsesReceivedByMethod"]      = methodMap(p.responsesReceivedByMethod);

   // --- Per-method per-code response histograms (2-level sparse) ---
   obj["responsesSentByMethodByCode"]          = methodCodeMap(p.responsesSentByMethodByCode);
   obj["responsesRetransmittedByMethodByCode"] = methodCodeMap(p.responsesRetransmittedByMethodByCode);
   obj["responsesReceivedByMethodByCode"]      = methodCodeMap(p.responsesReceivedByMethodByCode);

   return obj;
}
}


RestAdmin::RestAdmin(WebAdmin& webAdmin)
   : mWebAdmin(webAdmin)
{
}

RestAdmin::~RestAdmin()
{
}

void
RestAdmin::parseRequest(const Data& uri,
                        std::vector<Data>& pathSegments,
                        ParamMap& queryParams) const
{
   pathSegments.clear();
   queryParams.clear();

   // Split URI into path portion and query portion.
   Data pathPart;
   Data queryPart;
   Data::size_type q = uri.find("?");
   if (q == Data::npos)
   {
      pathPart = uri;
   }
   else
   {
      pathPart = uri.substr(0, q);
      queryPart = uri.substr(q + 1);
   }

   // Strip the /api/v1 prefix so remaining path is e.g. "users/alice@example.com".
   const Data prefix("/api/v1");
   if (pathPart.prefix(prefix))
   {
      pathPart = pathPart.substr(prefix.size());
   }
   // Strip leading slash from what remains.
   if (!pathPart.empty() && pathPart.data()[0] == '/')
   {
      pathPart = pathPart.substr(1);
   }

   // Split path on '/'. Each segment is URL-decoded.
   if (!pathPart.empty())
   {
      ParseBuffer pb(pathPart);
      while (!pb.eof())
      {
         const char* anchor = pb.position();
         pb.skipToChar('/');
         Data seg;
         pb.data(seg, anchor);
         pathSegments.push_back(seg.urlDecoded());
         if (!pb.eof())
         {
            pb.skipChar('/');
         }
      }
   }

   // Parse query string "k=v&k=v". Values URL-decoded; keys kept raw
   // (they're simple ASCII names in practice).
   if (!queryPart.empty())
   {
      ParseBuffer pb(queryPart);
      while (!pb.eof())
      {
         const char* anchor1 = pb.position();
         pb.skipToChar('=');
         Data key;
         pb.data(key, anchor1);

         Data value;
         if (!pb.eof())
         {
            const char* anchor2 = pb.skipChar('=');
            pb.skipToChar('&');
            pb.data(value, anchor2);
         }

         if (!pb.eof())
         {
            pb.skipChar('&');
         }

         if (!key.empty())
         {
            queryParams[key] = value.urlDecoded();
         }
      }
   }
}

void
RestAdmin::sendJson(int pageNumber, int statusCode, const Data& json)
{
   mWebAdmin.setApiResponse(pageNumber, statusCode, json);
}

void
RestAdmin::sendOk(int pageNumber)
{
   sendJson(pageNumber, 200, okEnvelope());
}

void
RestAdmin::sendError(int pageNumber, int statusCode, const Data& message)
{
   sendJson(pageNumber, statusCode, errorEnvelope(statusCode, message));
}

void
RestAdmin::sendMethodNotAllowed(int pageNumber, const Data& method)
{
   sendError(pageNumber, 405, Data("Method not allowed: ") + method);
}

void
RestAdmin::sendNotFound(int pageNumber)
{
   sendJson(pageNumber, 404, errorEnvelope(404, "Not found"));
}

Data
RestAdmin::param(const ParamMap& q, const Data& key, const Data& defaultValue)
{
   ParamMap::const_iterator it = q.find(key);
   if (it == q.end())
   {
      return defaultValue;
   }
   return it->second;
}

bool
RestAdmin::hasParam(const ParamMap& q, const Data& key)
{
   return q.find(key) != q.end();
}

void
RestAdmin::dispatch(const Data& method,
                    const Data& uri,
                    int pageNumber,
                    const Data& /*authenticatedUser*/)
{
   std::vector<Data> path;
   ParamMap query;
   parseRequest(uri, path, query);

   DebugLog(<< "RestAdmin dispatch: method=" << method << " uri=" << uri
            << " segments=" << path.size());

   if (path.empty())
   {
      sendNotFound(pageNumber);
      return;
   }

   const Data& resource = path[0];

   if (resource == "registrations")
   {
      handleRegistrations(method, path, query, pageNumber);
   }
   else if (resource == "publications")
   {
      handlePublications(method, path, query, pageNumber);
   }
   else if (resource == "domains")
   {
      handleDomains(method, path, query, pageNumber);
   }
   else if (resource == "users")
   {
      handleUsers(method, path, query, pageNumber);
   }
   else if (resource == "acls")
   {
      handleAcls(method, path, query, pageNumber);
   }
   else if (resource == "routes")
   {
      handleRoutes(method, path, query, pageNumber);
   }
   else if (resource == "filters")
   {
      handleFilters(method, path, query, pageNumber);
   }
   else if (resource == "settings")
   {
      handleSettings(method, pageNumber);
   }
   else if (resource == "stackinfo")
   {
      handleStackInfo(method, pageNumber);
   }
   else if (resource == "stats")
   {
      handleStats(method, path, pageNumber);
   }
   else if (resource == "congestion")
   {
      handleCongestion(method, pageNumber);
   }
   else if (resource == "loglevel")
   {
      handleLogLevel(method, query, pageNumber);
   }
   else if (resource == "dnscache")
   {
      handleDnsCache(method, path, pageNumber);
   }
   else if (resource == "restart")
   {
      handleRestart(method, pageNumber);
   }
   else if (resource == "certs")
   {
      handleCertsReload(method, pageNumber);
   }
   else
   {
      sendNotFound(pageNumber);
   }
}

// ---------------------------------------------------------------------------
// Registrations
// ---------------------------------------------------------------------------
void
RestAdmin::handleRegistrations(const Data& method,
                               const std::vector<Data>& path,
                               const ParamMap& /*query*/,
                               int pageNumber)
{
   RegistrationPersistenceManager& regDb = mWebAdmin.mRegDb;

   if (method == "GET" && path.size() == 1)
   {
      uint64_t now = Timer::getTimeSecs();
      json::Array aorArr;

      RegistrationPersistenceManager::UriList aors;
      regDb.getAors(aors);
      for (RegistrationPersistenceManager::UriList::const_iterator aorIt = aors.begin();
           aorIt != aors.end(); ++aorIt)
      {
         Uri aor = *aorIt;
         ContactList contacts;
         regDb.getContacts(aor, contacts);

         json::Array contactArr;
         for (ContactList::iterator i = contacts.begin();
              i != contacts.end(); ++i)
         {
            if (i->mRegExpires > now)
            {
               const ContactInstanceRecord& r = *i;
               json::Object c;
               c["contact"] = json::String(Data::from(r.mContact.uri()).c_str());
               c["userAgent"] = json::String(r.mUserAgent.c_str());
               c["instanceId"] = json::String(r.mInstance.c_str());
               c["regId"] = json::Number(r.mRegId);
               if (r.mContact.exists(p_q))
               {
#ifdef RESIP_FIXED_POINT
                  c["qValue"] = json::Number(r.mContact.param(p_q));
#else
                  c["qValue"] = json::Number(r.mContact.param(p_q).floatVal());
#endif
               }
               json::Array pathArr;
               for (NameAddrs::const_iterator naIt = r.mSipPath.begin();
                    naIt != r.mSipPath.end(); ++naIt)
               {
                  pathArr.Insert(json::String(Data::from(naIt->uri()).c_str()));
               }
               c["path"] = pathArr;
               c["synced"] = json::Boolean(r.mSyncContact);
               bool staticRegContact = (r.mRegExpires == NeverExpire);
               c["static"] = json::Boolean(staticRegContact);
               if (staticRegContact)
               {
                  c["expiresIn"] = json::Null();
               }
               else
               {
                  c["expiresIn"] = json::Number((double)(r.mRegExpires - now));
               }
               contactArr.Insert(c);
            }
            else
            {
               // Opportunistically remove expired contacts, matching HTML behavior.
               regDb.removeContact(aor, *i);
            }
         }

         json::Object aorObj;
         aorObj["aor"] = json::String(Data::from(aor).c_str());
         aorObj["contacts"] = contactArr;
         aorArr.Insert(aorObj);
      }

      sendJson(pageNumber, 200, successEnvelope(aorArr));
      return;
   }

   if (method == "DELETE" && path.size() == 2)
   {
      // DELETE /api/v1/registrations/{aor} -> remove all contacts for this AOR
      const Data& aorStr = path[1];
      try
      {
         Uri aor(aorStr);
         ContactList contacts;
         regDb.getContacts(aor, contacts);
         int removed = 0;
         for (ContactList::iterator i = contacts.begin(); i != contacts.end(); ++i)
         {
            regDb.removeContact(aor, *i);
            // If this was a static registration, also remove from the store.
            if (i->mRegExpires == NeverExpire)
            {
               mWebAdmin.mStore.mStaticRegStore.eraseStaticReg(aor, i->mContact);
            }
            ++removed;
         }
         json::Object payload;
         payload["removed"] = json::Number(removed);
         payload["aor"] = json::String(aorStr.c_str());
         sendJson(pageNumber, 200, successEnvelope(payload));
      }
      catch (BaseException& e)
      {
         WarningLog(<< "REST delete registration: bad AOR " << aorStr << ": " << e);
         sendError(pageNumber, 400, Data("Invalid AOR: ") + aorStr);
      }
      return;
   }

   sendMethodNotAllowed(pageNumber, method);
}

// ---------------------------------------------------------------------------
// Publications
// ---------------------------------------------------------------------------
void
RestAdmin::handlePublications(const Data& method,
                              const std::vector<Data>& path,
                              const ParamMap& /*query*/,
                              int pageNumber)
{
   PublicationPersistenceManager& pubDb = mWebAdmin.mPubDb;

   if (method == "GET" && path.size() == 1)
   {
      uint64_t now = Timer::getTimeSecs();
      json::Array arr;

      pubDb.lockDocuments();
      PublicationPersistenceManager::KeyToETagMap& publications = pubDb.getDocuments();
      for (PublicationPersistenceManager::KeyToETagMap::iterator keyIt = publications.begin();
           keyIt != publications.end(); ++keyIt)
      {
         for (PublicationPersistenceManager::ETagToDocumentMap::iterator eTagIt = keyIt->second.begin();
              eTagIt != keyIt->second.end(); ++eTagIt)
         {
            if (eTagIt->second.mExpirationTime > now)
            {
               json::Object pubObj;
               pubObj["aor"] = json::String(eTagIt->second.mDocumentKey.c_str());
               pubObj["eventType"] = json::String(eTagIt->second.mEventType.c_str());
               pubObj["eTag"] = json::String(eTagIt->second.mETag.c_str());

               GenericPidfContents* pidf = dynamic_cast<GenericPidfContents*>(eTagIt->second.mContents.get());
               if (pidf)
               {
                  Data summary;
                  summary += (pidf->getSimplePresenceOnline() ? "open" : "closed");
                  if (!pidf->getSimplePresenceNote().empty())
                  {
                     summary += " - ";
                     summary += pidf->getSimplePresenceNote();
                  }
                  pubObj["data"] = json::String(summary.c_str());
               }
               else
               {
                  pubObj["data"] = json::Null();
               }

               pubObj["synced"] = json::Boolean(eTagIt->second.mSyncPublication);
               pubObj["expiresIn"] = json::Number((double)(eTagIt->second.mExpirationTime - now));
               arr.Insert(pubObj);
            }
         }
      }
      pubDb.unlockDocuments();

      sendJson(pageNumber, 200, successEnvelope(arr));
      return;
   }

   sendMethodNotAllowed(pageNumber, method);
}

// ---------------------------------------------------------------------------
// Domains
// ---------------------------------------------------------------------------
void
RestAdmin::handleDomains(const Data& method,
                         const std::vector<Data>& path,
                         const ParamMap& query,
                         int pageNumber)
{
   ConfigStore& configStore = mWebAdmin.mStore.mConfigStore;

   if (method == "GET" && path.size() == 1)
   {
      json::Array arr;
      const ConfigStore::ConfigData& configs = configStore.getConfigs();
      for (ConfigStore::ConfigData::const_iterator i = configs.begin();
           i != configs.end(); ++i)
      {
         json::Object obj;
         obj["domain"] = json::String(i->second.mDomain.c_str());
         obj["tlsPort"] = json::Number(i->second.mTlsPort);
         arr.Insert(obj);
      }
      sendJson(pageNumber, 200, successEnvelope(arr));
      return;
   }

   if (method == "POST" && path.size() == 1)
   {
      if (!hasParam(query, "uri"))
      {
         sendError(pageNumber, 400, "Missing required parameter: uri");
         return;
      }
      Data uri = param(query, "uri");
      int tlsPort = param(query, "tlsPort", "0").convertInt();
      if (configStore.addDomain(uri, tlsPort))
      {
         json::Object obj;
         obj["domain"] = json::String(uri.c_str());
         obj["tlsPort"] = json::Number(tlsPort);
         sendJson(pageNumber, 200, successEnvelope(obj));
      }
      else
      {
         sendError(pageNumber, 500, "Failed to add domain (database error)");
      }
      return;
   }

   if (method == "DELETE" && path.size() == 2)
   {
      const Data& domain = path[1];
      configStore.eraseDomain(domain);
      sendOk(pageNumber);
      return;
   }

   sendMethodNotAllowed(pageNumber, method);
}

// ---------------------------------------------------------------------------
// Users
// ---------------------------------------------------------------------------
void
RestAdmin::handleUsers(const Data& method,
                       const std::vector<Data>& path,
                       const ParamMap& query,
                       int pageNumber)
{
   UserStore& userStore = mWebAdmin.mStore.mUserStore;

   // Helper lambda to build a JSON object for a user record
   auto userToJson = [](const AbstractDb::UserRecord& rec) -> json::Object {
      json::Object obj;
      obj["user"] = json::String(rec.user.c_str());
      obj["domain"] = json::String(rec.domain.c_str());
      obj["realm"] = json::String(rec.realm.c_str());
      obj["name"] = json::String(rec.name.c_str());
      obj["email"] = json::String(rec.email.c_str());
      return obj;
   };

   // GET /api/v1/users -> list all
   if (method == "GET" && path.size() == 1)
   {
      json::Array arr;
      int count = 0;
      UserStore::Key key = userStore.getFirstKey();
      while (!key.empty())
      {
         AbstractDb::UserRecord rec = userStore.getUserInfo(key);
         json::Object obj = userToJson(rec);
         // include the AOR-style identifier clients can use in PUT/DELETE
         obj["id"] = json::String((rec.user + Data("@") + rec.domain).c_str());
         arr.Insert(obj);

         if (++count > 10000)  // safety cap
         {
            break;
         }
         key = userStore.getNextKey();
      }
      sendJson(pageNumber, 200, successEnvelope(arr));
      return;
   }

   // POST /api/v1/users?user=...&domain=...&password=...&name=...&email=...
   if (method == "POST" && path.size() == 1)
   {
      if (!hasParam(query, "user") || !hasParam(query, "domain"))
      {
         sendError(pageNumber, 400, "Missing required parameter: user and/or domain");
         return;
      }
      Data user = param(query, "user");
      Data domain = param(query, "domain");
      Data password = param(query, "password");
      Data name = param(query, "name");
      Data email = param(query, "email");

      // Matches HTML form: realm is set to domain
      if (userStore.addUser(user, domain, domain, password, true, name, email))
      {
         json::Object obj;
         obj["user"] = json::String(user.c_str());
         obj["domain"] = json::String(domain.c_str());
         obj["id"] = json::String((user + Data("@") + domain).c_str());
         sendJson(pageNumber, 200, successEnvelope(obj));
      }
      else
      {
         sendError(pageNumber, 500, "Failed to add user (database error; possible duplicate)");
      }
      return;
   }

   // All remaining operations need {user@domain} in path[1]
   if (path.size() < 2)
   {
      sendMethodNotAllowed(pageNumber, method);
      return;
   }

   // Parse {user@domain}
   const Data& id = path[1];
   Data userPart;
   Data domainPart;
   {
      Data::size_type at = id.find("@");
      if (at == Data::npos)
      {
         sendError(pageNumber, 400, "User identifier must be user@domain");
         return;
      }
      userPart = id.substr(0, at);
      domainPart = id.substr(at + 1);
   }

   // The key uses realm; HTML code uses domain as realm.
   UserStore::Key key = userStore.buildKey(userPart, domainPart);

   if (method == "GET")
   {
      AbstractDb::UserRecord rec = userStore.getUserInfo(key);
      if (rec.user.empty())
      {
         sendNotFound(pageNumber);
         return;
      }
      json::Object obj = userToJson(rec);
      obj["id"] = json::String((rec.user + Data("@") + rec.domain).c_str());
      sendJson(pageNumber, 200, successEnvelope(obj));
      return;
   }

   if (method == "PUT")
   {
      AbstractDb::UserRecord rec = userStore.getUserInfo(key);
      if (rec.user.empty())
      {
         sendNotFound(pageNumber);
         return;
      }

      // Partial-update semantics: any field not supplied stays unchanged.
      // Note that the stored password hash is MD5(user:realm:password), so
      // if the username or domain changes, the old hash is no longer valid
      // for the new identity. In that case a new password is mandatory.
      Data newUser   = hasParam(query, "user")   ? param(query, "user")   : rec.user;
      Data newDomain = hasParam(query, "domain") ? param(query, "domain") : rec.domain;
      Data newName   = hasParam(query, "name")   ? param(query, "name")   : rec.name;
      Data newEmail  = hasParam(query, "email")  ? param(query, "email")  : rec.email;

      // Realm tracks domain (matching how addUser is called elsewhere).
      Data newRealm = newDomain;

      bool identityChanged = (newUser != rec.user) || (newDomain != rec.domain);
      bool passwordProvided = hasParam(query, "password") && !param(query, "password").empty();

      if (identityChanged && !passwordProvided)
      {
         sendError(pageNumber, 400,
                   "Changing user or domain requires a new password "
                   "(the stored password hash is bound to user+realm)");
         return;
      }

      Data password;
      Data passwordHashAlt;
      bool applyA1HashToPassword;
      if (passwordProvided)
      {
         password = param(query, "password");
         applyA1HashToPassword = true;
      }
      else
      {
         // Identity is unchanged and no new password supplied: keep existing hash.
         password = rec.passwordHash;
         passwordHashAlt = rec.passwordHashAlt;
         applyA1HashToPassword = false;
      }

      // updateUser handles a change of key internally (adds new, deletes old).
      if (userStore.updateUser(key, newUser, newDomain, newRealm, password,
                               applyA1HashToPassword, newName, newEmail,
                               passwordHashAlt))
      {
         // If identity changed, the record now lives under a different key.
         UserStore::Key updatedKey = identityChanged
                                     ? userStore.buildKey(newUser, newRealm)
                                     : key;
         AbstractDb::UserRecord updated = userStore.getUserInfo(updatedKey);
         json::Object obj = userToJson(updated);
         obj["id"] = json::String((updated.user + Data("@") + updated.domain).c_str());
         sendJson(pageNumber, 200, successEnvelope(obj));
      }
      else
      {
         sendError(pageNumber, 500, "Failed to update user (database error)");
      }
      return;
   }

   if (method == "DELETE")
   {
      AbstractDb::UserRecord rec = userStore.getUserInfo(key);
      if (rec.user.empty())
      {
         sendNotFound(pageNumber);
         return;
      }
      userStore.eraseUser(key);
      sendOk(pageNumber);
      return;
   }

   sendMethodNotAllowed(pageNumber, method);
}

// ---------------------------------------------------------------------------
// ACLs
// ---------------------------------------------------------------------------
void
RestAdmin::handleAcls(const Data& method,
                      const std::vector<Data>& path,
                      const ParamMap& query,
                      int pageNumber)
{
   AclStore& aclStore = mWebAdmin.mStore.mAclStore;

   if (method == "GET" && path.size() == 1)
   {
      json::Array arr;

      // TLS peer name ACLs
      AclStore::Key key = aclStore.getFirstTlsPeerNameKey();
      while (key != Data::Empty)
      {
         json::Object obj;
         obj["key"] = json::String(key.c_str());
         obj["type"] = json::String("tlsPeerName");
         obj["tlsPeerName"] = json::String(aclStore.getTlsPeerName(key).c_str());
         arr.Insert(obj);
         key = aclStore.getNextTlsPeerNameKey(key);
      }

      // Address ACLs
      key = aclStore.getFirstAddressKey();
      while (key != Data::Empty)
      {
         json::Object obj;
         obj["key"] = json::String(key.c_str());
         obj["type"] = json::String("address");
         obj["address"] = json::String(aclStore.getAddressTuple(key).presentationFormat().c_str());
         obj["mask"] = json::Number(aclStore.getAddressMask(key));
         obj["port"] = json::Number(aclStore.getAddressTuple(key).getPort());
         obj["transport"] = json::String(Tuple::toData(aclStore.getAddressTuple(key).getType()).c_str());
         arr.Insert(obj);
         key = aclStore.getNextAddressKey(key);
      }

      sendJson(pageNumber, 200, successEnvelope(arr));
      return;
   }

   if (method == "POST" && path.size() == 1)
   {
      if (!hasParam(query, "hostOrIp"))
      {
         sendError(pageNumber, 400, "Missing required parameter: hostOrIp");
         return;
      }
      Data hostOrIp = param(query, "hostOrIp");
      int port = param(query, "port", "0").convertInt();
      TransportType transport = Tuple::toTransport(param(query, "transport", "UDP"));

      if (aclStore.addAcl(hostOrIp, port, transport))
      {
         json::Object obj;
         obj["hostOrIp"] = json::String(hostOrIp.c_str());
         obj["port"] = json::Number(port);
         obj["transport"] = json::String(param(query, "transport", "UDP").c_str());
         sendJson(pageNumber, 200, successEnvelope(obj));
      }
      else
      {
         sendError(pageNumber, 400, Data("Failed to add ACL (parse error or database error): ") + hostOrIp);
      }
      return;
   }

   if (method == "DELETE" && path.size() == 2)
   {
      const Data& key = path[1];
      aclStore.eraseAcl(key);
      sendOk(pageNumber);
      return;
   }

   sendMethodNotAllowed(pageNumber, method);
}

// ---------------------------------------------------------------------------
// Routes
// ---------------------------------------------------------------------------
void
RestAdmin::handleRoutes(const Data& method,
                        const std::vector<Data>& path,
                        const ParamMap& query,
                        int pageNumber)
{
   RouteStore& routeStore = mWebAdmin.mStore.mRouteStore;

   auto routeToJson = [](const AbstractDb::RouteRecord& rec, const Data& key) -> json::Object {
      json::Object obj;
      obj["key"] = json::String(key.c_str());
      obj["uri"] = json::String(rec.mMatchingPattern.c_str());
      obj["method"] = json::String(rec.mMethod.c_str());
      obj["event"] = json::String(rec.mEvent.c_str());
      obj["destination"] = json::String(rec.mRewriteExpression.c_str());
      obj["order"] = json::Number(rec.mOrder);
      return obj;
   };

   if (method == "GET" && path.size() == 1)
   {
      json::Array arr;
      for (RouteStore::Key key = routeStore.getFirstKey();
           !key.empty();
           key = routeStore.getNextKey(key))
      {
         AbstractDb::RouteRecord rec = routeStore.getRouteRecord(key);
         arr.Insert(routeToJson(rec, key));
      }
      sendJson(pageNumber, 200, successEnvelope(arr));
      return;
   }

   if (method == "POST" && path.size() == 1)
   {
      if (!hasParam(query, "uri") || !hasParam(query, "destination"))
      {
         sendError(pageNumber, 400, "Missing required parameter: uri and/or destination");
         return;
      }
      Data uri = param(query, "uri");
      Data destination = param(query, "destination");
      Data rmethod = param(query, "method");
      Data event = param(query, "event");
      int order = param(query, "order", "0").convertInt();

      if (routeStore.addRoute(rmethod, event, uri, destination, order))
      {
         json::Object obj;
         obj["uri"] = json::String(uri.c_str());
         obj["destination"] = json::String(destination.c_str());
         obj["method"] = json::String(rmethod.c_str());
         obj["event"] = json::String(event.c_str());
         obj["order"] = json::Number(order);
         sendJson(pageNumber, 200, successEnvelope(obj));
      }
      else
      {
         sendError(pageNumber, 500, "Failed to add route (duplicate or database error)");
      }
      return;
   }

   if (path.size() < 2)
   {
      sendMethodNotAllowed(pageNumber, method);
      return;
   }

   const Data& key = path[1];

   if (method == "PUT")
   {
      AbstractDb::RouteRecord rec = routeStore.getRouteRecord(key);
      // We don't have a reliable way to detect "not found" from getRouteRecord
      // (it returns a default-constructed record). A record with all fields
      // empty is treated as not found.
      if (rec.mMatchingPattern.empty() && rec.mRewriteExpression.empty() &&
          rec.mMethod.empty() && rec.mEvent.empty())
      {
         sendNotFound(pageNumber);
         return;
      }

      // Partial update
      Data uri         = hasParam(query, "uri")         ? param(query, "uri")         : rec.mMatchingPattern;
      Data destination = hasParam(query, "destination") ? param(query, "destination") : rec.mRewriteExpression;
      Data rmethod     = hasParam(query, "method")      ? param(query, "method")      : rec.mMethod;
      Data event       = hasParam(query, "event")       ? param(query, "event")       : rec.mEvent;
      int  order       = hasParam(query, "order")       ? param(query, "order").convertInt() : rec.mOrder;

      if (uri.empty() || destination.empty())
      {
         sendError(pageNumber, 400, "Route uri and destination must not be empty");
         return;
      }

      if (routeStore.updateRoute(key, rmethod, event, uri, destination, order))
      {
         AbstractDb::RouteRecord updated = routeStore.getRouteRecord(key);
         sendJson(pageNumber, 200, successEnvelope(routeToJson(updated, key)));
      }
      else
      {
         sendError(pageNumber, 500, "Failed to update route (database error)");
      }
      return;
   }

   if (method == "DELETE")
   {
      routeStore.eraseRoute(key);
      sendOk(pageNumber);
      return;
   }

   sendMethodNotAllowed(pageNumber, method);
}

// ---------------------------------------------------------------------------
// Filters (GET only)
// ---------------------------------------------------------------------------
void
RestAdmin::handleFilters(const Data& method,
                         const std::vector<Data>& path,
                         const ParamMap& /*query*/,
                         int pageNumber)
{
   FilterStore& filterStore = mWebAdmin.mStore.mFilterStore;

   if (method == "GET" && path.size() == 1)
   {
      json::Array arr;
      for (FilterStore::Key key = filterStore.getFirstKey();
           !key.empty();
           key = filterStore.getNextKey(key))
      {
         AbstractDb::FilterRecord rec = filterStore.getFilterRecord(key);
         Data actionStr("Accept");
         if (rec.mAction == FilterStore::Reject)       actionStr = "Reject";
         else if (rec.mAction == FilterStore::SQLQuery) actionStr = "SQLQuery";

         json::Object obj;
         obj["key"] = json::String(key.c_str());
         obj["cond1Header"] = json::String(rec.mCondition1Header.c_str());
         obj["cond1Regex"]  = json::String(rec.mCondition1Regex.c_str());
         obj["cond2Header"] = json::String(rec.mCondition2Header.c_str());
         obj["cond2Regex"]  = json::String(rec.mCondition2Regex.c_str());
         obj["method"]      = json::String(rec.mMethod.c_str());
         obj["event"]       = json::String(rec.mEvent.c_str());
         obj["action"]      = json::String(actionStr.c_str());
         obj["actionData"]  = json::String(rec.mActionData.c_str());
         obj["order"]       = json::Number(rec.mOrder);
         arr.Insert(obj);
      }
      sendJson(pageNumber, 200, successEnvelope(arr));
      return;
   }

   sendMethodNotAllowed(pageNumber, method);
}

// ---------------------------------------------------------------------------
// Settings / stack info / stack statistics / congestion
// ---------------------------------------------------------------------------
void
RestAdmin::handleSettings(const Data& method, int pageNumber)
{
   if (method != "GET")
   {
      sendMethodNotAllowed(pageNumber, method);
      return;
   }

   Data buffer;
   {
      DataStream strm(buffer);
      strm << mWebAdmin.mProxy.getConfig();
      strm.flush();
   }
   sendJson(pageNumber, 200, successEnvelope(json::String(buffer.c_str())));
}

void
RestAdmin::handleStackInfo(const Data& method, int pageNumber)
{
   if (method != "GET")
   {
      sendMethodNotAllowed(pageNumber, method);
      return;
   }

   Data buffer;
   {
      DataStream strm(buffer);
      mWebAdmin.mProxy.getStack().dump(strm);
      strm.flush();
   }
   sendJson(pageNumber, 200, successEnvelope(json::String(buffer.c_str())));
}

void
RestAdmin::handleStats(const Data& method,
                       const std::vector<Data>& path,
                       int pageNumber)
{
   // POST /api/v1/stats/reset -> zero out the stack's statistics counters.
   if (method == "POST" && path.size() == 2 && path[1] == "reset")
   {
      mWebAdmin.mProxy.getStack().zeroOutStatistics();
      sendOk(pageNumber);
      return;
   }

   // GET /api/v1/stats -> fresh stack statistics from the StatisticsManager.
   // This is async: pollStatistics() asks the stack to deliver a message, and
   // we wait (with a timeout) for ReproRunner to route that message to
   // WebAdmin::handleStatisticsMessage, which populates mStatsPayload.
   if (method == "GET" && path.size() == 1)
   {
      StatisticsMessage::Payload payload;
      bool gotPayload = false;

      {
         Lock lock(mWebAdmin.mStatsMutex);

         // Clear any previous payload so we wait for *this* request's poll.
         mWebAdmin.mStatsReady = false;

         if (!mWebAdmin.mProxy.getStack().pollStatistics())
         {
            sendError(pageNumber, 503,
                      "Statistics Manager is not enabled "
                      "(set StatisticsLogInterval in repro.config)");
            return;
         }

         // Wait up to 10 seconds for the stats message to arrive. Loop to
         // handle spurious wakeups; exit when either mStatsReady becomes
         // true or the deadline passes.
         const std::chrono::steady_clock::time_point deadline =
            std::chrono::steady_clock::now() + std::chrono::seconds(10);
         while (!mWebAdmin.mStatsReady)
         {
            std::chrono::steady_clock::duration remaining =
               deadline - std::chrono::steady_clock::now();
            if (remaining <= std::chrono::steady_clock::duration::zero())
            {
               break;
            }
            mWebAdmin.mStatsCondition.wait_for(lock, remaining);
         }

         if (mWebAdmin.mStatsReady)
         {
            // Copy the payload under the lock, then serialize to JSON
            // outside the critical section.
            payload = mWebAdmin.mStatsPayload;
            gotPayload = true;
         }
      }

      if (!gotPayload)
      {
         sendError(pageNumber, 504,
                   "Timed out waiting for stack statistics");
         return;
      }

      sendJson(pageNumber, 200, successEnvelope(payloadToJson(payload)));
      return;
   }

   sendMethodNotAllowed(pageNumber, method);
}

void
RestAdmin::handleCongestion(const Data& method, int pageNumber)
{
   if (method != "GET")
   {
      sendMethodNotAllowed(pageNumber, method);
      return;
   }

   if (!mWebAdmin.mProxy.getStack().getCongestionManager())
   {
      sendJson(pageNumber, 200, successEnvelope(json::Null()));
      return;
   }

   Data buffer;
   {
      DataStream strm(buffer);
      mWebAdmin.mProxy.getStack().getCongestionManager()->encodeCurrentState(strm);
      strm.flush();
   }
   sendJson(pageNumber, 200, successEnvelope(json::String(buffer.c_str())));
}

// ---------------------------------------------------------------------------
// Log level
// ---------------------------------------------------------------------------
void
RestAdmin::handleLogLevel(const Data& method,
                          const ParamMap& query,
                          int pageNumber)
{
   // Log::toString returns values like "LOG_INFO"; we strip the LOG_ prefix
   // so the API uses the same short names ("INFO", "DEBUG", etc.) that
   // Log::toLevel accepts as input.
   auto levelName = [](Log::Level l) -> Data {
      Data s = Log::toString(l);
      const Data prefix("LOG_");
      if (s.prefix(prefix))
      {
         return s.substr(prefix.size());
      }
      return s;
   };

   if (method == "GET")
   {
      json::Object obj;
      obj["level"] = json::String(levelName(Log::level()).c_str());
      sendJson(pageNumber, 200, successEnvelope(obj));
      return;
   }

   if (method == "PUT")
   {
      if (!hasParam(query, "level"))
      {
         sendError(pageNumber, 400, "Missing required parameter: level");
         return;
      }
      Data levelStr = param(query, "level");
      Log::Level l = Log::toLevel(levelStr);
      Log::setLevel(l);
      InfoLog(<< "Log level changed via REST API to: " << levelStr);

      json::Object obj;
      obj["level"] = json::String(levelName(Log::level()).c_str());
      sendJson(pageNumber, 200, successEnvelope(obj));
      return;
   }

   sendMethodNotAllowed(pageNumber, method);
}

// ---------------------------------------------------------------------------
// DNS cache
// ---------------------------------------------------------------------------
void
RestAdmin::handleDnsCache(const Data& method,
                          const std::vector<Data>& path,
                          int pageNumber)
{
   // GET /api/v1/dnscache -> retrieve cache dump
   if (method == "GET" && path.size() == 1)
   {
      Data cacheDump;
      {
         Lock lock(mWebAdmin.mDnsCacheMutex);
         mWebAdmin.mProxy.getStack().getDnsCacheDump(make_pair(0, 0), &mWebAdmin);
         // Blocks until onDnsCacheDumpRetrieved populates mDnsCache and
         // signals the condition.
         mWebAdmin.mDnsCacheCondition.wait(lock);
         cacheDump = mWebAdmin.mDnsCache;
      }
      sendJson(pageNumber, 200, successEnvelope(json::String(cacheDump.c_str())));
      return;
   }

   // DELETE /api/v1/dnscache -> clear
   if (method == "DELETE" && path.size() == 1)
   {
      mWebAdmin.mProxy.getStack().clearDnsCache();
      sendOk(pageNumber);
      return;
   }

   // POST /api/v1/dnscache/reload -> reload DNS servers (reread resolv.conf etc.)
   if (method == "POST" && path.size() == 2 && path[1] == "reload")
   {
      mWebAdmin.mProxy.getStack().reloadDnsServers();
      sendOk(pageNumber);
      return;
   }

   sendMethodNotAllowed(pageNumber, method);
}

// ---------------------------------------------------------------------------
// Restart
// ---------------------------------------------------------------------------
void
RestAdmin::handleRestart(const Data& method, int pageNumber)
{
   if (method != "POST")
   {
      sendMethodNotAllowed(pageNumber, method);
      return;
   }

   unsigned short port = mWebAdmin.mProxy.getConfig().getConfigUnsignedShort("CommandPort", 0);
   if (port == 0)
   {
      sendError(pageNumber, 503, "CommandServer must be running (CommandPort must be set) to use restart");
      return;
   }

   // Connect to local CommandPort and send the restart XML, same mechanism
   // as WebAdmin::buildRestartSubPage.
   const char* host = "127.0.0.1";
   struct addrinfo hints = {};
   struct addrinfo* res = nullptr;
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_STREAM;

   if (getaddrinfo(host, nullptr, &hints, &res) != 0 || res == nullptr)
   {
      sendError(pageNumber, 500, "Failed to resolve loopback for restart");
      return;
   }

   struct sockaddr_in servAddr = {};
   memcpy(&servAddr, res->ai_addr, res->ai_addrlen);
   servAddr.sin_port = htons(port);
   freeaddrinfo(res);

   int sd = (int)socket(AF_INET, SOCK_STREAM, 0);
   if (sd < 0)
   {
      sendError(pageNumber, 500, "Failed to create socket for restart");
      return;
   }

   struct sockaddr_in localAddr = {};
   localAddr.sin_family = AF_INET;
   localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
   localAddr.sin_port = 0;
   if (::bind(sd, (struct sockaddr*)&localAddr, sizeof(localAddr)) < 0)
   {
      closeSocket(sd);
      sendError(pageNumber, 500, "Failed to bind local socket for restart");
      return;
   }

   if (::connect(sd, (struct sockaddr*)&servAddr, sizeof(servAddr)) < 0)
   {
      closeSocket(sd);
      sendError(pageNumber, 500, "Failed to connect to CommandPort for restart");
      return;
   }

   Data request("<Restart>\r\n  <Request>\r\n  </Request>\r\n</Restart>\r\n");
   if (send(sd, request.c_str(), (int)request.size(), 0) < 0)
   {
      closeSocket(sd);
      sendError(pageNumber, 500, "Failed to send restart command");
      return;
   }

   closeSocket(sd);

   json::Object obj;
   obj["status"] = json::String("restarting");
   sendJson(pageNumber, 200, successEnvelope(obj));
}

// ---------------------------------------------------------------------------
// Certs reload
// ---------------------------------------------------------------------------
void
RestAdmin::handleCertsReload(const Data& method, int pageNumber)
{
   if (method != "POST")
   {
      sendMethodNotAllowed(pageNumber, method);
      return;
   }

#ifdef USE_SSL
   mWebAdmin.mProxy.getStack().reloadCertificates();
   sendOk(pageNumber);
#else
   sendError(pageNumber, 501, "Certificates not supported (built without SSL)");
#endif
}


/* ====================================================================

 Copyright (c) 2026 SIP Spectrum, Inc. https://www.sipspectrum.com
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are
 met:

 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 3. Neither the name of Plantronics nor the names of its contributors
    may be used to endorse or promote products derived from this
    software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
