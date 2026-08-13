#include "rutil/ResipAssert.h"
#include <fstream>
#include <iostream>
#include <string>
#include <time.h>

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "resip/dum/RegistrationPersistenceManager.hxx"
#include "resip/dum/PublicationPersistenceManager.hxx"
#include "resip/stack/StatisticsMessage.hxx"
#include "resip/stack/Symbols.hxx"
#include "resip/stack/Tuple.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/GenericPidfContents.hxx"
#include "rutil/Data.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Lock.hxx"
#include "rutil/Logger.hxx"
#include "rutil/DigestStream.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/Socket.hxx"
#include "rutil/Timer.hxx"
#include "rutil/TransportType.hxx"

#include "repro/ReproVersion.hxx"
#include "repro/Proxy.hxx"
#include "repro/HttpBase.hxx"
#include "repro/HttpConnection.hxx"
#include "repro/WebAdmin.hxx"
#include "repro/RestAdmin.hxx"
#include "repro/RouteStore.hxx"
#include "repro/UserStore.hxx"
#include "repro/FilterStore.hxx"
#include "repro/Store.hxx"

#ifdef USE_SSL
#include "resip/stack/ssl/Security.hxx"
#endif

using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

// Presentation is driven entirely by the stylesheet in webadmin/pageOutlinePre.html.
// "form-table" is the borderless label/field layout, "data-table" is the bordered
// record listing.
#define REPRO_BORDERLESS_TABLE_PROPS " class=\"form-table\""
#define REPRO_BORDERED_TABLE_PROPS " class=\"data-table\""

// Must match the default ReproRunner uses when deciding whether to start the
// CommandServer (see ReproRunner.cxx).  These drifted apart: WebAdmin defaulted
// to 0, so on any install that left CommandPort unset -- which is the shipped
// repro.config, where the line is commented out -- the CommandServer was running
// on 5081 but the web UI hid the Restart Proxy button and refused the restart.
#define REPRO_DEFAULT_COMMAND_PORT 5081

// The repro product mark: a request arriving at the proxy and being forked onward.
// Kept in sync with the copy in webadmin/pageOutlinePre.html.
static const char*
productMarkSvg()
{
   return
      "<svg class=\"logo\" viewBox=\"0 0 32 32\" role=\"img\" aria-label=\"Repro\">"
      "<rect x=\"0.9\" y=\"0.9\" width=\"30.2\" height=\"30.2\" rx=\"6.5\""
      " fill=\"var(--mark-navy)\" stroke=\"var(--mark-edge)\" stroke-width=\"1.6\"/>"
      "<g stroke=\"var(--mark-amber)\" stroke-width=\"2.2\" fill=\"none\" stroke-linecap=\"round\">"
      "<path d=\"M8 16h7\"/><path d=\"M15 16l8-6.5\"/><path d=\"M15 16l8 6.5\"/></g>"
      "<circle cx=\"8\" cy=\"16\" r=\"3\" fill=\"var(--mark-amber)\"/>"
      "<circle cx=\"23\" cy=\"9.5\" r=\"3\" fill=\"#ffb454\"/>"
      "<circle cx=\"23\" cy=\"22.5\" r=\"3\" fill=\"#ffb454\"/>"
      "</svg>";
}

// The same mark as an inline data: URI, for the browser tab icon.  '#' has to be
// percent encoded (%23) or it would terminate the URI at the first colour.
static const char*
faviconLink()
{
   return
      "<link rel=\"icon\" href=\"data:image/svg+xml,"
      "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 32 32'>"
      "<rect width='32' height='32' rx='7' fill='%2312294d'/>"
      "<g stroke='%23ff8d09' stroke-width='2.4' fill='none' stroke-linecap='round'>"
      "<path d='M8 16h7'/><path d='M15 16l8-6.5'/><path d='M15 16l8 6.5'/></g>"
      "<circle cx='8' cy='16' r='3' fill='%23ff8d09'/>"
      "<circle cx='23' cy='9.5' r='3' fill='%23ffb454'/>"
      "<circle cx='23' cy='22.5' r='3' fill='%23ffb454'/>"
      "</svg>\" />\n";
}

// Stylesheet for the pages that are served outside of the admin page outline
// (the login page and the per-user page).  It uses the same palette as
// webadmin/pageOutlinePre.html so that every repro page looks like one product.
static const char*
standaloneStyle()
{
   return
      "<style>\n"
      "  :root { --bg:#0a0c0f; --bg2:#10141a; --border:#1e2730; --green:#00e676;\n"
      "          --blue:#40c4ff; --text:#c8d0da; --text-dim:#4a5568; --text-mid:#7a8899;\n"
      "          --mark-navy:#12294d; --mark-edge:#2f5da8; --mark-amber:#ff8d09;\n"
      "          --mono:'IBM Plex Mono',ui-monospace,SFMono-Regular,Menlo,Consolas,monospace;\n"
      "          color-scheme:dark; }\n"
      "  *,*::before,*::after { box-sizing:border-box; margin:0; padding:0; }\n"
      "  body { background:var(--bg); color:var(--text); font-family:var(--mono); font-size:13px;\n"
      "         line-height:1.55; min-height:100vh; display:flex; align-items:center;\n"
      "         justify-content:center; padding:24px; }\n"
      "  .card { background:var(--bg2); border:1px solid var(--border); border-radius:3px;\n"
      "          padding:26px 28px; max-width:520px; width:100%; }\n"
      "  .brand { display:flex; align-items:center; gap:12px; }\n"
      "  .logo { width:38px; height:38px; flex-shrink:0; display:block; }\n"
      "  .name { font-size:16px; font-weight:600; letter-spacing:.12em; text-transform:uppercase; }\n"
      "  .sub { font-size:10px; letter-spacing:.14em; text-transform:uppercase;\n"
      "         color:var(--text-dim); margin:6px 0 20px; }\n"
      "  .cta { display:inline-block; padding:8px 20px; border:1px solid var(--green);\n"
      "         border-radius:2px; color:var(--green); text-decoration:none; font-size:11px;\n"
      "         font-weight:600; letter-spacing:.12em; text-transform:uppercase;\n"
      "         transition:all .15s; }\n"
      "  .cta:hover { background:var(--green); color:var(--bg); }\n"
      "  p { color:var(--text-mid); margin-top:18px; font-size:12px; }\n"
      "  a { color:var(--blue); text-decoration:none; }\n"
      "  a:hover { text-decoration:underline; }\n"
      "</style>\n";
}

WebAdmin::RemoveKey::RemoveKey(const Data &key1, const Data &key2) : mKey1(key1), mKey2(key2)
{
}

bool
WebAdmin::RemoveKey::operator<(const RemoveKey& rhs) const
{
   if(mKey1 < rhs.mKey1) 
   {
      return true;
   }
   else if(mKey1 == rhs.mKey1 && mKey2 < rhs.mKey2) 
   { 
      return true; 
   }
   else 
   {
      return false;
   }
}

WebAdmin::WebAdmin(Proxy& proxy,
                   RegistrationPersistenceManager& regDb,
                   PublicationPersistenceManager& pubDb,
                   const Data& realm, // this realm is used for http challenges                
                   int port,
                   IpVersion version,
                   const Data& ipAddr):
   HttpBase(port, version, realm, ipAddr),
   mProxy(proxy),
   mStore(*mProxy.getConfig().getDataStore()),
   mRegDb(regDb),
   mPubDb(pubDb),
   mNoWebChallenges(proxy.getConfig().getConfigBool("DisableHttpAuth", false)),
   mPageOutlinePre(
#include "repro/webadmin/pageOutlinePre.ixx"
   ),
   mPageOutlinePost(
#include "repro/webadmin/pageOutlinePost.ixx"
   ),
   mUserFile(proxy.getConfig().getConfigData("HttpAdminUserFile", "users.txt")),
   mStatsReady(false)
{
   // Place repro version into PageOutlinePre
   mPageOutlinePre.replace("VERSION", VersionUtils::instance().releaseVersion().c_str());

   parseUserFile();

   // Construct the REST API handler now that all members are initialized.
   mRestAdmin.reset(new RestAdmin(*this));
}

WebAdmin::~WebAdmin()
{
   // mRestAdmin is destroyed here. Defined out-of-line so that RestAdmin is
   // a complete type at the point where unique_ptr invokes its deleter,
   // which avoids the "invalid application of 'sizeof' to incomplete type"
   // error from GCC's unique_ptr implementation.
}

/**
 * Load web admin users from a file.
 *
 * We use the same type of file that is generated by Apache's htdigest utility
 *
 *    username:realm:HA1
 *
 * where HA1 can be created by:
 *
 *    echo -n 'user:realm:password' | md5sum
 */
void
WebAdmin::parseUserFile()
{
   InfoLog(<< "Trying to load web admin users from: " << mUserFile);
   std::ifstream users(mUserFile.c_str());
   std::string sline;
   int lineNbr = 0;
   if(!users)
   {
      throw ConfigException("Error opening/reading user database file", __FILE__, __LINE__);
   }
   std::map<Data,Data> userMap;

   while(std::getline(users, sline))
   {
      Data username;
      Data realm;
      Data ha1;

      Data line(sline);
      ParseBuffer pb(line);
      lineNbr++;

      // Jump over empty lines.
      if(line.size() == 0)
      {
          continue;
      }

      pb.skipWhitespace();
      if(!pb.eof() && *pb.position() == '#')
      {
         // Line is commented out, skip it
         continue;
      }

      const char * anchor = pb.position();

      pb.skipToOneOf(" :");

      if (pb.eof())
      {
         ErrLog(<< "Missing or invalid credentials at line " << lineNbr);
         continue;
      }

      pb.data(username, anchor);

      pb.skipToChar(':');
      if (!pb.eof())
      {
         pb.skipChar(':');
         pb.skipWhitespace();
      }

      anchor = pb.position();
      pb.skipToOneOf(" :");

      if (pb.eof())
      {
         ErrLog(<< "Missing or invalid credentials at line " << lineNbr);
         continue;
      }

      pb.data(realm, anchor);

      pb.skipToChar(':');
      if (!pb.eof())
      {
         pb.skipChar(':');
         pb.skipWhitespace();
      }

      anchor = pb.position();
      // Tolerate tab padding, and include the EOL chars: getline only strips
      // the LF, so a file with CRLF line endings would otherwise leave a
      // trailing CR on this last field.
      pb.skipToOneOf(" \t:\r\n");

      pb.data(ha1, anchor);

      // We can only authenticate users in the realm we challenge for
      if(realm != mRealm)
      {
         DebugLog(<< "Ignoring user " << username << " for realm " << realm);
         continue;
      }

      userMap[username] = ha1;
   }
   users.close();
   InfoLog(<< "Processed " << userMap.size() << " user(s) from " << lineNbr << " line(s) in " << mUserFile);
   mUsers = userMap;
}

