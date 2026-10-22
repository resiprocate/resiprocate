#if !defined(RESIP_RESTADMIN_HXX)
#define RESIP_RESTADMIN_HXX

#include "rutil/Data.hxx"

#include <map>
#include <vector>

namespace resip
{
class DataStream;
}

namespace repro
{
class WebAdmin;

// RestAdmin provides a simple JSON/REST interface to repro, living alongside
// the existing HTML WebAdmin on the same HTTP listener. It does not inherit
// from HttpBase; instead it is owned by WebAdmin, which intercepts URIs with
// the /api/v1/ prefix and delegates to RestAdmin::dispatch.
//
// Authentication is performed by WebAdmin before dispatch is called; the
// authenticated user (empty if challenges are disabled) is passed through.
//
// Because we do not parse HTTP request bodies, all mutating endpoints take
// their parameters in the query string.
//
// Supported endpoints (see implementation for details):
//
//   GET    /api/v1/registrations
//   GET    /api/v1/publications
//   GET    /api/v1/domains
//   POST   /api/v1/domains?uri=...&tlsPort=...
//   DELETE /api/v1/domains/{uri}
//   GET    /api/v1/users
//   POST   /api/v1/users?user=...&domain=...&password=...&name=...&email=...
//   GET    /api/v1/users/{user@domain}
//   PUT    /api/v1/users/{user@domain}?user=...&domain=...&password=...&name=...&email=...
//          (user and domain may be changed, but password is required if either changes)
//   DELETE /api/v1/users/{user@domain}
//   GET    /api/v1/acls
//   POST   /api/v1/acls?hostOrIp=...&port=...&transport=...
//   DELETE /api/v1/acls/{key}
//   GET    /api/v1/routes
//   POST   /api/v1/routes?uri=...&destination=...&method=...&event=...&order=...
//   PUT    /api/v1/routes/{key}?uri=...&destination=...&method=...&event=...&order=...
//   DELETE /api/v1/routes/{key}
//   GET    /api/v1/filters
//   GET    /api/v1/settings
//   GET    /api/v1/stackinfo
//   GET    /api/v1/stats
//   POST   /api/v1/stats/reset
//   GET    /api/v1/congestion
//   GET    /api/v1/loglevel
//   PUT    /api/v1/loglevel?level=...
//   GET    /api/v1/dnscache
//   DELETE /api/v1/dnscache
//   POST   /api/v1/dnscache/reload
//   POST   /api/v1/restart
//   POST   /api/v1/certs/reload
class RestAdmin
{
public:
   explicit RestAdmin(WebAdmin& webAdmin);
   ~RestAdmin();

   // Main entrypoint. Called by WebAdmin after authentication succeeds.
   // method is the HTTP verb (e.g. "GET"); uri is the full request URI
   // (path + optional query string). pageNumber identifies the pending
   // HTTP connection to reply on. authenticatedUser is the username that
   // authenticated, or empty if challenges were disabled.
   void dispatch(const resip::Data& method,
                 const resip::Data& uri,
                 int pageNumber,
                 const resip::Data& authenticatedUser);

private:
   typedef std::map<resip::Data, resip::Data> ParamMap;

   // Split the URI into its path segments (after /api/v1/) and a map of
   // query parameters. Path segments are URL-decoded; query values are
   // URL-decoded.
   void parseRequest(const resip::Data& uri,
                     std::vector<resip::Data>& pathSegments,
                     ParamMap& queryParams) const;

   // Response helpers
   void sendJson(int pageNumber, int statusCode, const resip::Data& json);
   void sendOk(int pageNumber);
   void sendError(int pageNumber, int statusCode, const resip::Data& message);
   void sendMethodNotAllowed(int pageNumber, const resip::Data& method);
   void sendNotFound(int pageNumber);

   // Resource handlers
   void handleRegistrations(const resip::Data& method,
                            const std::vector<resip::Data>& path,
                            const ParamMap& query,
                            int pageNumber);
   void handlePublications(const resip::Data& method,
                           const std::vector<resip::Data>& path,
                           const ParamMap& query,
                           int pageNumber);
   void handleDomains(const resip::Data& method,
                      const std::vector<resip::Data>& path,
                      const ParamMap& query,
                      int pageNumber);
   void handleUsers(const resip::Data& method,
                    const std::vector<resip::Data>& path,
                    const ParamMap& query,
                    int pageNumber);
   void handleAcls(const resip::Data& method,
                   const std::vector<resip::Data>& path,
                   const ParamMap& query,
                   int pageNumber);
   void handleRoutes(const resip::Data& method,
                     const std::vector<resip::Data>& path,
                     const ParamMap& query,
                     int pageNumber);
   void handleFilters(const resip::Data& method,
                      const std::vector<resip::Data>& path,
                      const ParamMap& query,
                      int pageNumber);
   void handleSettings(const resip::Data& method, int pageNumber);
   void handleStackInfo(const resip::Data& method, int pageNumber);
   void handleStats(const resip::Data& method,
                    const std::vector<resip::Data>& path,
                    int pageNumber);
   void handleCongestion(const resip::Data& method, int pageNumber);
   void handleLogLevel(const resip::Data& method,
                       const ParamMap& query,
                       int pageNumber);
   void handleDnsCache(const resip::Data& method,
                       const std::vector<resip::Data>& path,
                       int pageNumber);
   void handleRestart(const resip::Data& method, int pageNumber);
   void handleCertsReload(const resip::Data& method, int pageNumber);

   // Utility: look up a query param, return defaultValue if not present
   static resip::Data param(const ParamMap& q,
                            const resip::Data& key,
                            const resip::Data& defaultValue = resip::Data::Empty);
   static bool hasParam(const ParamMap& q, const resip::Data& key);

   // Access to WebAdmin internals (registration/publication dbs, store, proxy,
   // DNS cache machinery) via the friend relationship declared in WebAdmin.
   WebAdmin& mWebAdmin;
};

}

#endif


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