void 
WebAdmin::buildPage( const Data& method,
                     const Data& uri,
                     int pageNumber, 
                     const resip::Data& pUser,
                     const resip::Data& pPassword )
{
   ParseBuffer pb(uri);
   
   DebugLog (<< "Parsing URL" << uri );

   const char* anchor = pb.skipChar('/');
   pb.skipToChar('?');
   Data pageName;
   pb.data(pageName,anchor);
   
   DebugLog (<< "  got page name: " << pageName );

   // Dispatch any URI starting with /api/v1 to the REST handler.
   // Authentication still happens below; the REST handler is invoked only
   // after the user has been authenticated (or challenges disabled).
   bool isRestRequest = uri.prefix("/api/v1");

   // if this is not a valid page, redirect it
   if ( !isRestRequest &&
      ( pageName != Data("index.html") ) && 
      ( pageName != Data("input") ) && 
      ( pageName != Data("cert.cer") ) && 
      ( ! pageName.prefix("cert") ) && 
      ( pageName != Data("userTest.html") ) && 
      ( pageName != Data("domains.html")  ) &&
      ( pageName != Data("acls.html")  ) &&
      ( pageName != Data("addUser.html") ) && 
      ( pageName != Data("editUser.html") ) &&
      ( pageName != Data("showUsers.html")  ) &&
      ( pageName != Data("addFilter.html") ) && 
      ( pageName != Data("editFilter.html") ) &&
      ( pageName != Data("showFilters.html") )&& 
      ( pageName != Data("addRoute.html") ) && 
      ( pageName != Data("editRoute.html") ) &&
      ( pageName != Data("showRoutes.html") )&& 
      ( pageName != Data("registrations.html") ) &&  
      ( pageName != Data("publications.html")) &&
      ( pageName != Data("settings.html")) &&
      ( pageName != Data("restart.html") ) &&  
      ( pageName != Data("logLevel.html") ) &&
      ( pageName != Data("reloadcerts.html") ) &&
      ( pageName != Data("user.html")  ) )
   { 
      setPage( resip::Data::Empty, pageNumber, 301 );
      return; 
   }
   
   // pages anyone can use 
   if ( pageName == Data("index.html") ) 
   {
      setPage( buildDefaultPage(), pageNumber, 200); 
      return;
   }

   // certificate pages 
   if ( pageName.prefix("cert") || pageName == Data("cert.cer") )
   {
#ifdef USE_SSL
      Data domain = mRealm;
      try 
      {
         const char* anchor = pb.skipChar('?');
         pb.skipToChar('=');
         Data query;
         pb.data(query, anchor);
         InfoLog( << "query is " << query );
         if ( query == "domain" ) 
         {
           anchor = pb.skipChar('=');
           pb.skipToEnd();
           pb.data(domain, anchor);
         }
      }
      catch (ParseException& )
      {
      }

      if ( !domain.empty() )
      {
         InfoLog( << "domain is " << domain );
         try
         {
            setPage( buildCertPage(domain), pageNumber, 200, Mime("application","pkix-cert") );
         }
         catch(BaseSecurity::Exception&)
         {
            setPage( resip::Data::Empty, pageNumber, 404 );
         }
         return;
      }
      else
      {
         setPage( resip::Data::Empty, pageNumber, 404 );
         return;
      }
#else
      // ?bwc? Probably could use a better indication?
      setPage(resip::Data::Empty, pageNumber, 404);
#endif
   }
  
   Data authenticatedUser;
   if (mNoWebChallenges)
   {
      // do't do authentication - give everyone admin privilages
      authenticatedUser = Data("admin");
   }
   else
   {
      // TODO !cj! - this code is broken - the user name in the web digest should be
      // moved to alice@example.com instead of alice and assuming the realm is
      // empty

      // all pages after this, user must authenticate  
      if ( pUser.empty() )
      {  
         setPage( resip::Data::Empty, pageNumber,401 );
         return;
      }
      
      // check that authentication is correct 
      Data dbA1;
      std::map<Data,Data>::iterator it = mUsers.find(pUser);
      if(it != mUsers.end())
      {
         dbA1 = it->second;
      }
      
      if ( !dbA1.empty() )
      {
         DigestStream a1;
         a1 << pUser // username
            << Symbols::COLON
            << mRealm // realm
            << Symbols::COLON
            << pPassword;
         Data compA1 = a1.getHex();
         
         if ( dbA1 == compA1 )
         {
            authenticatedUser = pUser;
         }
         else
         {
            InfoLog(  << "user " << pUser << " failed to authenticate to web server" );
            DebugLog( << " compA1="<<compA1<< " dbA1="<<dbA1 );
            setPage( resip::Data::Empty, pageNumber,401 );
            return;
         }
      }
      else //No A1, so we must assume this user does not exist.
      {
         setPage( "User does not exist.", pageNumber,401 );
         return;         
      }
   }

   // If this is a REST API request, dispatch to the REST handler.
   // The REST handler is responsible for parsing query parameters and
   // generating a JSON response.
   if ( isRestRequest )
   {
      resip_assert( mRestAdmin );
      mRestAdmin->dispatch(method, uri, pageNumber, authenticatedUser);
      return;
   }
      
   // parse any URI tags from form entry
   mRemoveSet.clear();
   mHttpParams.clear();

   if (!pb.eof())
   {
      pb.skipChar('?');
           
      while ( !pb.eof() )
      {
         const char* anchor1 = pb.position();
         pb.skipToChar('=');
         Data key;
         pb.data(key,anchor1);
 
         const char* anchor2 = pb.skipChar('=');
         pb.skipToChar('&');
         Data value;
         pb.data(value,anchor2); 
           
         if ( !pb.eof() )
         {
            pb.skipChar('&');
         }
           
         if ( key.prefix("remove.") )  // special case of parameters to delete one or more records
         {
            Data tmp = key.substr(7);  // the ID is everything after the dot
            if (!tmp.empty())
            {
               DebugLog (<< "  remove key=" << tmp.urlDecoded());
               mRemoveSet.insert(RemoveKey(tmp.urlDecoded(),value.urlDecoded()));   // add to the set of records to remove
            }
         }
         else if ( !key.empty() && !value.empty() ) // make sure both exist
         {
            DebugLog (<< "  key=" << key << " value=" << value << " & unencoded form: " << value.urlDecoded() );
            mHttpParams[key] = value.urlDecoded();  // add other parameters to the Map
         }
      }
   }
   
   DebugLog( << "building page for user=" << authenticatedUser  );

   Data page;
   if ( authenticatedUser == Data("admin") )
   {
      DataStream s(page);
      s << mPageOutlinePre;
      
      // admin only pages 
      if ( pageName == Data("user.html")    ) {}; /* do nothing */ 
      //if ( pageName == Data("input")    ) ; /* do nothing */ 
      if ( pageName == Data("domains.html")    ) buildDomainsSubPage(s);
      if ( pageName == Data("acls.html")       ) buildAclsSubPage(s);
      
      if ( pageName == Data("addUser.html")    ) buildAddUserSubPage(s);
      if ( pageName == Data("editUser.html")   ) buildEditUserSubPage(s);
      if ( pageName == Data("showUsers.html")  ) buildShowUsersSubPage(s);
      
      if ( pageName == Data("addFilter.html")   ) buildAddFilterSubPage(s);
      if ( pageName == Data("editFilter.html")  ) buildEditFilterSubPage(s);
      if ( pageName == Data("showFilters.html") ) buildShowFiltersSubPage(s);

      if ( pageName == Data("addRoute.html")   ) buildAddRouteSubPage(s);
      if ( pageName == Data("editRoute.html")  ) buildEditRouteSubPage(s);
      if ( pageName == Data("showRoutes.html") ) buildShowRoutesSubPage(s);
      
      if ( pageName == Data("registrations.html")) buildRegistrationsSubPage(s);
      if ( pageName == Data("publications.html"))  buildPublicationsSubPage(s);
      if ( pageName == Data("settings.html"))    buildSettingsSubPage(s);
      if ( pageName == Data("restart.html"))     buildRestartSubPage(s);
      if ( pageName == Data("logLevel.html"))    buildLogLevelSubPage(s);
      if ( pageName == Data("reloadcerts.html")) buildReloadCertsSubPage(s);
      
      s << mPageOutlinePost;
      s.flush();

      if ( pageName == Data("userTest.html")   ) page=buildUserPage();
   }
   else if ( !authenticatedUser.empty() )
   {
      // user only pages 
      if ( pageName == Data("user.html") ) page=buildUserPage(); 
      //if ( pageName == Data("input") ) page=buildUserPage();

      if(page.empty())
      {
         DataStream s(page);
         s << "Invalid page request";
         s.flush();
         setPage(page, pageNumber, 500);
         return;
      }
   }
   
   resip_assert( !authenticatedUser.empty() );
   resip_assert( !page.empty() );
   
   setPage( page, pageNumber,200 );
}

void
WebAdmin::buildDomainsSubPage(DataStream& s)
{ 
   Data domainUri;

  if (!mRemoveSet.empty() && (mHttpParams["action"] == "Remove"))
   {
      int j = 0;
      for (set<RemoveKey>::iterator i = mRemoveSet.begin(); i != mRemoveSet.end(); ++i)
      {
         mStore.mConfigStore.eraseDomain(i->mKey1);
         ++j;
      }
      s << "<p class=\"notice ok\"><em>Removed:</em> " << j << " records</p>" << endl;
   }
   
   Dictionary::iterator pos = mHttpParams.find("domainUri");
   if (pos != mHttpParams.end() && (mHttpParams["action"] == "Add")) // found domainUri key
   {
      domainUri = pos->second;
      // The per-domain TLS port is dead weight: nothing in repro reads it back
      // (ConfigStore::getTlsPort has no callers), so the field is no longer
      // offered in the UI and every record is written with 0.  The column stays
      // in the database record for schema compatibility.
      if(mStore.mConfigStore.addDomain(domainUri, 0))
      {
         s << "<p class=\"notice ok\"><em>Added</em> domain: " << domainUri << "</p>" << endl;
      }
      else
      {
         s << "<p class=\"notice err\"><em>Error</em> adding domain: likely database error (check logs).</p>\n";
      }
   }   

   s <<
      "     <h2>Domains</h2>" << endl <<
      "     <p>Requests addressed to these domains are treated as locally served, rather than" << endl <<
      "     being proxied onwards to another host.</p>" << endl <<
      "     <form id=\"domainForm\" method=\"get\" action=\"domains.html\" name=\"domainForm\">" << endl <<
      "        <table" REPRO_BORDERLESS_TABLE_PROPS ">" << endl <<
      "          <tr>" << endl <<
      "            <td align=\"right\">New Domain:</td>" << endl <<
      "            <td><input type=\"text\" name=\"domainUri\" size=\"24\" placeholder=\"example.com\"/></td>" << endl <<
      "            <td><input type=\"submit\" name=\"action\" value=\"Add\"/></td>" << endl <<
      "          </tr>" << endl <<
      "        </table>" << endl <<
      "      <div class=space>" << endl <<
      "        <br>" << endl <<
      "      </div>" << endl <<
      "      <table" REPRO_BORDERED_TABLE_PROPS ">" << endl <<
      "        <thead>" << endl <<
      "          <tr>" << endl <<
      "            <td>Domain</td>" << endl <<
      "            <td><input type=\"submit\" name=\"action\" value=\"Remove\"/></td>" << endl <<
      "          </tr>" << endl <<
      "        </thead>" << endl <<
      "        <tbody>" << endl;
   
   const ConfigStore::ConfigData& configs = mStore.mConfigStore.getConfigs();
   for ( ConfigStore::ConfigData::const_iterator i = configs.begin();
        i != configs.end(); i++ )
   {
      s << 
         "          <tr>" << endl <<
         "            <td>" << i->second.mDomain << "</td>" << endl <<
         "            <td><input type=\"checkbox\" name=\"remove." << i->second.mDomain << "\"/></td>" << endl <<
         "          </tr>" << endl;
   }
   
   s <<
      "        </tbody>" << endl <<
      "      </table>" << endl <<
      "     </form>" << endl <<
      "<p class=\"notice warn\"><em>WARNING:</em>  You must restart repro after adding domains.</p>" << endl;
}

void
WebAdmin::buildAclsSubPage(DataStream& s)
{ 
   if (!mRemoveSet.empty() && (mHttpParams["action"] == "Remove"))
   {
      int j = 0;
      for (set<RemoveKey>::iterator i = mRemoveSet.begin(); i != mRemoveSet.end(); ++i)
      {
         mStore.mAclStore.eraseAcl(i->mKey1);
         ++j;
      }
      s << "<p class=\"notice ok\"><em>Removed:</em> " << j << " records</p>" << endl;
   }
   
   Dictionary::iterator pos = mHttpParams.find("aclUri");
   if (pos != mHttpParams.end() && (mHttpParams["action"] == "Add")) // found 
   {
      Data hostOrIp = mHttpParams["aclUri"];
      int port = mHttpParams["aclPort"].convertInt();
      TransportType transport = Tuple::toTransport(mHttpParams["aclTransport"]);
      
      if (mStore.mAclStore.addAcl(hostOrIp, port, transport))
      {
         s << "<p class=\"notice ok\"><em>Added</em> trusted access for: " << hostOrIp << "</p>\n";
      }
      else 
      {
         s << "<p class=\"notice err\"><em>Error</em> parsing: " << hostOrIp << "</p>\n";
      }
   }   
   
   s << 
      "     <h2>ACLs</h2>" << endl <<
      "     <div class=\"help\">" << endl <<
      "       <p>Access lists are used as a whitelist to allow gateways and other trusted" << endl <<
      "       nodes to skip authentication.</p>" << endl <<
      "       <p>If a hostname or FQDN is used then a TLS transport type is assumed.  All" << endl <<
      "       other transport types must specify ACLs by address.  Leave Port empty to match" << endl <<
      "       any port.</p>" << endl <<
      "       <pre>Host or IP can be in any of these formats" << endl <<
      "  localhost         localhost  (becomes 127.0.0.1/8, ::1/128 and fe80::1/64)" << endl <<
      "  bare hostname     server1" << endl <<
      "  FQDN              server1.example.com" << endl <<
      "  IPv4 address      192.168.1.100" << endl <<
      "  IPv4 + mask       192.168.1.0/24" << endl <<
      "  IPv6 address      ::341:0:23:4bb:0011:2435:abcd" << endl <<
      "  IPv6 + mask       ::341:0:23:4bb:0011:2435:abcd/80" << endl <<
      "  IPv6 reference    [::341:0:23:4bb:0011:2435:abcd]" << endl <<
      "  IPv6 ref + mask   [::341:0:23:4bb:0011:2435:abcd]/64</pre>" << endl <<
      "     </div>" << endl <<
      "      <form id=\"aclsForm\" method=\"get\" action=\"acls.html\" name=\"aclsForm\">" << endl <<
      "        <table" REPRO_BORDERLESS_TABLE_PROPS ">" << endl <<
      "          <tr>" << endl <<
      "            <td align=\"right\">Host or IP:</td>" << endl <<
      "            <td><input type=\"text\" name=\"aclUri\" size=\"24\" placeholder=\"host, FQDN or IP/mask\"/></td>" << endl <<
      // An empty port box submits nothing, and convertInt() of a missing param is
      // 0, which is what "any port" means here -- so the box can be left blank.
      "            <td><input type=\"text\" name=\"aclPort\" size=\"10\" placeholder=\"Port\"/></td>" << endl <<
      "            <td><select name=\"aclTransport\">" << endl <<
      "                <option selected=\"selected\">UDP</option>" << endl <<
      "                <option>TCP</option>" << endl <<
#ifdef USE_SSL
      "                <option>TLS</option>" << endl <<
#endif
#ifdef USE_DTLS
      "                <option>DTLS</option>" << endl <<
#endif
      "            </select></td>" << endl <<
      "            <td><input type=\"submit\" name=\"action\" value=\"Add\"/></td>" << endl <<
      "          </tr>" << endl <<
      "        </table>" << endl <<
      "      <div class=space>" << endl <<
      "        <br>" << endl <<
      "      </div>" << endl <<
      "      <table" REPRO_BORDERED_TABLE_PROPS ">" << endl <<
      "        <thead>" << endl <<
      "          <tr>" << endl <<
      "            <td>Host Address or Peer Name</td>" << endl <<
      "            <td>Port</td>" << endl <<
      "            <td>Transport</td>" << endl <<
      "            <td><input type=\"submit\" name=\"action\" value=\"Remove\"/></td>" << endl <<
      "          </tr>" << endl <<
      "        </thead>" << endl <<
      "        <tbody>" << endl;
   
   AclStore::Key key = mStore.mAclStore.getFirstTlsPeerNameKey();
   while (key != Data::Empty)
   {
      s << 
         "          <tr>" << endl << 
         "            <td colspan=\"2\">" << mStore.mAclStore.getTlsPeerName(key) << "</td>" << endl <<
         "            <td>TLS auth</td>" << endl <<
         "            <td><input type=\"checkbox\" name=\"remove." << key << "\"/></td>" << endl <<
         "</tr>" << endl;
         
      key = mStore.mAclStore.getNextTlsPeerNameKey(key);
   }
   key = mStore.mAclStore.getFirstAddressKey();
   while (key != Data::Empty)
   {
      s <<
         "          <tr>" << endl << 
         "            <td>" << mStore.mAclStore.getAddressTuple(key).presentationFormat() << "/"
                            << mStore.mAclStore.getAddressMask(key) << "</td>" << endl <<
         "            <td>" << mStore.mAclStore.getAddressTuple(key).getPort() << "</td>" << endl <<
         "            <td>" << Tuple::toData(mStore.mAclStore.getAddressTuple(key).getType()) << "</td>" << endl <<
         "            <td><input type=\"checkbox\" name=\"remove." << key << "\"/></td>" << endl <<
         "          </tr>" << endl;
      key = mStore.mAclStore.getNextAddressKey(key);
   }
   
   s <<
      "        </tbody>" << endl <<
      "      </table>" << endl <<
      "     </form>" << endl;
}

void
WebAdmin::buildAddUserSubPage(DataStream& s)
{
   Dictionary::iterator pos;
   Data user;
   
   pos = mHttpParams.find("user");
   if (pos != mHttpParams.end()) // found user key
   {
      user = pos->second;
      Data domain = mHttpParams["domain"];
      
//      pos = mHttpParams.find("realm");
//      if (pos == mHttpParams.end())
//      {
//         realm = mHttpParams["domain"];
//      }
            
      if(mStore.mUserStore.addUser(user,domain,domain,mHttpParams["password"],true,mHttpParams["name"],mHttpParams["email"]))
      {
         s << "<p class=\"notice ok\"><em>Added:</em> " << user << "@" << domain << "</p>\n";
      }
      else
      {
         s << "<p class=\"notice err\"><em>Error</em> adding user: likely database error (check logs).</p>\n";
      }
   }

      s << 
         "<h2>Add User</h2>" << endl <<
         "<div class=\"help\">" << endl <<
         "  <p>Full Name and Email are recorded for display purposes only.  They are not used" << endl <<
         "  for authentication, routing, or as a SIP identity.</p>" << endl <<
         "</div>" << endl <<
         "<form id=\"addUserForm\" action=\"addUser.html\"  method=\"get\" name=\"addUserForm\" enctype=\"application/x-www-form-urlencoded\">" << endl <<
         "<table" REPRO_BORDERLESS_TABLE_PROPS ">" << endl << 
         "<tr>" << endl << 
         "  <td align=\"right\" valign=\"middle\">User Name:</td>" << endl << 
         "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"user\" size=\"40\" placeholder=\"alice\"/></td>" << endl << 
         "</tr>" << endl << 

         //"<tr>" << endl << 
         //"<td align=\"right\" valign=\"middle\" >Realm:</td>" << endl << 
         //"<td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"realm\" size=\"40\"/></td>" << endl << 
         //"</tr>" << endl << 

         "<tr>" << endl << 
         "  <td align=\"right\" valign=\"middle\" >Domain:</td>" << endl << 
         "  <td align=\"left\" valign=\"middle\"><select name=\"domain\">" << endl
         ; 
         
         // for each domain, add an option in the pulldown         
         const ConfigStore::ConfigData& list = mStore.mConfigStore.getConfigs();
         for ( ConfigStore::ConfigData::const_iterator i = list.begin();
              i != list.end(); i++ )
         {
            s << "            <option";
            
            // if i->Domain is the default domain
            // {
            //    s << " selected=\"true\""; 
            // }
            
            s << ">" << i->second.mDomain << "</option>" << endl;
         }

         s <<
         "</select></td></tr>" << endl <<
         "<tr>" << endl << 
         "  <td align=\"right\" valign=\"middle\" >Password:</td>" << endl << 
         "  <td align=\"left\" valign=\"middle\"><input type=\"password\" name=\"password\" size=\"40\" placeholder=\"account password\"/></td>" << endl << 
         "</tr>" << endl << 

         "<tr>" << endl << 
         "  <td align=\"right\" valign=\"middle\" >Full Name:</td>" << endl << 
         "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"name\" size=\"40\" placeholder=\"Alice Smith\"/></td>" << endl << 
         "</tr>" << endl << 

         "<tr>" << endl << 
         "  <td align=\"right\" valign=\"middle\" >Email:</td>" << endl << 
         "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"email\" size=\"40\" placeholder=\"alice@example.com\"/></td>" << endl << 
         "</tr>" << endl << 

         "<tr>" << endl << 
         "  <td colspan=\"2\" align=\"right\" valign=\"middle\">" << endl << 
         "    <input type=\"reset\" value=\"Cancel\"/>" << endl <<
         "    <input type=\"submit\" name=\"submit\" value=\"Add\"/>" << endl <<
         "  </td>" << endl <<
         "</tr>" << endl <<

         "</table>" << endl <<
         "</form>" << endl
         ;
}

void
WebAdmin::buildEditUserSubPage(DataStream& s)
{
   Dictionary::iterator pos;
   pos = mHttpParams.find("key");
   if (pos != mHttpParams.end()) 
   {
      Data key = pos->second;
      AbstractDb::UserRecord rec = mStore.mUserStore.getUserInfo(key);
      // !rwm! TODO check to see if we actually found a record corresponding to the key.  how do we do that?
      
      s << "<h2>Edit User</h2>" << endl <<
           "<div class=\"help\">" << endl <<
           "  <p>Editing record with key: " << key << "</p>" << endl <<
           "  <p>If you leave the password field empty, the user's current "
           "password will not be reset.  However, if you change the username or "
           "domain, a new password is required (the stored password hash is "
           "bound to user+realm, so renaming without a new password would "
           "invalidate the account).</p>" << endl <<
           "  <p>Full Name and Email are recorded for display purposes only.  They are not used "
           "for authentication, routing, or as a SIP identity.</p>" << endl <<
           "</div>" << endl;
      
      s << 
         "<form id=\"editUserForm\" action=\"showUsers.html\"  method=\"get\" name=\"editUserForm\" enctype=\"application/x-www-form-urlencoded\">" << endl << 
         "<table" REPRO_BORDERLESS_TABLE_PROPS ">" << endl << 
         "<input type=\"hidden\" name=\"key\" value=\"" << key << "\"/>" << endl << 
         "<tr>" << endl << 
         "  <td align=\"right\" valign=\"middle\">User Name:</td>" << endl << 
         "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"user\" placeholder=\"alice\" value=\"" << rec.user << "\" size=\"40\"/></td>" << endl << 
         "</tr>" << endl << 
         
         //"<tr>" << endl << 
         //"<td align=\"right\" valign=\"middle\" >Realm:</td>" << endl << 
         //"<td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"realm\" size=\"40\"/></td>" << endl << 
         //"</tr>" << endl << 

         "<tr>" << endl << 
         "  <td align=\"right\" valign=\"middle\" >Domain:</td>" << endl << 
         "  <td align=\"left\" valign=\"middle\"><select name=\"domain\">" << endl
         ; 
      
      // for each domain, add an option in the pulldown      
      const ConfigStore::ConfigData& list = mStore.mConfigStore.getConfigs();      
      for ( ConfigStore::ConfigData::const_iterator i = list.begin();
            i != list.end(); i++ )
      {
         s << "            <option";
         
         if ( i->second.mDomain == rec.domain)
         {
            s << " selected=\"true\""; 
         }
         
         s << ">" << i->second.mDomain << "</option>" << endl;
      }
      
      s <<
         "</select></td></tr>" << endl <<
         "<tr>" << endl << 
         "  <td align=\"right\" valign=\"middle\" >Password:</td>" << endl << 
         "  <td align=\"left\" valign=\"middle\"><input type=\"password\" name=\"password\" size=\"40\" placeholder=\"leave empty to keep current password\"/></td>" << endl <<
         "</tr>" << endl <<
         // Note that the UserStore only stores a passwordHash, so we will collect a password.  If one is provided in the
         // edit page, we will use it to generate a new passwordHash, otherwise we will leave the hash alone.

         "<tr>" << endl <<
         "  <td align=\"right\" valign=\"middle\" >Full Name:</td>" << endl <<
         "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"name\" value=\"" << rec.name <<
         "\" size=\"40\" placeholder=\"Alice Smith\"/></td>" << endl <<
         "</tr>" << endl <<

         "<tr>" << endl <<
         "  <td align=\"right\" valign=\"middle\" >Email:</td>" << endl <<
         "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"email\" value=\"" << rec.email <<
         "\" size=\"40\" placeholder=\"alice@example.com\"/></td>" << endl <<
         "</tr>" << endl <<

         "<tr>" << endl <<
         "  <td colspan=\"2\" align=\"right\" valign=\"middle\">" << endl <<
         "    <input type=\"submit\" name=\"submit\" value=\"Update\"/>" << endl <<
         "  </td>" << endl <<
         "</tr>" << endl <<

         "</table>" << endl <<
         "</form>" << endl
         ;
   }
   else
   {
      // go back to show users page
   }
}

void 
WebAdmin::buildShowUsersSubPage(DataStream& s)
{
   Dictionary::iterator pos;
   Data key;
   AbstractDb::UserRecord rec;

   if (!mRemoveSet.empty())
   {
      int j = 0;
      for (set<RemoveKey>::iterator i = mRemoveSet.begin(); i != mRemoveSet.end(); ++i)
      {
         mStore.mUserStore.eraseUser(i->mKey1);
         ++j;
      }
      s << "<p class=\"notice ok\"><em>Removed:</em> " << j << " records</p>" << endl;
   }
   
   pos = mHttpParams.find("key");
   if (pos != mHttpParams.end())  // check if the user parameter exists, if so, use as a key to update the record
   {
      key = pos->second;
      rec = mStore.mUserStore.getUserInfo(key);
      // check to see if we actually found a record corresponding to the key
      if (!rec.user.empty())
      {
         Data user = mHttpParams["user"];
         Data domain = mHttpParams["domain"];                  
         Data realm = mHttpParams["domain"];   // eventually sort out realms
         Data password = mHttpParams["password"];
         Data passwordHashAlt = Data::Empty;
         Data name = mHttpParams["name"];
         Data email = mHttpParams["email"];
         bool applyA1HashToPassword = true;

         // The stored password hash is MD5(user:realm:password), so if the
         // username or domain changes, the old hash is no longer valid for
         // the new identity and a new password is required.
         bool identityChanged = (user != rec.user) || (realm != rec.realm);
         if (identityChanged && password.empty())
         {
            s << "<p class=\"notice err\"><em>Error</em> updating user: changing user or domain "
                 "requires a new password (the stored password hash is bound "
                 "to user+realm).</p>\n";
         }
         else
         {
            // if no password was specified (and identity is unchanged), leave
            // the current password hash untouched.
            if (password.empty() && !identityChanged)
            {
               password = rec.passwordHash;
               passwordHashAlt = rec.passwordHashAlt;
               applyA1HashToPassword = false;
            }
            // write out the updated record to the database now
            if(mStore.mUserStore.updateUser(key, user, domain, realm, password, applyA1HashToPassword, name, email, passwordHashAlt))
            {
               s << "<p class=\"notice ok\"><em>Updated:</em> " << key << "</p>" << endl;
            }
            else
            {
               s << "<p class=\"notice err\"><em>Error</em> updating user: likely database error (check logs).</p>\n";
            }
         }
      }
   }
   
   s << 
      "<h2>Users</h2>" << endl <<
      "<form id=\"showUsers\" method=\"get\" action=\"showUsers.html\" name=\"showUsers\" enctype=\"application/x-www-form-urlencoded\">" << endl << 
      "<table" REPRO_BORDERED_TABLE_PROPS ">" << endl <<
      "<thead><tr>" << endl <<
      "  <td>User@Domain</td>" << endl <<
      //  "  <td>Realm</td>" << endl <<
      "  <td>Name</td>" << endl <<
      "  <td>Email</td>" << endl <<
      "  <td><input type=\"submit\" value=\"Remove\"/></td>" << endl <<
      "</tr></thead>" << endl <<
      "<tbody>" << endl;
   
   s << endl;
   
   int count =0;
   
   key = mStore.mUserStore.getFirstKey();
   while ( !key.empty() )
   {
      rec = mStore.mUserStore.getUserInfo(key);

      s << "<tr>" << endl 
        << "  <td><a href=\"editUser.html?key=";
      key.urlEncode(s);
      s << "\">" << rec.user << "@" << rec.domain << "</a></td>" << endl
        << "  <td>" << rec.name << "</td>" << endl
        << "  <td>" << rec.email << "</td>" << endl
        << "  <td><input type=\"checkbox\" name=\"remove." << key << "\"/></td>" << endl
        << "</tr>" << endl;
         
      key = mStore.mUserStore.getNextKey();

      // make a limit to how many users are displayed 
      if ( ++count > 1000 )
      {
         break;
      }
   }
   
   if ( !key.empty() )
   {
      s << "<tr><td colspan=\"4\">Only first 1000 users were displayed</td></tr>" << endl;
   }

   s <<
      "</tbody>" << endl <<
      "</table>" << endl <<
      "</form>" << endl;
}

void
WebAdmin::buildAddFilterSubPage(DataStream& s)
{
   Dictionary::iterator pos;

   pos = mHttpParams.find("cond1header");
   if (pos != mHttpParams.end())
   {
      Data action = mHttpParams["action"];
      Data actionData = mHttpParams["actiondata"];
      
      if (action != "Accept" && actionData.empty())
      {
         s << "<p class=\"notice err\"><em>Error</em> adding request filter.  You must provide appropriate Action Data for non-Accept action.</p>\n";
      }
      else
      {
         short actionShort = 0;  // 0 - Accept, 1 - Reject, 2 - SQL Query
         if(action == "Reject") actionShort = 1;
         else if(action == "SQL Query") actionShort = 2;

         if(mStore.mFilterStore.addFilter(mHttpParams["cond1header"],
                                          mHttpParams["cond1regex"],
                                          mHttpParams["cond2header"],
                                          mHttpParams["cond2regex"],
                                          mHttpParams["method"], 
                                          mHttpParams["event"],
                                          actionShort,
                                          actionData,
                                          mHttpParams["order"].convertInt()))
         {
            s << "<p class=\"notice ok\"><em>Added</em> request filter: " << mHttpParams["cond1header"] << "=" << mHttpParams["cond1regex"] << ", "
                                                      << mHttpParams["cond2header"] << "=" << mHttpParams["cond2regex"] << "</p>\n";
         }
         else
         {
            s << "<p class=\"notice err\"><em>Error</em> adding request filter, likely duplicate found.</p>\n";
         }
      }
   }

   s << 
      "<h2>Add Request Filter</h2>" << endl <<
      "<div class=\"help\">" << endl <<
      "  <p>A request filter matches an incoming request against one or two header values" << endl <<
      "  and decides whether to accept or reject it.  Both conditions must match for the" << endl <<
      "  filter to fire; leave the second pair empty to match on one header alone.  Regexes" << endl <<
      "  are POSIX-standard and are matched against the header's value.</p>" << endl <<
      "  <p>Method and Event narrow the filter to a single request type; leave them empty to" << endl <<
      "  match any.  Order decides evaluation sequence when several filters could match --" << endl <<
      "  lowest first.</p>" << endl <<
      "  <p>Action Data depends on the chosen Action:</p>" << endl <<
      "  <pre>Accept       Action Data is ignored." << endl <<
      "Reject       SIPRejectionCode[, SIPReason]  e.g. 403, Request Blocked" << endl <<
#ifdef USE_MYSQL
      "SQL Query    The SQL query to execute.  Replacement strings from the regexes" << endl <<
      "             above can be used in the query.  The query must return a string" << endl <<
      "             formatted like the Reject data above, or a string with a status" << endl <<
      "             code of 0 to accept the request." << endl <<
#endif
      "</pre>" << endl <<
      "</div>" << endl <<
      "<form id=\"addFilterForm\" method=\"get\" action=\"addFilter.html\" name=\"addFilterForm\">" << endl <<
      "<table" REPRO_BORDERLESS_TABLE_PROPS ">" << endl <<

      "<tr>" << endl << 
      "  <td align=\"right\" valign=\"middle\">Condition1 Header:</td>" << endl << 
      "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"cond1header\" size=\"40\" value=\"From\" placeholder=\"header name, e.g. From\"/></td>" << endl << 
      "</tr>" << endl << 

      "<tr>" << endl << 
      "  <td align=\"right\" valign=\"middle\">Condition1 Regex:</td>" << endl << 
      "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"cond1regex\" size=\"40\" placeholder=\"POSIX regex, e.g. sip:1234@.*\"/></td>" << endl << 
      "</tr>" << endl << 

      "<tr>" << endl << 
      "  <td align=\"right\" valign=\"middle\">Condition2 Header:</td>" << endl << 
      "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"cond2header\" size=\"40\" value=\"To\" placeholder=\"header name, e.g. To\"/></td>" << endl << 
      "</tr>" << endl << 

      "<tr>" << endl << 
      "  <td align=\"right\" valign=\"middle\">Condition2 Regex:</td>" << endl << 
      "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"cond2regex\" size=\"40\" placeholder=\"POSIX regex, empty = any\"/></td>" << endl << 
      "</tr>" << endl << 

      "<tr>" << endl << 
      "  <td align=\"right\" valign=\"middle\">Method:</td>" << endl << 
      "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"method\" size=\"40\" placeholder=\"INVITE, REGISTER... empty = any\"/></td>" << endl << 
      "</tr>" << endl << 

      "<tr>" << endl << 
      "  <td align=\"right\" valign=\"middle\">Event:</td>" << endl << 
      "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"event\" size=\"40\" placeholder=\"presence, message-summary... empty = any\"/></td>" << endl << 
      "</tr>" << endl << 
      
      "<tr>" << endl << 
      "  <td align=\"right\" valign=\"middle\">Action:</td>" << endl << 
      "  <td align=\"left\" valign=\"middle\">" << endl <<
      "    <select name=\"action\">" << endl <<
      "      <option>Reject</option>" << endl << 
      "      <option>Accept</option>" << endl << 
#ifdef USE_MYSQL
      "      <option>SQL Query</option>" << endl << 
#endif
      "    </select>" << endl <<
      "  </td>" << endl <<
      "</tr>" << endl <<

      "<tr>" << endl << 
      "  <td align=\"right\" valign=\"middle\">Action Data:</td>" << endl << 
      "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"actiondata\" size=\"40\" value=\"403, Request Blocked\" placeholder=\"e.g. 403, Request Blocked\"/></td>" << endl << 
      "</tr>" << endl << 

      "<tr>" << endl << 
      "  <td align=\"right\" valign=\"middle\">Order:</td>" << endl << 
      "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"order\" size=\"8\" value=\"0\" placeholder=\"0\"/></td>" << endl << 
      "</tr>" << endl << 

      "<tr>" << endl << 
      "  <td colspan=\"2\" align=\"right\" valign=\"middle\">" << endl << 
      "    <input type=\"reset\"  value=\"Cancel\"/>" << endl << 
      "    <input type=\"submit\" name=\"filterAdd\" value=\"Add\"/>" << endl << 
      "  </td>" << endl << 
      "</tr>" << endl << 

      "</table>" << endl <<
      "</form>" << endl;
}

void
WebAdmin::buildEditFilterSubPage(DataStream& s)
{
   Dictionary::iterator pos;
   pos = mHttpParams.find("key");
   if (pos != mHttpParams.end()) 
   {
      Data key = pos->second;

      // !rwm! TODO check to see if we actually found a record corresponding to the key.  how do we do that?
      DebugLog( << "Creating page to edit filter " << key );
      
      AbstractDb::FilterRecord rec = mStore.mFilterStore.getFilterRecord(key);

      s <<"<h2>Edit Request Filter</h2>" << endl <<
         "<p>Editing Record with conditions: " << rec.mCondition1Header << "=" << rec.mCondition1Regex << ", "
                                               << rec.mCondition2Header << "=" << rec.mCondition2Regex << "</p>" << endl;

      s << 
      "<form id=\"editFilterForm\" method=\"get\" action=\"showFilters.html\" name=\"editFilterForm\">" << endl << 
      "<table" REPRO_BORDERLESS_TABLE_PROPS ">" << endl << 
      "<input type=\"hidden\" name=\"key\" value=\"" << key << "\"/>" << endl << 
      "<tr>" << endl << 

      "<tr>" << endl << 
      "  <td align=\"right\" valign=\"middle\">Condition1 Header:</td>" << endl << 
      "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"cond1header\" size=\"40\" placeholder=\"header name, e.g. From\" value=\"" << rec.mCondition1Header.xmlCharDataEncode() << "\"/></td>" << endl << 
      "</tr>" << endl << 

      "<tr>" << endl << 
      "  <td align=\"right\" valign=\"middle\">Condition1 Regex:</td>" << endl << 
      "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"cond1regex\" size=\"40\" placeholder=\"POSIX regex, e.g. sip:1234@.*\" value=\"" << rec.mCondition1Regex.xmlCharDataEncode() << "\"/></td>" << endl << 
      "</tr>" << endl << 

      "<tr>" << endl << 
      "  <td align=\"right\" valign=\"middle\">Condition2 Header:</td>" << endl << 
      "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"cond2header\" size=\"40\" placeholder=\"header name, e.g. To\" value=\"" << rec.mCondition2Header.xmlCharDataEncode() << "\"/></td>" << endl << 
      "</tr>" << endl << 

      "<tr>" << endl << 
      "  <td align=\"right\" valign=\"middle\">Condition2 Regex:</td>" << endl << 
      "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"cond2regex\" size=\"40\" placeholder=\"POSIX regex, empty = any\" value=\"" << rec.mCondition2Regex.xmlCharDataEncode() << "\"/></td>" << endl << 
      "</tr>" << endl << 

      "<tr>" << endl << 
      "  <td align=\"right\" valign=\"middle\">Method:</td>" << endl << 
      "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"method\" size=\"40\" placeholder=\"INVITE, REGISTER... empty = any\" value=\"" << rec.mMethod << "\"/></td>" << endl << 
      "</tr>" << endl << 

      "<tr>" << endl << 
      "  <td align=\"right\" valign=\"middle\">Event:</td>" << endl << 
      "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"event\" size=\"40\" placeholder=\"presence, message-summary... empty = any\" value=\"" << rec.mEvent << "\"/></td>" << endl << 
      "</tr>" << endl << 
      
      "<tr>" << endl << 
      "  <td align=\"right\" valign=\"middle\">Action:</td>" << endl << 
      "  <td align=\"left\" valign=\"middle\">" << endl <<
      "    <select name=\"action\">" << endl <<
      "      <option" << (rec.mAction == FilterStore::Reject ? " selected=\"selected\"" : "") << ">Reject</option>" << endl << 
      "      <option" << (rec.mAction == FilterStore::Accept ? " selected=\"selected\"" : "") << ">Accept</option>" << endl << 
#ifdef USE_MYSQL
      "      <option" << (rec.mAction == FilterStore::SQLQuery ? " selected=\"selected\"" : "") << ">SQL Query</option>" << endl << 
#endif
      "    </select>" << endl <<
      "  </td>" << endl <<
      "</tr>" << endl <<

      "<tr>" << endl << 
      "  <td align=\"right\" valign=\"middle\">Action Data:</td>" << endl << 
      "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"actiondata\" size=\"40\" placeholder=\"e.g. 403, Request Blocked\" value=\"" << rec.mActionData.xmlCharDataEncode() << "\"/></td>" << endl << 
      "</tr>" << endl << 

      "<tr>" << endl << 
      "  <td align=\"right\" valign=\"middle\">Order:</td>" << endl << 
      "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"order\" size=\"8\" placeholder=\"0\" value=\"" << rec.mOrder << "\"/></td>" << endl << 
      "</tr>" << endl << 

      "<tr>" << endl << 
      "  <td colspan=\"2\" align=\"right\" valign=\"middle\">" << endl << 
      "    <input type=\"submit\" name=\"routeEdit\" value=\"Update\"/>" << endl << 
      "  </td>" << endl << 
      "</tr>" << endl << 

      "</table>" << endl << 
      "</form>" << endl;
   }
   else
   {
      // go back to show filter page
   }
}

void
WebAdmin::buildShowFiltersSubPage(DataStream& s)
{
   Dictionary::iterator pos;
   Data key;
   AbstractDb::RouteRecord rec;

   if (!mRemoveSet.empty())
   {
      int j = 0;
      for (set<RemoveKey>::iterator i = mRemoveSet.begin(); i != mRemoveSet.end(); ++i)
      {
         mStore.mFilterStore.eraseFilter(i->mKey1);
         ++j;
      }
      s << "<p class=\"notice ok\"><em>Removed:</em> " << j << " records</p>" << endl;
   }
   
   pos = mHttpParams.find("key");
   if (pos != mHttpParams.end())   // if a key parameter exists, use the key to update the record
   {
      key = pos->second;

      // !rwm! TODO check to see if we actually found a record corresponding to the key.  how do we do that?
      if (1)
      {
         Data action = mHttpParams["action"];
         Data actionData = mHttpParams["actiondata"];
      
         if (action != "Accept" && actionData.empty())
         {
            s << "<p class=\"notice err\"><em>Error</em> updating request filter.  You must provide appropriate Action Data for non-Accept action.</p>\n";
         }
         else
         {
            short actionShort = 0;  // 0 - Accept, 1 - Reject, 2 - SQL Query
            if(action == "Reject") actionShort = 1;
            else if(action == "SQL Query") actionShort = 2;

            if(mStore.mFilterStore.updateFilter(key,
                                             mHttpParams["cond1header"],
                                             mHttpParams["cond1regex"],
                                             mHttpParams["cond2header"],
                                             mHttpParams["cond2regex"],
                                             mHttpParams["method"], 
                                             mHttpParams["event"],
                                             actionShort,
                                             actionData,
                                             mHttpParams["order"].convertInt()))
            {
               s << "<p class=\"notice ok\"><em>Updated</em> request filter: " << mHttpParams["cond1header"] << "=" << mHttpParams["cond1regex"] << ", "
                                                           << mHttpParams["cond2header"] << "=" << mHttpParams["cond2regex"] << "</p>\n";
            }
            else
            {
               s << "<p class=\"notice err\"><em>Error</em> updating request filter: likely database error (check logs).</p>\n";
            }
         }
      }
   }

   s <<
      "<h2>Request Filters</h2>" << endl <<
      "<form id=\"showFilters\" action=\"showFilters.html\" method=\"get\" name=\"showFilters\" enctype=\"application/x-www-form-urlencoded\">" << endl << 
      // "            <button name=\"removeAllRoute\" value=\"\" type=\"submit\">Remove All</button>" << endl << 
      "<table" REPRO_BORDERED_TABLE_PROPS ">" << endl << 
      "<thead><tr>" << endl << 
      "  <td>Condition 1</td>" << endl << 
      "  <td>Condition 2</td>" << endl << 
      "  <td>Method</td>" << endl << 
      "  <td>Event</td>" << endl << 
      "  <td>Action</td>" << endl << 
      "  <td>Action Data</td>" << endl << 
      "  <td>Order</td>" << endl << 
      "  <td><input type=\"submit\" value=\"Remove\"/></td>" << endl << 
      "</tr></thead>" << endl << 
      "<tbody>" << endl;

   for(FilterStore::Key key = mStore.mFilterStore.getFirstKey();
       !key.empty();
        key = mStore.mFilterStore.getNextKey(key))
   {
      AbstractDb::FilterRecord rec = mStore.mFilterStore.getFilterRecord(key);
      Data action("Accept");
      if(rec.mAction == FilterStore::Reject)
      {
         action = "Reject";
      }
      else if(rec.mAction == FilterStore::SQLQuery)
      {
         action = "SQL Query";
      }
      s <<  "<tr>" << endl << 
         "<td><a href=\"editFilter.html?key=";
            key.urlEncode(s); 
      s << 
         "\">" << rec.mCondition1Header << "=" << rec.mCondition1Regex << "</a></td>" << endl << 
         "<td>" << rec.mCondition2Header << "=" << rec.mCondition2Regex << "</td>" << endl << 
         "<td>" << rec.mMethod << "</td>" << endl << 
         "<td>" << rec.mEvent << "</td>" << endl << 
         "<td>" << action << "</td>" << endl << 
         "<td>" << rec.mActionData << "</td>" << endl << 
         "<td>" << rec.mOrder << "</td>" << endl << 
         "<td><input type=\"checkbox\" name=\"remove." <<  key << "\"/></td>" << endl << 
         "</tr>" << endl;
   }

   s << 
      "</tbody>" << endl << 
      "</table>" << endl << 
      "</form>" << endl;

   Data cond1TestHeader;
   pos = mHttpParams.find("cond1TestHeader");
   if (pos != mHttpParams.end()) // found it
   {
      cond1TestHeader = pos->second;
   }
   Data cond2TestHeader;
   pos = mHttpParams.find("cond2TestHeader");
   if (pos != mHttpParams.end()) // found it
   {
      cond2TestHeader = pos->second;
   }

   s << 
      "<h3>Test Filters</h3>" << endl <<
      "<form id=\"testFilter\" action=\"showFilters.html\" method=\"get\" name=\"testFilter\" enctype=\"application/x-www-form-urlencoded\">" << endl <<
      "<table" REPRO_BORDERLESS_TABLE_PROPS ">" << endl << 
      "<tr>" << endl << 
      "  <td align=\"right\">Condition 1 Header:</td>" << endl << 
      "  <td><input type=\"text\" name=\"cond1TestHeader\" placeholder=\"header value to match, e.g. sip:1234@example.com\" value=\"" << cond1TestHeader.xmlCharDataEncode() << "\" size=\"40\"/></td>" << endl << 
      "</tr>" << endl <<
      "<tr>" << endl << 
      "  <td align=\"right\">Condition 2 Header:</td>" << endl << 
      "  <td><input type=\"text\" name=\"cond2TestHeader\" placeholder=\"header value to match, empty if unused\" value=\"" << cond2TestHeader.xmlCharDataEncode() << "\" size=\"40\"/></td>" << endl << 
      "  <td><input type=\"submit\" name=\"testFilter\" value=\"Test Filters\"/></td>" << endl << 
      "</tr>" << endl <<
      "</table>" << endl <<
      "</form>" << endl;

   if(!cond1TestHeader.empty())
   {
      s << "<p class=\"notice\"><em>Test Result:</em> ";
      short action;
      Data actionData;
      if(mStore.mFilterStore.test(cond1TestHeader, cond2TestHeader, action, actionData))
      {
         switch(action)
         {
         case FilterStore::Reject:
            s << "Match found, action=Reject " << actionData << endl;
            break;
         case FilterStore::SQLQuery:
            s << "Match found, action=SQL Query '" << actionData << "'" << endl;
            break;
         case FilterStore::Accept:
         default:
            s << "Match found, action=Accept" << endl;
            break;
         }
      }
      else
      {
         s << "<em class=\"warn\">No matches</em>";
      }
      s << "</p>" << endl;
   }
}

void
WebAdmin::buildAddRouteSubPage(DataStream& s)
{
   Dictionary::iterator pos;

   pos = mHttpParams.find("routeUri");
   if (pos != mHttpParams.end())
   {
      Data routeUri = mHttpParams["routeUri"];
      Data routeDestination = mHttpParams["routeDestination"];
      
      if (!routeUri.empty() && !routeDestination.empty())
      {
         if(mStore.mRouteStore.addRoute(mHttpParams["routeMethod"], 
                                        mHttpParams["routeEvent"], 
                                        routeUri,
                                        routeDestination,
                                        mHttpParams["routeOrder"].convertInt()))
         {
            s << "<p class=\"notice ok\"><em>Added</em> route for: " << routeUri << "</p>\n";
         }
         else
         {
            s << "<p class=\"notice err\"><em>Error</em> adding route, likely duplicate found.</p>\n";
         }
      }
      else
      {
         s << "<p class=\"notice err\"><em>Error</em> adding route.  You must provide a URI and a route destination.</p>\n";
      }
   }

   s << 
      "<h2>Add Route</h2>" << endl <<
      "<div class=\"help\">" << endl <<
      "  <p>Static routes use (POSIX-standard) regular expressions to match and rewrite SIP" << endl <<
      "  URIs.  Method and Event narrow the route to a single request type; leave them empty" << endl <<
      "  to match any.  Order decides evaluation sequence when several routes could match --" << endl <<
      "  lowest first.</p>" << endl <<
      "  <p>The following sends all requests whose userpart is only digits to a gateway:</p>" << endl <<
      "  <pre>URI:         ^sip:([0-9]+)@example\\.com" << endl <<
      "Destination: sip:$1@gateway.example.com</pre>" << endl <<
      "</div>" << endl <<
      "<form id=\"addRouteForm\" method=\"get\" action=\"addRoute.html\" name=\"addRouteForm\">" << endl <<
      "<table" REPRO_BORDERLESS_TABLE_PROPS ">" << endl <<

      "<tr>" << endl << 
      "  <td align=\"right\" valign=\"middle\">URI:</td>" << endl << 
      "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"routeUri\" size=\"40\" placeholder=\"^sip:([0-9]+)@example.com\"/></td>" << endl << 
      "</tr>" << endl << 

      "<tr>" << endl << 
      "  <td align=\"right\" valign=\"middle\">Method:</td>" << endl << 
      "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"routeMethod\" size=\"40\" placeholder=\"INVITE, REGISTER... empty = any\"/></td>" << endl << 
      "</tr>" << endl << 
      
      "<tr>" << endl << 
      "  <td align=\"right\" valign=\"middle\">Event:</td>" << endl << 
      "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"routeEvent\" size=\"40\" placeholder=\"presence, message-summary... empty = any\"/></td>" << endl << 
      "</tr>" << endl << 
      
      "<tr>" << endl << 
      "  <td align=\"right\" valign=\"middle\">Destination:</td>" << endl << 
      "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"routeDestination\" size=\"40\" placeholder=\"sip:$1@gateway.example.com\"/></td>" << endl << 
      "</tr>" << endl << 
      
      "<tr>" << endl << 
      "  <td align=\"right\" valign=\"middle\">Order:</td>" << endl << 
      "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"routeOrder\" size=\"8\" placeholder=\"0\"/></td>" << endl << 
      "</tr>" << endl << 

      "<tr>" << endl << 
      "  <td colspan=\"2\" align=\"right\" valign=\"middle\">" << endl << 
      "    <input type=\"reset\"  value=\"Cancel\"/>" << endl << 
      "    <input type=\"submit\" name=\"routeAdd\" value=\"Add\"/>" << endl << 
      "  </td>" << endl << 
      "</tr>" << endl << 

      "</table>" << endl <<
      "</form>" << endl;
}

void
WebAdmin::buildEditRouteSubPage(DataStream& s)
{
   Dictionary::iterator pos;
   pos = mHttpParams.find("key");
   if (pos != mHttpParams.end()) 
   {
      Data key = pos->second;

      // !rwm! TODO check to see if we actually found a record corresponding to the key.  how do we do that?
      DebugLog( << "Creating page to edit route " << key );
      
      AbstractDb::RouteRecord rec = mStore.mRouteStore.getRouteRecord(key);

      s <<"<h2>Edit Route</h2>" << endl <<
          "<p>Editing Record with matching pattern: " << rec.mMatchingPattern << "</p>" << endl;      

      s << 
      "<form id=\"editRouteForm\" method=\"get\" action=\"showRoutes.html\" name=\"editRouteForm\">" << endl << 
      "<table" REPRO_BORDERLESS_TABLE_PROPS ">" << endl << 
      "<input type=\"hidden\" name=\"key\" value=\"" << key << "\"/>" << endl << 
      "<tr>" << endl << 
      "<td align=\"right\" valign=\"middle\">URI:</td>" << endl << 
      "<td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"routeUri\" placeholder=\"^sip:([0-9]+)@example.com\" value=\"" <<  rec.mMatchingPattern << "\" size=\"40\"/></td>" << endl << 
      "</tr>" << endl << 

      "<tr>" << endl << 
      "<td align=\"right\" valign=\"middle\">Method:</td>" << endl << 
      "<td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"routeMethod\" placeholder=\"INVITE, REGISTER... empty = any\" value=\"" <<  rec.mMethod  << "\" size=\"40\"/></td>" << endl << 
      "</tr>" << endl << 
      
      "<tr>" << endl << 
      "<td align=\"right\" valign=\"middle\">Event:</td>" << endl << 
      "<td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"routeEvent\" placeholder=\"presence, message-summary... empty = any\" value=\"" << rec.mEvent  << "\" size=\"40\"/></td>" << endl << 
      "</tr>" << endl << 
      
      "<tr>" << endl << 
      "<td align=\"right\" valign=\"middle\">Destination:</td>" << endl << 
      "<td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"routeDestination\" placeholder=\"sip:$1@gateway.example.com\" value=\"" << rec.mRewriteExpression <<
                            "\" size=\"40\"/></td>" << endl << 
      "</tr>" << endl << 
      
      "<tr>" << endl << 
      "<td align=\"right\" valign=\"middle\">Order:</td>" << endl << 
      "<td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"routeOrder\" placeholder=\"0\" value=\"" << rec.mOrder  <<
                            "\" size=\"4\"/></td>" << endl << 
      "</tr>" << endl << 

      "<tr>" << endl << 
      "  <td colspan=\"2\" align=\"right\" valign=\"middle\">" << endl << 
      "    <input type=\"submit\" name=\"routeEdit\" value=\"Update\"/>" << endl << 
      "  </td>" << endl << 
      "</tr>" << endl << 

      "</table>" << endl << 
      "</form>" << endl;
   }
   else
   {
      // go back to show route page
   }
}

void
WebAdmin::buildShowRoutesSubPage(DataStream& s)
{
   Dictionary::iterator pos;
   Data key;
   AbstractDb::RouteRecord rec;

   if (!mRemoveSet.empty())
   {
      int j = 0;
      for (set<RemoveKey>::iterator i = mRemoveSet.begin(); i != mRemoveSet.end(); ++i)
      {
         mStore.mRouteStore.eraseRoute(i->mKey1);
         ++j;
      }
      s << "<p class=\"notice ok\"><em>Removed:</em> " << j << " records</p>" << endl;
   }
   
   pos = mHttpParams.find("key");
   if (pos != mHttpParams.end())   // if a key parameter exists, use the key to update the record
   {
      key = pos->second;

      // !rwm! TODO check to see if we actually found a record corresponding to the key.  how do we do that?
      if (1)
      {
         Data method = mHttpParams["routeMethod"]; 
         Data event = mHttpParams["routeEvent"]; 
         Data matchingPattern = mHttpParams["routeUri"];
         Data rewriteExpression = mHttpParams["routeDestination"];
         int  order = mHttpParams["routeOrder"].convertInt();
         
         if (!matchingPattern.empty() && !rewriteExpression.empty())
         {
            // write out the updated record to the database now
            if(mStore.mRouteStore.updateRoute(key, method, event, matchingPattern, rewriteExpression, order))
            {
               s << "<p class=\"notice ok\"><em>Updated:</em> " << rec.mMatchingPattern << "</p>" << endl; 
            }
            else
            {
               s << "<p class=\"notice err\"><em>Error</em> updating route: likely database error (check logs).</p>\n";
            }
         }
         else
         {
            s << "<p class=\"notice err\"><em>Error</em> updating route.  You must provide a URI and a route destination.</p>\n";
         }
      }
   }

   s <<
      "<h2>Routes</h2>" << endl <<
      "<form id=\"showRoutes\" action=\"showRoutes.html\" method=\"get\" name=\"showRoutes\" enctype=\"application/x-www-form-urlencoded\">" << endl << 
      // "            <button name=\"removeAllRoute\" value=\"\" type=\"submit\">Remove All</button>" << endl << 
      "<table" REPRO_BORDERED_TABLE_PROPS ">" << endl << 
      "<thead><tr>" << endl << 
      "  <td>URI</td>" << endl << 
      "  <td>Method</td>" << endl << 
      "  <td>Event</td>" << endl << 
      "  <td>Destination</td>" << endl << 
      "  <td>Order</td>" << endl << 
      "  <td><input type=\"submit\" value=\"Remove\"/></td>" << endl << 
      "</tr></thead>" << endl << 
      "<tbody>" << endl;
   
   for ( RouteStore::Key key = mStore.mRouteStore.getFirstKey();
         !key.empty();
         key = mStore.mRouteStore.getNextKey(key) )
   {
      AbstractDb::RouteRecord rec = mStore.mRouteStore.getRouteRecord(key);

      s <<  "<tr>" << endl << 
         "<td><a href=\"editRoute.html?key=";
            key.urlEncode(s); 
      s << 
         "\">" << rec.mMatchingPattern << "</a></td>" << endl << 
         "<td>" << rec.mMethod << "</td>" << endl << 
         "<td>" << rec.mEvent << "</td>" << endl << 
         "<td>" << rec.mRewriteExpression << "</td>" << endl << 
         "<td>" << rec.mOrder << "</td>" << endl << 
         "<td><input type=\"checkbox\" name=\"remove." <<  key << "\"/></td>" << endl << 
         "</tr>" << endl;
   }
   
   s << 
      "</tbody>" << endl << 
      "</table>" << endl << 
      "</form>" << endl;

   int badUri = true;
   Uri uri;
   Data routeTestUri;
   
   pos = mHttpParams.find("routeTestUri");
   if (pos != mHttpParams.end()) // found it
   {
      routeTestUri = pos->second;
      if ( routeTestUri  != "sip:" )
      {
         try 
         {
            uri = Uri(routeTestUri);
            badUri=false;
         }
         catch( BaseException&  )
         {
            try 
            {
               uri = Uri( Data("sip:")+routeTestUri );
               badUri=false;
            }
            catch( BaseException&  )
            {
            }
         }
      }
   }
      
   // !cj! - TODO - should input method and event type to test 
   RouteStore::UriList routeList;
   if (!badUri)
   {
      routeList = mStore.mRouteStore.process(uri, Data("INVITE"), Data::Empty);
   }
   
   s << 
      "<h3>Test Routes</h3>" << endl <<
      "<form id=\"testRoute\" action=\"showRoutes.html\" method=\"get\" name=\"testRoute\" enctype=\"application/x-www-form-urlencoded\">" << endl <<
      "<table" REPRO_BORDERLESS_TABLE_PROPS ">" << endl << 
      "<tr>" << endl << 
      " <td align=\"right\">Input:</td>" << endl << 
      " <td><input type=\"text\" name=\"routeTestUri\" placeholder=\"sip:1234@example.com\" value=\"" << uri << "\" size=\"40\"/></td>" << endl << 
      " <td><input type=\"submit\" name=\"testRoute\" value=\"Test Routes\"/></td>" << endl << 
      "</tr>" << endl;
   
   bool first=true;
   for ( RouteStore::UriList::const_iterator i=routeList.begin();
         i != routeList.end(); i++)
   {
      s<<"              <tr>" << endl;
      if (first)
      {
         first=false;
         s<<"             <td align=\"right\">Targets:</td>" << endl;
      }
      else
      {
         s<<"             <td align=\"right\"></td>" << endl;
      }
      s<<"                <td><label>" << *i << "</label></td>" << endl;
      s<<"                <td></td>" << endl;
      s<<"              </tr>" << endl;
   }

   // A test that matched nothing has to say so, otherwise the page looks
   // identical to one where the test was never run.
   if (first && !routeTestUri.empty())
   {
      s<<"              <tr>" << endl;
      s<<"                <td align=\"right\">Targets:</td>" << endl;
      s<<"                <td><em class=\"warn\">"
        << (badUri ? "Not a valid SIP URI" : "No matches")
        << "</em></td>" << endl;
      s<<"                <td></td>" << endl;
      s<<"              </tr>" << endl;
   }


   s<<
      "</table>" << endl << 
      "</form>" << endl;
}

void
WebAdmin::buildRegistrationsSubPage(DataStream& s)
{
   if (!mRemoveSet.empty())
   {
      int j = 0;
      for (set<RemoveKey>::iterator i = mRemoveSet.begin(); i != mRemoveSet.end(); ++i)
      {
         Uri aor(i->mKey1);
         ContactInstanceRecord rec;
         Data::size_type bar1 = i->mKey2.find("|");
         Data::size_type bar2 = i->mKey2.find("|",bar1+1);
         Data::size_type bar3 = i->mKey2.find("|",bar2+1);
         
         if(bar1==Data::npos || bar2 == Data::npos || bar3==Data::npos)
         {
            WarningLog(<< "Registration removal key was malformed: " << i->mKey2);
            continue;
         }
         
         bool staticRegContact=false;
         try
         {
            resip::Data rawNameAddr = i->mKey2.substr(0,bar1).urlDecoded();
            rec.mContact = NameAddr(rawNameAddr);
            rec.mInstance = i->mKey2.substr(bar1+1,bar2-bar1-1).urlDecoded();
            rec.mRegId = i->mKey2.substr(bar2+1,Data::npos).convertInt();
            staticRegContact = i->mKey2.substr(bar3+1,Data::npos).convertInt() == 1;

            // Remove from RegistrationPersistanceManager
            mRegDb.removeContact(aor, rec);

            if(staticRegContact)
            {
               // Remove from StateRegStore
               mStore.mStaticRegStore.eraseStaticReg(aor, rec.mContact);
            }

            ++j;
         }
         catch(resip::ParseBuffer::Exception& e)
         {
            WarningLog(<< "Registration removal key was malformed: " << e <<
                     " Key was: " << i->mKey2);
         }
      }
      s << "<p class=\"notice ok\"><em>Removed:</em> " << j << " records</p>" << endl;
   }

   Dictionary::iterator pos = mHttpParams.find("regAor");
   if (pos != mHttpParams.end() && (mHttpParams["action"] == "Add Static Registration")) // found
   {
      Data regAor = mHttpParams["regAor"];
      Data regContact = mHttpParams["regContact"];
      Data regPath = mHttpParams["regPath"];

      ContactInstanceRecord rec;
      try
      {
         rec.mContact = NameAddr(regContact);
         try
         {
            ParseBuffer pb(regPath);
            Data path;
            const char* anchor = pb.position();
            while(!pb.eof())
            {
               pb.skipToChar(Symbols::COMMA[0]);
               pb.data(path, anchor);
               rec.mSipPath.push_back(NameAddr(path));
               if(!pb.eof()) pb.skipChar();  // skip over comma
               anchor = pb.position();
            }
            try
            {
               rec.mRegExpires = NeverExpire;
               rec.mSyncContact = true;  // Tag this permanent contact as being a synchronized contact so that it will
                                         // not be synchronized to a paired server (this is actually configuration information)

               // Add to DB Store
               Uri aor(regAor);
               if(mStore.mStaticRegStore.addStaticReg(aor, rec.mContact, rec.mSipPath))
               {   
                  // Add to RegistrationPersistanceManager
                  mRegDb.updateContact(aor, rec);

                  s << "<p class=\"notice ok\"><em>Added</em> permanent registered contact for: " << regAor << "</p>\n";
               }
               else
               {
                  s << "<p class=\"notice err\"><em>Error</em> adding static registration: likely database error (check logs).</p>\n";
               }
            }
            catch(resip::ParseBuffer::Exception& e)
            {
               WarningLog(<< "Registration add: aor " << regAor << " was malformed: " << e);
               s << "<p class=\"notice err\"><em>Error</em> parsing: AOR=" << regAor << "</p>\n";
            }  
         }
         catch(resip::ParseBuffer::Exception& e)
         {
            WarningLog(<< "Registration add: path " << regPath << " was malformed: " << e);
            s << "<p class=\"notice err\"><em>Error</em> parsing: Path=" << regPath << "</p>\n";
         }
      }
      catch(resip::ParseBuffer::Exception& e)
      {
         WarningLog(<< "Registration add: contact " << regContact << " was malformed: " << e);
         s << "<p class=\"notice err\"><em>Error</em> parsing: Contact=" << regContact << "</p>\n";
      }
   }   
   
   s << 
      "<h2>Registrations</h2>" << endl <<
       "<form id=\"showReg\" method=\"get\" action=\"registrations.html\" name=\"showReg\" enctype=\"application/x-www-form-urlencoded\">" << endl <<
      //"<button name=\"removeAllReg\" value=\"\" type=\"button\">Remove All</button>" << endl <<
      //"<hr/>" << endl <<

      "<h3>Add Static Registration</h3>" << endl <<
      "<p>A static registration is a permanent contact for an AOR: it never expires and is" << endl <<
      "restored on restart, so it does not need the device to send a REGISTER.  Use it for" << endl <<
      "gateways and other endpoints that cannot register for themselves.</p>" << endl <<
      "<table" REPRO_BORDERLESS_TABLE_PROPS ">" << endl <<
      "  <tr>" << endl <<
      "    <td align=\"right\">AOR:</td>" << endl <<
      "    <td><input type=\"text\" name=\"regAor\" size=\"46\" placeholder=\"sip:alice@example.com\"/></td>" << endl <<
      "  </tr>" << endl <<
      "  <tr>" << endl <<
      "    <td align=\"right\">Contact:</td>" << endl <<
      "    <td><input type=\"text\" name=\"regContact\" size=\"46\" placeholder=\"&lt;sip:alice@192.168.1.10:5060&gt;\"/></td>" << endl <<
      "  </tr>" << endl <<
      "  <tr>" << endl <<
      "    <td align=\"right\">Path:</td>" << endl <<
      "    <td><input type=\"text\" name=\"regPath\" size=\"46\" placeholder=\"optional, comma separated URIs\"/></td>" << endl <<
      "  </tr>" << endl <<
      "  <tr>" << endl <<
      "    <td></td>" << endl <<
      "    <td align=\"right\"><input type=\"submit\" name=\"action\" value=\"Add Static Registration\"/></td>" << endl <<
      "  </tr>" << endl <<
      "</table>" << endl <<

      "<h3>Active Registrations</h3>" << endl <<
      "<table" REPRO_BORDERED_TABLE_PROPS ">" << endl <<

      "<thead><tr>" << endl <<
      "  <td>AOR</td>" << endl <<
      "  <td>Contact</td>" << endl <<
      "  <td>User Agent</td>" << endl <<
      "  <td>Instance ID</td>" << endl <<
      "  <td>Reg ID</td>" << endl <<
      "  <td>QValue</td>" << endl <<
      "  <td>Path</td>" << endl <<
      "  <td>Sync'd?</td>" << endl <<
      "  <td>Expires In</td>" << endl <<
      "  <td><input type=\"submit\" value=\"Remove\"/></td>" << endl <<
      "</tr></thead>" << endl <<
      "<tbody>" << endl;
  
   uint64_t now = Timer::getTimeSecs();
   RegistrationPersistenceManager::UriList aors;
   mRegDb.getAors(aors);
   for ( RegistrationPersistenceManager::UriList::const_iterator 
            aorIt = aors.begin(); aorIt != aors.end(); ++aorIt )
   {
      Uri aor = *aorIt;
      ContactList contacts;
      mRegDb.getContacts(aor, contacts);
         
      bool first = true;
      for (ContactList::iterator i = contacts.begin();
            i != contacts.end(); ++i )
      {
         if(i->mRegExpires > now)
         {
            uint64_t secondsRemaining = i->mRegExpires - now;

            s << "<tr>" << endl
              << "  <td>" ;
            if (first) 
            { 
               s << aor;
               first = false;
            }
            s << "</td>" << endl
              << "  <td>";
            
            const ContactInstanceRecord& r = *i;
            const NameAddr& contact = r.mContact;
            const Data& instanceId = r.mInstance;
            int regId = r.mRegId;

            s << contact.uri();
            s << "</td>" << endl
                << "  <td>";

            s << r.mUserAgent;

            s <<"</td>" << endl 
               << "<td>" << instanceId.xmlCharDataEncode() 
               << "</td><td>" << regId 
               << "</td><td>";
#ifdef RESIP_FIXED_POINT
            // If RESIP_FIXED_POINT is enabled then q-value is shown as an integer in the range of 0..1000 where 1000 = qvalue of 1.0
            s << (contact.exists(p_q) ? contact.param(p_q) : 1000) << "</td><td>";  
#else
            s << (contact.exists(p_q) ? contact.param(p_q).floatVal() : 1.0f) << "</td><td>";
#endif
            NameAddrs::const_iterator naIt = r.mSipPath.begin();
            for(;naIt != r.mSipPath.end(); naIt++)
            {
               s << naIt->uri() << "<br>" << endl;
            }

            bool staticRegContact = r.mRegExpires == NeverExpire;
            if(!staticRegContact)
            {
               s << "</td><td>" << (r.mSyncContact ? "true" : "false") << "</td>" << endl;
               s << "<td>" << secondsRemaining << "s</td>" << endl;
            }
            else
            {
               s << "</td><td></td>" << endl;
               s << "</td><td>Never</td>" << endl;
            }
            s << "  <td>"
               << "<input type=\"checkbox\" name=\"remove." << aor << "\" value=\"" << Data::from(contact.uri()).urlEncoded() 
                                                            << "|" << instanceId.urlEncoded() 
                                                            << "|" << regId
                                                            << "|" << (staticRegContact ? "1" : "0")
               << "\"/></td>" << endl
               << "</tr>" << endl;
         }
         else
         {
            // remove expired contact 
            mRegDb.removeContact(aor, *i);
         }
      }
   }
                  
   s << "</tbody>" << endl <<
      "</table>" << endl <<
      "</form>" << endl;
}

void
WebAdmin::buildPublicationsSubPage(DataStream& s)
{
   if (!mRemoveSet.empty())
   {
      int j = 0;
      for (set<RemoveKey>::iterator i = mRemoveSet.begin(); i != mRemoveSet.end(); ++i)
      {
         Data::size_type bar1 = i->mKey2.find("|");

         if (bar1 == Data::npos)
         {
            WarningLog(<< "Publication removal key was malformed: " << i->mKey2);
            continue;
         }

         try
         {
            Data eventType = i->mKey2.substr(0, bar1);
            Data eTag = i->mKey2.substr(bar1 + 1, Data::npos).urlDecoded();
            if (mPubDb.removeDocument(eventType, i->mKey1, eTag, Timer::getTimeSecs()))
            {
               ++j;
            }
            else
            {
               WarningLog(<< "Publication removal was unsuccessful: eventType=" << eventType << ", docKey=" << i->mKey1 << ", eTag=" << eTag);
            }
         }
         catch (resip::ParseBuffer::Exception& e)
         {
            WarningLog(<< "Publication removal key was malformed: " << e << " Key was: " << i->mKey2);
         }
      }
      s << "<p class=\"notice ok\"><em>Removed:</em> " << j << " records</p>" << endl;
   }

   s <<
      "<h2>Publications</h2>" << endl <<
      "<form id=\"showPub\" method=\"get\" action=\"publications.html\" name=\"showPub\" enctype=\"application/x-www-form-urlencoded\">" << endl <<
      "<div class=space>" << endl <<
      "</div>" << endl <<

      "<table" REPRO_BORDERED_TABLE_PROPS ">" << endl <<

      "<thead><tr>" << endl <<
      "  <td>AOR</td>" << endl <<
      "  <td>Event Type</td>" << endl <<
      "  <td>ETag</td>" << endl <<
      "  <td>Data</td>" << endl <<
      "  <td>Sync'd?</td>" << endl <<
      "  <td>Expires In</td>" << endl <<
      "  <td><input type=\"submit\" value=\"Remove\"/></td>" << endl <<
      "</tr></thead>" << endl <<
      "<tbody>" << endl;

   uint64_t now = Timer::getTimeSecs();
   mPubDb.lockDocuments();
   PublicationPersistenceManager::KeyToETagMap& publications = mPubDb.getDocuments();
   // Iterate through keys
   PublicationPersistenceManager::KeyToETagMap::iterator keyIt = publications.begin();
   for (; keyIt != publications.end(); keyIt++)
   {
      bool first = true;
      // Iterator through documents in sub-map
      PublicationPersistenceManager::ETagToDocumentMap::iterator eTagIt = keyIt->second.begin();
      for (; eTagIt != keyIt->second.end(); eTagIt++)
      {
         if (eTagIt->second.mExpirationTime > now)
         {
            uint64_t secondsRemaining = eTagIt->second.mExpirationTime - now;

            s << "<tr>" << endl
               << "  <td>";
            if (first)
            {
               s << eTagIt->second.mDocumentKey;
            }
            s << "</td>" << endl
               << "  <td>";

            if (first)
            {
               s << eTagIt->second.mEventType;
               first = false;
            }
            s << "</td>" << endl
              << "  <td>";
            s << eTagIt->second.mETag;
            s << "</td>" << endl
               << "  <td>";
            GenericPidfContents* pidf = dynamic_cast<GenericPidfContents*>(eTagIt->second.mContents.get());
            if (pidf)
            {
               s << (pidf->getSimplePresenceOnline() ? "open" : "closed");
               if (!pidf->getSimplePresenceNote().empty())
               {
                  s << " - " << pidf->getSimplePresenceNote();
               }
            }
            s << "</td>" << endl;

            s << "<td>" << (eTagIt->second.mSyncPublication ? "true" : "false") << "</td>" << endl
               << "<td>" << secondsRemaining << "s</td>" << endl
               << "  <td>"
               << "<input type=\"checkbox\" name=\"remove." << eTagIt->second.mDocumentKey << "\" value=\"" << eTagIt->second.mEventType << "|" << eTagIt->second.mETag.urlEncoded()
               << "\"/></td>" << endl
               << "</tr>" << endl;
         }
      }
   }
   mPubDb.unlockDocuments();
   s << "</tbody>" << endl <<
      "</table>" << endl <<
      "</form>" << endl;
}

void
WebAdmin::buildSettingsSubPage(DataStream& s)
{
   if (mHttpParams["action"] == "Clear DNS Cache")
   {
      mProxy.getStack().clearDnsCache();
   }

   if (mHttpParams["action"] == "Reload DNS Servers")
   {
       mProxy.getStack().reloadDnsServers();
   }

   s << "<h2>Logging and Admin</h2>" << endl
       << "<div class=\"toolbar\">" << endl
       << "<form id=\"logLevel\" method=\"get\" action=\"logLevel.html\" name=\"logLevel\">" << endl
       << "  <label>Log level</label> <select name=\"level\">" << endl
       << "        <option value=\"NONE\"" << (Log::level() == Log::None ? " selected" : "") << ">NONE" << (Log::level() == Log::None ? " *" : "") << "</option>" << endl
       << "        <option value=\"CRIT\"" << (Log::level() == Log::Crit ? " selected" : "") << ">CRIT" << (Log::level() == Log::Crit ? " *" : "") << "</option>" << endl
       << "        <option value=\"ERR\"" << (Log::level() == Log::Err ? " selected" : "") << ">ERR" << (Log::level() == Log::Err ? " *" : "") << "</option>" << endl
       << "        <option value=\"WARNING\"" << (Log::level() == Log::Warning ? " selected" : "") << ">WARNING" << (Log::level() == Log::Warning ? " *" : "") << "</option>" << endl
       << "        <option value=\"INFO\"" << (Log::level() == Log::Info ? " selected" : "") << ">INFO" << (Log::level() == Log::Info ? " *" : "") << "</option>" << endl
       << "        <option value=\"DEBUG\"" << (Log::level() == Log::Debug ? " selected" : "") << ">DEBUG" << (Log::level() == Log::Debug ? " *" : "") << "</option>" << endl
       << "        <option value=\"STACK\"" << (Log::level() == Log::Stack ? " selected" : "") << ">STACK" << (Log::level() == Log::Stack ? " *" : "") << "</option>" << endl
       << "       </select>" << endl
       << "  <input type=\"submit\" name=\"action\" value=\"Set level\"/>" << endl
       << "</form>" << endl;

#ifdef USE_SSL
   s << "<form id=\"reloadCerts\" method=\"get\" action=\"reloadcerts.html\" name=\"reloadcerts\">" << endl
       << "  <input type=\"submit\" name=\"action\" value=\"Reload Certificates\"/>" << endl
       << "</form>" << endl;
#endif

   if (mProxy.getConfig().getConfigUnsignedShort("CommandPort", REPRO_DEFAULT_COMMAND_PORT) != 0)
   {
       s << "<form id=\"restartProxy\" method=\"get\" action=\"restart.html\" name=\"restart\">" << endl
           << "  <input type=\"submit\" name=\"action\" value=\"Restart Proxy\"/>" << endl
           << "</form>" << endl;
   }

   s << "</div>" << endl;   // toolbar

   s << "<h2>DNS Cache</h2>" << endl;

   // Get Dns Cache
   {
       Lock lock(mDnsCacheMutex);
       mProxy.getStack().getDnsCacheDump(make_pair(0, 0), this);
       // Retrieving DNS cache is asyncronous
       // Use condition variable to wait for DNS results to be returned in onDnsCacheDumpRetrieved
       mDnsCacheCondition.wait(lock);
       s << "<pre>" << mDnsCache << "</pre>"
         << endl;
   }

   s << "<div class=\"toolbar\">" << endl
       << "<form id=\"dnsButtons\" method=\"get\" action=\"settings.html\" name=\"dnsButtons\">" << endl
       << "  <input type=\"submit\" name=\"action\" value=\"Clear DNS Cache\"/>" << endl
       << "  <input type=\"submit\" name=\"action\" value=\"Reload DNS Servers\"/>" << endl
       << "</form>" << endl
       << "</div>" << endl;

   {
      Data buffer;
      DataStream strm(buffer);
      mProxy.getStack().dump(strm);
      strm.flush();
      s << "<h2>Stack Info</h2>" << endl
        << "<pre>" <<  buffer << "</pre>"
        << endl;
   }

   if(mProxy.getStack().getCongestionManager())
   {
      Data buffer;
      DataStream strm(buffer);
      mProxy.getStack().getCongestionManager()->encodeCurrentState(strm);
      s << "<h2>Congestion Manager Statistics</h2>" << endl
        << "<pre>" <<  buffer << "</pre>"
        << endl;
   }

   s << "<h2>Settings</h2>" << endl <<
        "<pre>" << mProxy.getConfig() << "</pre>" << endl;
}

void 
WebAdmin::onDnsCacheDumpRetrieved(std::pair<unsigned long, unsigned long> key, const resip::Data& dnsEntryStrings)
{
   Lock lock(mDnsCacheMutex); (void)lock;
   if(dnsEntryStrings.empty())
   {
      mDnsCache = "<i>empty</i>";
   }
   else
   {
      mDnsCache = dnsEntryStrings;
   }
   mDnsCacheCondition.notify_one();
}

void
WebAdmin::buildRestartSubPage(DataStream& s)
{
   unsigned short port = mProxy.getConfig().getConfigUnsignedShort("CommandPort", REPRO_DEFAULT_COMMAND_PORT);
   if (port != 0)
   {
      int sd = 0, rc;
      const char* host = "127.0.0.1";

      struct addrinfo hints = {};
      struct addrinfo* res = nullptr;
      hints.ai_family = AF_INET;
      hints.ai_socktype = SOCK_STREAM;

      if (getaddrinfo(host, nullptr, &hints, &res) == 0 && res != nullptr)
      {
         struct sockaddr_in servAddr = {};
         memcpy(&servAddr, res->ai_addr, res->ai_addrlen);
         servAddr.sin_port = htons(port);
         freeaddrinfo(res);

         // Create TCP Socket
         sd = (int)socket(AF_INET, SOCK_STREAM, 0);
         if (sd >= 0)
         {
            // bind to any local interface/port
            struct sockaddr_in localAddr = {};
            localAddr.sin_family = AF_INET;
            localAddr.sin_addr.s_addr = htonl(INADDR_ANY);
            localAddr.sin_port = 0;
            rc = ::bind(sd, (struct sockaddr*)&localAddr, sizeof(localAddr));
            if (rc >= 0)
            {
               // Connect to server
               rc = ::connect(sd, (struct sockaddr*)&servAddr, sizeof(servAddr));
               if (rc >= 0)
               {
                  Data request("<Restart>\r\n  <Request>\r\b  </Request>\r\n</Restart>\r\n");
                  rc = send(sd, request.c_str(), (int)request.size(), 0);
                  if (rc >= 0)
                  {
                     s << "<p class=\"notice ok\"><em>Restarting proxy...</em></p>" << endl;
                     closeSocket(sd);
                     return;
                  }
               }
            }
            closeSocket(sd);
         }
      }
      s << "<p class=\"notice err\"><em>Error</em> issuing restart command.</p>" << endl;
   }
   else
   {
      s << "<p class=\"notice warn\"><em>Unavailable:</em> CommandServer must be running to use the restart feature.</p>" << endl;
   }
}

void
WebAdmin::buildLogLevelSubPage(resip::DataStream& s)
{
   Dictionary::iterator pos;
   Data newLevel;

   pos = mHttpParams.find("level");
   if (pos != mHttpParams.end()) // found user key
   {
      newLevel = pos->second;
      InfoLog(<<"new log level requested: " << newLevel);

      Log::Level l = Log::toLevel(newLevel);
      Log::setLevel(l);

      s << "<p class=\"notice ok\"><em>Log level changed</em> to " << newLevel.xmlCharDataEncode() << ".</p>" << endl;
   }
   else
   {
      WarningLog(<<"no log level specified");
      s << "<p class=\"notice err\"><em>Error</em> no log level specified.</p>" << endl;
   }
}

void
WebAdmin::buildReloadCertsSubPage(resip::DataStream& s)
{
    mProxy.getStack().reloadCertificates();
    s << "<p class=\"notice ok\"><em>Reloaded certificates.</em></p>" << endl;
}

Data 
WebAdmin::buildUserPage()
{ 
   Data ret;
   {
      DataStream s(ret);
      
      s <<  "<!DOCTYPE html>" << endl
        <<    "<html lang=\"en\">" << endl
        <<    "<head>" << endl
        <<    "<meta charset=\"utf-8\" />" << endl
        <<    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />" << endl
        <<    "<title>Repro Proxy</title>" << endl
        <<    faviconLink()
        <<    standaloneStyle()
        <<    "</head>" << endl
        <<    "<body>" << endl
        <<    "<div class=\"card\">" << endl
        <<    "  <div class=\"brand\">" << productMarkSvg() << "<span class=\"name\">Repro</span></div>" << endl;

      //buildAddUserSubPage(s); // !cj! TODO - should do beter page here

      s <<    "</div>" << endl
        <<    "</body>" << endl
        <<    "</html>" << endl;
            
      s.flush();
   }
   return ret;
}

Data
WebAdmin::buildCertPage(const Data& domain)
{
   resip_assert(!domain.empty());
#ifdef USE_SSL
   resip_assert( mProxy.getStack().getSecurity() );
   return mProxy.getStack().getSecurity()->getDomainCertDER(domain);
#else
   ErrLog( << "Proxy not build with support for certificates" );
   return Data::Empty;
#endif
}

Data 
WebAdmin::buildDefaultPage()
{ 
   Data ret;
   {
      DataStream s(ret);
      
      s <<
         "<!DOCTYPE html>" << endl <<
         "<html lang=\"en\">" << endl <<
         "<head>" << endl <<
         "<meta charset=\"utf-8\" />" << endl <<
         "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />" << endl <<
         "<title>Repro Proxy Login</title>" << endl <<
         faviconLink() <<
         standaloneStyle() <<
         "</head>" << endl <<

         "<body>" << endl <<
         "  <div class=\"card\">" << endl <<
         "    <div class=\"brand\">" << productMarkSvg() << "<span class=\"name\">Repro</span></div>" << endl <<
         "    <div class=\"sub\">SIP Proxy &middot; Admin Console</div>" << endl <<
         "    <a class=\"cta\" href=\"user.html\">Log in</a>" << endl <<
         "    <p>Note: Web admin accounts are stored in a file (default filename is users.txt).  "
                "You can create it with the <a href=\"https://httpd.apache.org/docs/current/programs/htdigest.html\">htdigest</a> utility.</p>" << endl <<
         "  </div>" << endl <<
         "</body>" << endl <<
         "</html>" << endl;
      
      s.flush();
   }
   return ret;
}


void
WebAdmin::setApiResponse(int pageNumber, int statusCode, const Data& jsonBody)
{
   setPage(jsonBody, pageNumber, statusCode, Mime("application", "json"));
}

void
WebAdmin::handleStatisticsMessage(StatisticsMessage& statsMessage)
{
   // Called on the stack thread when a StatisticsMessage is delivered.
   // Copy the payload struct under the lock and wake any RestAdmin::handleStats
   // caller that is blocked waiting for fresh data. Serialization to JSON is
   // deferred to the REST handler so that the stack thread isn't doing any
   // more work here than necessary.
   {
      Lock lock(mStatsMutex); (void)lock;
      statsMessage.loadOut(mStatsPayload);
      mStatsReady = true;
   }
   mStatsCondition.notify_all();
}


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2026 SIP Spectrum, Inc. https://www.sipspectrum.com
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
