#include <cassert>
#include <time.h>

#include "resip/dum/RegistrationPersistenceManager.hxx"
#include "resip/stack/Symbols.hxx"
#include "resip/stack/Tuple.hxx"
#include "rutil/Data.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/MD5Stream.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/Socket.hxx"
#include "rutil/TransportType.hxx"

#include "repro/HttpBase.hxx"
#include "repro/HttpConnection.hxx"
#include "repro/WebAdmin.hxx"
#include "repro/RouteStore.hxx"
#include "repro/UserStore.hxx"
#include "repro/Store.hxx"


using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO


WebAdmin::RemoveKey::RemoveKey(const Data &key1, const Data &key2) : mKey1(key1), mKey2(key2) 
{
}; 

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

// !cj! TODO - make all the removes on the web pages work 

WebAdmin::WebAdmin(  Store& store,
                     RegistrationPersistenceManager& regDb,
                     Security* security,
                     bool noChal,  
                     const Data& realm, // this realm is used for http challenges
                     const Data& adminPassword,
                     int port, 
                     IpVersion version ):
   HttpBase( port, version, realm ),
   mStore(store),
   mRegDb(regDb),
   mSecurity(security),
   mNoWebChallenges( noChal ) 
{
      const Data adminName("admin");

      Data dbA1 = mStore.mUserStore.getUserAuthInfo( adminName, Data::Empty );
      
      DebugLog(<< " Looking to see if admin user exists (creating WebAdmin)");
      if ( dbA1.empty() ) // if the admin user does not exist, add it 
      { 
         DebugLog(<< "Creating admin user" );
         
         mStore.mUserStore.addUser( adminName, // user
                          Data::Empty, // domain 
                          Data::Empty, // realm 
                          (adminPassword==""?Data("admin"):adminPassword), // password 
                          true,        // applyA1HashToPassword
                          Data::Empty, // name 
                          Data::Empty ); // email 
         dbA1 = mStore.mUserStore.getUserAuthInfo( adminName, Data::Empty );
         assert( !dbA1.empty() );
      }
      else if (adminPassword!=Data(""))
      {
         //All we're using for admin is the password.
         //This next bit of code relies on it being ok that we 
         //blow away any other information
         //in that row. It also expects addUser to replace anything matching the existing key
         DebugLog(<< "Changing the web admin password" );
         mStore.mUserStore.addUser( adminName,
                                       Data::Empty,
                                       Data::Empty,
                                       adminPassword,
                                       true,        // applyA1HashToPassword
                                       Data::Empty,
                                       Data::Empty);
      }
}


void 
WebAdmin::buildPage( const Data& uri,
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

   // if this is not a valid page, redirect it
   if (
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
      ( pageName != Data("addRoute.html") ) && 
      ( pageName != Data("editRoute.html") ) &&
      ( pageName != Data("showRoutes.html") )&& 
      ( pageName != Data("registrations.html") ) &&  
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
         setPage( buildCertPage(domain), pageNumber, 200, Mime("application","pkix-cert") );
         return;
      }
      else
      {
         setPage( resip::Data::Empty, pageNumber, 404 );
         return;
      }
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
      Data dbA1 = mStore.mUserStore.getUserAuthInfo( pUser, Data::Empty );
      
#if 0
      if ( dbA1.empty() ) // if the admin user does not exist, add it 
      { 
         mStore.mUserStore.addUser( pUser, // user
                          Data::Empty, // domain 
                          Data::Empty, // realm 
                          Data("admin"), // password 
                          Data::Empty, // name 
                          Data::Empty ); // email 
         dbA1 = mStore.mUserStore.getUserAuthInfo( pUser, Data::Empty );
         assert( !dbA1.empty() );
      }
#endif

      if ( !dbA1.empty() )
      {
         MD5Stream a1;
         a1 << pUser // username
            << Symbols::COLON
            << Data::Empty // realm
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
      buildPageOutlinePre(s);
      
      // admin only pages 
	  if ( pageName == Data("user.html")    ) {}; /* do nothing */ 
      //if ( pageName == Data("input")    ) ; /* do nothing */ 
      if ( pageName == Data("domains.html")    ) buildDomainsSubPage(s);
      if ( pageName == Data("acls.html")       ) buildAclsSubPage(s);
      
      if ( pageName == Data("addUser.html")    ) buildAddUserSubPage(s);
      if ( pageName == Data("editUser.html")   ) buildEditUserSubPage(s);
      if ( pageName == Data("showUsers.html")  ) buildShowUsersSubPage(s);
      
      if ( pageName == Data("addRoute.html")   ) buildAddRouteSubPage(s);
      if ( pageName == Data("editRoute.html")  ) buildEditRouteSubPage(s);
      if ( pageName == Data("showRoutes.html") ) buildShowRoutesSubPage(s);
      
      if ( pageName == Data("registrations.html")) buildRegistrationsSubPage(s);
      
      buildPageOutlinePost(s);
      s.flush();

      if ( pageName == Data("userTest.html")   ) page=buildUserPage();
   }
   else if ( !authenticatedUser.empty() )
   {
      // user only pages 
      if ( pageName == Data("user.html") ) page=buildUserPage(); 
      //if ( pageName == Data("input") ) page=buildUserPage();
  }
   
   assert( !authenticatedUser.empty() );
   assert( !page.empty() );
   
   setPage( page, pageNumber,200 );
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
      s << "<p><em>Removed:</em> " << j << " records</p>" << endl;
   }
   
   Dictionary::iterator pos = mHttpParams.find("aclUri");
   if (pos != mHttpParams.end() && (mHttpParams["action"] == "Add")) // found 
   {
      Data hostOrIp = mHttpParams["aclUri"];
      int port = mHttpParams["aclPort"].convertInt();
      TransportType transport = Tuple::toTransport(mHttpParams["aclTransport"]);
      
      if (mStore.mAclStore.addAcl(hostOrIp, port, transport)){
         s << "<p><em>Added</em> trusted access for: " << hostOrIp << "</p>\n";
      }
      else {
         s << "<p>Error parsing: " << hostOrIp << "</p>\n";
      }
   }   
   
   s << 
      "      <form id=\"aclsForm\" method=\"get\" action=\"acls.html\" name=\"aclsForm\">" << endl <<
      "      <div class=space>" << endl <<
      "      </div>" << endl <<
      "        <table cellspacing=\"2\" cellpadding=\"0\">" << endl <<
      "          <tr>" << endl <<
      "            <td align=\"right\">Host or IP:</td>" << endl <<
      "            <td><input type=\"text\" name=\"aclUri\" size=\"24\"/></td>" << endl <<
      "            <td><input type=\"text\" name=\"aclPort\" value=\"0\" size=\"5\"/></td>" << endl <<
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
      "      <table border=\"1\" cellspacing=\"2\" cellpadding=\"2\">" << endl <<
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
      "     </form>" << endl <<
      
      "<pre>" << endl <<
      "      Input can be in any of these formats" << endl <<
      "      localhost         localhost  (becomes 127.0.0.1/8, ::1/128 and fe80::1/64)" << endl <<
      "      bare hostname     server1" << endl <<
      "      FQDN              server1.example.com" << endl <<
      "      IPv4 address      192.168.1.100" << endl <<
      "      IPv4 + mask       192.168.1.0/24" << endl <<
      "      IPv6 address      ::341:0:23:4bb:0011:2435:abcd" << endl <<
      "      IPv6 + mask       ::341:0:23:4bb:0011:2435:abcd/80" << endl <<
      "      IPv6 reference    [::341:0:23:4bb:0011:2435:abcd]" << endl <<
      "      IPv6 ref + mask   [::341:0:23:4bb:0011:2435:abcd]/64" << endl <<
      "</pre>" << endl <<
      
      "<p>Access lists are used as a whitelist to allow " << endl <<
      "gateways and other trusted nodes to skip authentication.</p>" << endl <<
      "<p>Note:  If hostnames or FQDN's are used then a TLS transport type is" << endl <<
      "assumed.  All other transport types must specify ACLs by address.</p>" << endl;
}


void
WebAdmin::buildDomainsSubPage(DataStream& s)
{ 
   Data domainUri;
   int domainTlsPort;

  if (!mRemoveSet.empty() && (mHttpParams["action"] == "Remove"))
   {
      int j = 0;
      for (set<RemoveKey>::iterator i = mRemoveSet.begin(); i != mRemoveSet.end(); ++i)
      {
         mStore.mConfigStore.eraseDomain(i->mKey1);
         ++j;
      }
      s << "<p><em>Removed:</em> " << j << " records</p>" << endl;
   }
   
   Dictionary::iterator pos = mHttpParams.find("domainUri");
   if (pos != mHttpParams.end() && (mHttpParams["action"] == "Add")) // found domainUri key
   {
      domainUri = pos->second;
      domainTlsPort = mHttpParams["domainTlsPort"].convertInt();
      mStore.mConfigStore.addDomain(domainUri,domainTlsPort);
      
      s << "<p><em>Added</em> domain: " << domainUri << "</p>" << endl;
   }   

   
   // TODO !cj! - make a web page after you add a domain that tells people they
   // have to restart the proxy 

   s <<
      "     <form id=\"domainForm\" method=\"get\" action=\"domains.html\" name=\"domainForm\">" << endl <<
      "        <table cellspacing=\"2\" cellpadding=\"0\">" << endl <<
      "          <tr>" << endl <<
      "            <td align=\"right\">New Domain:</td>" << endl <<
      "            <td><input type=\"text\" name=\"domainUri\" size=\"24\"/></td>" << endl <<
      "            <td><input type=\"text\" name=\"domainTlsPort\" size=\"4\"/></td>" << endl <<
      "            <td><input type=\"submit\" name=\"action\" value=\"Add\"/></td>" << endl <<
      "          </tr>" << endl <<
      "        </table>" << endl <<
      "      <div class=space>" << endl <<
      "        <br>" << endl <<
      "      </div>" << endl <<
      "      <table border=\"1\" cellspacing=\"2\" cellpadding=\"2\">" << endl <<
      "        <thead>" << endl <<
      "          <tr>" << endl <<
      "            <td>Domain</td>" << endl <<
      "            <td align=\"center\">TLS Port</td>" << endl <<
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
         "            <td align=\"center\">" << i->second.mTlsPort << "</td>" << endl <<
         "            <td><input type=\"checkbox\" name=\"remove." << i->second.mDomain << "\"/></td>" << endl <<
         "          </tr>" << endl;
   }
   
   s <<
      "        </tbody>" << endl <<
      "      </table>" << endl <<
      "     </form>" << endl;
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
      
      if (!routeDestination.empty())
      {
         mStore.mRouteStore.addRoute(mHttpParams["routeMethod"], 
                                     mHttpParams["routeEvent"], 
                                     routeUri,
                                     routeDestination,
                                     mHttpParams["routeOrder"].convertInt());
         
         // check if successful
         // {
               s << "<p><em>Added</em> route for: " << routeUri << "</p>\n";
         // }
      }
   }
   
   s << 
      "<form id=\"addRouteFrom\" method=\"get\" action=\"addRoute.html\" name=\"addRouteForm\">" << endl << 
      "<table border=\"1\" cellspacing=\"2\" cellpadding=\"0\">" << endl << 

      "<tr>" << endl << 
      "<td>URI</td>" << endl << 
      "<td><input type=\"text\" name=\"routeUri\" size=\"40\"/></td>" << endl << 
      "</tr>" << endl << 

      "<tr>" << endl << 
      "<td>Method</td>" << endl << 
      "<td><input type=\"text\" name=\"routeMethod\" size=\"40\"/></td>" << endl << 
      "</tr>" << endl << 
      
      "<tr>" << endl << 
      "<td>Event</td>" << endl << 
      "<td><input type=\"text\" name=\"routeEvent\" size=\"40\"/></td>" << endl << 
      "</tr>" << endl << 
      
      "<tr>" << endl << 
      "<td>Destination</td>" << endl << 
      "<td><input type=\"text\" name=\"routeDestination\" size=\"40\"/></td>" << endl << 
      "</tr>" << endl << 
      
      "<tr>" << endl << 
      "<td>Order</td>" << endl << 
      "<td><input type=\"text\" name=\"routeOrder\" size=\"4\"/></td>" << endl << 
      "</tr>" << endl << 

      "<tr>" << endl << 
      "  <td colspan=\"2\" align=\"right\" valign=\"middle\">" << endl << 
      "    <input type=\"reset\"  value=\"Cancel\"/>" << endl << 
      "    <input type=\"submit\" name=\"routeAdd\" value=\"Add\"/>" << endl << 
      "  </td>" << endl << 
      "</tr>" << endl << 

      "</table>" << endl << 
      "</form>" << endl <<

      "<pre>" << endl <<
      "Static routes use (POSIX-standard) regular expression to match" << endl <<
      "and rewrite SIP URIs.  The following is an example of sending" << endl <<
      "all requests that consist of only digits in the userpart of the" << endl <<
      "SIP URI to a gateway:" << endl << endl <<
      "   URI:         ^sip:([0-9]+)@example\\.com" << endl <<
      "   Destination: sip:$1@gateway.example.com" << endl <<
      "</pre>" << endl;
}


void
WebAdmin::buildAddUserSubPage( DataStream& s)
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
            
      mStore.mUserStore.addUser(user,domain,domain,mHttpParams["password"],true,mHttpParams["name"],mHttpParams["email"]);      
      // !rwm! TODO check if the add was successful
      // if (success)
      //{
            s << "<p><em>Added:</em> " << user << "@" << domain << "</p>\n";
      //}
   }

      s << 
         "<form id=\"addUserForm\" action=\"addUser.html\"  method=\"get\" name=\"addUserForm\" enctype=\"application/x-www-form-urlencoded\">" << endl << 
         "<table border=\"0\" cellspacing=\"2\" cellpadding=\"0\" align=\"left\">" << endl << 
         "<tr>" << endl << 
         "  <td align=\"right\" valign=\"middle\">User Name:</td>" << endl << 
         "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"user\" size=\"40\"/></td>" << endl << 
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
         "  <td align=\"left\" valign=\"middle\"><input type=\"password\" name=\"password\" size=\"40\"/></td>" << endl << 
         "</tr>" << endl << 

         "<tr>" << endl << 
         "  <td align=\"right\" valign=\"middle\" >Full Name:</td>" << endl << 
         "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"name\" size=\"40\"/></td>" << endl << 
         "</tr>" << endl << 

         "<tr>" << endl << 
         "  <td align=\"right\" valign=\"middle\" >Email:</td>" << endl << 
         "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"email\" size=\"40\"/></td>" << endl << 
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
WebAdmin::buildRegistrationsSubPage(DataStream& s)
{
   if (!mRemoveSet.empty())
   {
      int j = 0;
      for (set<RemoveKey>::iterator i = mRemoveSet.begin(); i != mRemoveSet.end(); ++i)
      {
         Uri key(i->mKey1);
         Uri contact(i->mKey2);
         mRegDb.removeContact(key, contact);
         ++j;
      }
      s << "<p><em>Removed:</em> " << j << " records</p>" << endl;
   }

   s << 
       "<form id=\"showReg\" method=\"get\" action=\"registrations.html\" name=\"showReg\" enctype=\"application/x-www-form-urlencoded\">" << endl << 
      //"<button name=\"removeAllReg\" value=\"\" type=\"button\">Remove All</button>" << endl << 
      //"<hr/>" << endl << 

      "<table border=\"1\" cellspacing=\"2\" cellpadding=\"0\" align=\"left\">" << endl << 

      "<tr>" << endl << 
      "  <td>AOR</td>" << endl << 
      "  <td>Contact</td>" << endl << 
      "  <td>Expires In</td>" << endl << 
      "  <td><input type=\"submit\" value=\"Remove\"/></td>" << endl << 
      "</tr>" << endl;
  
      RegistrationPersistenceManager::UriList aors = mRegDb.getAors();
      for ( RegistrationPersistenceManager::UriList::const_iterator 
               aor = aors.begin(); aor != aors.end(); ++aor )
      {
         Uri uri = *aor;
         RegistrationPersistenceManager::ContactRecordList 
            contacts = mRegDb.getContacts(uri);
         
         bool first = true;
         for (RegistrationPersistenceManager::ContactRecordList::iterator i = contacts.begin();
              i != contacts.end(); ++i )
         {
            if (i->expires >= time(NULL))
            {
               s << "<tr>" << endl
                 << "  <td>" ;
               if (first) 
               { 
                  s << uri;
                  first = false;
               }
               s << "</td>" << endl
                 << "  <td>";
            
               const RegistrationPersistenceManager::ContactRecord& r = *i;
               const Uri& contact = r.uri; 

               s << contact;
               s <<"</td>" << endl 
                 <<"<td>" << i->expires - time(NULL) << "s</td>" << endl
                 << "  <td>"
                 << "<input type=\"checkbox\" name=\"remove." << uri << "\" value=\"" << contact
                 << "\"/></td>" << endl
                 << "</tr>" << endl;
            }
            else
            {
               // remove expired contact 
               mRegDb.removeContact(uri, i->uri);
            }
         }
      }
                  
      s << "</table>" << endl << 
         "</form>" << endl;
}


void
WebAdmin::buildEditUserSubPage( DataStream& s)
{
   Dictionary::iterator pos;
   pos = mHttpParams.find("key");
   if (pos != mHttpParams.end()) 
   {
      Data key = pos->second;
      AbstractDb::UserRecord rec = mStore.mUserStore.getUserInfo(key);
      // !rwm! TODO check to see if we actually found a record corresponding to the key.  how do we do that?
      
      s << "<p>Editing Record with key: " << key << "</p>" << endl <<
           "<p>Note:  If the username is not modified and you leave the password field empty the users current password will not be reset.</p>" << endl;
      
      s << 
         "<form id=\"editUserForm\" action=\"showUsers.html\"  method=\"get\" name=\"editUserForm\" enctype=\"application/x-www-form-urlencoded\">" << endl << 
         "<table border=\"0\" cellspacing=\"2\" cellpadding=\"0\" align=\"left\">" << endl << 
         "<input type=\"hidden\" name=\"key\" value=\"" << key << "\"/>" << endl << 
         "<tr>" << endl << 
         "  <td align=\"right\" valign=\"middle\">User Name:</td>" << endl << 
         "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"user\" value=\"" << rec.user << "\" size=\"40\"/></td>" << endl << 
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
         "  <td align=\"left\" valign=\"middle\"><input type=\"password\" name=\"password\" size=\"40\"/></td>" << endl << 
         "</tr>" << endl << 
         // Note that the UserStore only stores a passwordHash, so we will collect a password.  If one is provided in the
         // edit page, we will use it to generate a new passwordHash, otherwise we will leave the hash alone.
         
         "<tr>" << endl << 
         "  <td align=\"right\" valign=\"middle\" >Full Name:</td>" << endl << 
         "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"name\" value=\"" << rec.name << 
         "\" size=\"40\"/></td>" << endl << 
         "</tr>" << endl << 

         "<tr>" << endl << 
         "  <td align=\"right\" valign=\"middle\" >Email:</td>" << endl << 
         "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"email\" value=\"" << rec.email <<
         "\" size=\"40\"/></td>" << endl << 
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
WebAdmin::buildEditRouteSubPage(DataStream& s)
{
   Dictionary::iterator pos;
   pos = mHttpParams.find("key");
   if (pos != mHttpParams.end()) 
   {
      Data key = pos->second;

      // !rwm! TODO check to see if we actually found a record corresponding to the key.  how do we do that?
      DebugLog( << "Creating page to edit route " << key );
      
      s << "<p>Editing Record with matching pattern: " << mStore.mRouteStore.getRoutePattern(key) << "</p>" << endl;      

      s << 
      "<form id=\"editRouteForm\" method=\"get\" action=\"showRoutes.html\" name=\"editRouteForm\">" << endl << 
      "<table border=\"1\" cellspacing=\"2\" cellpadding=\"0\">" << endl << 
      "<input type=\"hidden\" name=\"key\" value=\"" << key << "\"/>" << endl << 
      "<tr>" << endl << 
      "<td>URI</td>" << endl << 
      "<td><input type=\"text\" name=\"routeUri\" value=\"" <<  mStore.mRouteStore.getRoutePattern(key) << "\" size=\"40\"/></td>" << endl << 
      "</tr>" << endl << 

      "<tr>" << endl << 
      "<td>Method</td>" << endl << 
      "<td><input type=\"text\" name=\"routeMethod\" value=\"" <<  mStore.mRouteStore.getRouteMethod(key)  << "\" size=\"40\"/></td>" << endl << 
      "</tr>" << endl << 
      
      "<tr>" << endl << 
      "<td>Event</td>" << endl << 
      "<td><input type=\"text\" name=\"routeEvent\" value=\"" << mStore.mRouteStore.getRouteEvent(key)  << "\" size=\"40\"/></td>" << endl << 
      "</tr>" << endl << 
      
      "<tr>" << endl << 
      "<td>Destination</td>" << endl << 
      "<td><input type=\"text\" name=\"routeDestination\" value=\"" << mStore.mRouteStore.getRouteRewrite(key)  <<
                            "\" size=\"40\"/></td>" << endl << 
      "</tr>" << endl << 
      
      "<tr>" << endl << 
      "<td>Order</td>" << endl << 
      "<td><input type=\"text\" name=\"routeOrder\" value=\"" << mStore.mRouteStore.getRouteOrder(key)  <<
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
      s << "<p><em>Removed:</em> " << j << " records</p>" << endl;
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
         Data name = mHttpParams["name"];
         Data email = mHttpParams["email"];
         bool applyA1HashToPassword = true;
         
         // if no password was specified, then leave current password untouched
         if(password == "" && user == rec.user && realm == rec.realm) 
         {
            password = rec.passwordHash;
            applyA1HashToPassword = false;
         }
         // write out the updated record to the database now
         mStore.mUserStore.updateUser(key, user, domain, realm, password, applyA1HashToPassword, name, email );
         
         s << "<p><em>Updated:</em> " << key << "</p>" << endl; 
      }
   }
   
      
      s << 
         //"<h1>Users</h1>" << endl << 
         "<form id=\"showUsers\" method=\"get\" action=\"showUsers.html\" name=\"showUsers\" enctype=\"application/x-www-form-urlencoded\">" << endl << 
         "<table border=\"1\" cellspacing=\"2\" cellpadding=\"0\" align=\"left\">" << endl << 
         "<tr>" << endl << 
         "  <td>User@Domain</td>" << endl << 
         //  "  <td>Realm</td>" << endl << 
         "  <td>Name</td>" << endl << 
         "  <td>Email</td>" << endl << 
         "  <td><input type=\"submit\" value=\"Remove\"/></td>" << endl << 
         "</tr>" << endl;
      
      s << endl;
      
      int count =0;
      
      key = mStore.mUserStore.getFirstKey();
      while ( !key.empty() )
      {
         rec = mStore.mUserStore.getUserInfo(key);

         if ((rec.domain == Data::Empty) && (rec.user == "admin"))
         {
            key = mStore.mUserStore.getNextKey();
            continue;   // skip the row for the admin web user
         }
      
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
         s << "<tr><td>Only first 1000 users were displayed<td></tr>" << endl;
      }
      
      s << 
         "</table>" << endl << 
         "</form>" << endl;
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
      s << "<p><em>Removed:</em> " << j << " records</p>" << endl;
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
            mStore.mRouteStore.updateRoute(key, method,event,matchingPattern,rewriteExpression,order  );
         
            s << "<p><em>Updated:</em> " << rec.mMatchingPattern << "</p>" << endl; 
         }
      }
   }

   s <<
      "    <table border=\"0\" cellspacing=\"2\" cellpadding=\"0\" align=\"left\">" << endl << 
      //"      <tr>" << endl << 
      // "        <td>" << endl << 
      //"          <h1>Routes</h1>" << endl << 
      //"        </td>" << endl << 
      //"      </tr>" << endl <<  
      "      <tr>" << endl << 
      "        <td>" << endl <<  
      "          <form id=\"showRoutes\" action=\"showRoutes.html\" method=\"get\" name=\"showRoutes\" enctype=\"application/x-www-form-urlencoded\">" << endl << 
      // "            <button name=\"removeAllRoute\" value=\"\" type=\"submit\">Remove All</button>" << endl << 
      "            <hr/>" << endl << 
      "            <table border=\"1\" cellspacing=\"2\" cellpadding=\"0\" align=\"left\">" << endl << 
      "              <thead><tr>" << endl << 
      "                <td>URI</td>" << endl << 
      "                <td>Method</td>" << endl << 
      "                <td>Event</td>" << endl << 
      "                <td>Destination</td>" << endl << 
      "                <td>Order</td>" << endl << 
      "                <td><input type=\"submit\" value=\"Remove\"/></td>" << endl << 
      "              </tr></thead>" << endl << 
      "              <tbody>" << endl;
   
   for ( RouteStore::Key key = mStore.mRouteStore.getFirstKey();
         !key.empty();
         key = mStore.mRouteStore.getNextKey(key) )
   {
      s <<  "<tr>" << endl << 
         "<td><a href=\"editRoute.html?key=";
            key.urlEncode(s); 
      s << 
         "\">" << mStore.mRouteStore.getRoutePattern(key) << "</a></td>" << endl << 
         "<td>" << mStore.mRouteStore.getRouteMethod(key) << "</td>" << endl << 
         "<td>" << mStore.mRouteStore.getRouteEvent(key) << "</td>" << endl << 
         "<td>" << mStore.mRouteStore.getRouteRewrite(key) << "</td>" << endl << 
         "<td>" << mStore.mRouteStore.getRouteOrder(key) << "</td>" << endl << 
         "<td><input type=\"checkbox\" name=\"remove." <<  key << "\"/></td>" << endl << 
         "</tr>" << endl;
   }
   
   s << 
      "              </tbody>" << endl << 
      "            </table>" << endl << 
      "          </form>" << endl << 
      "        </td>" << endl << 
      "      </tr>" << endl << 
      "      <tr><td>" << endl << 
      "          <hr noshade=\"noshade\"/>" << endl << 
      "          <br>" << endl << 
      "        </td></tr>" << endl;

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
      
   // !cj! - TODO - shoudl input method and envent type to test 
   RouteStore::UriList routeList;
   if ( !badUri )
   {
      routeList = mStore.mRouteStore.process( uri, Data("INVITE"), Data::Empty);
   }
   
   s << 
      "      <tr>" << endl << 
      "        <td>" << endl << 
      "          <form id=\"testRoute\" action=\"showRoutes.html\" method=\"get\" name=\"testRoute\" enctype=\"application/x-www-form-urlencoded\">" << endl << 
      "            <table border=\"0\" cellspacing=\"2\" cellpadding=\"0\">" << endl << 
      "              "
      "              <tr>" << endl << 
      "                <td align=\"right\">Input:</td>" << endl << 
      "                <td><input type=\"text\" name=\"routeTestUri\" value=\"" << uri << "\" size=\"40\"/></td>" << endl << 
      "                <td><input type=\"submit\" name=\"testRoute\" value=\"Test Route\"/></td>" << endl << 
      "              </tr>" << endl;
   
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
   
   s<<
      "            </table>" << endl << 
      "          </form>" << endl << 
      "        </td>" << endl << 
      "      </tr>" << endl << 
      "    </table>" << endl;      
 
}


Data
WebAdmin::buildCertPage(const Data& domain)
{
	assert(!domain.empty());
#ifdef USE_SSL
	assert( mSecurity );
	return mSecurity->getDomainCertDER(domain);
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
         "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << endl << 
         "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">" << endl << 
         "<html xmlns=\"http://www.w3.org/1999/xhtml\">" << endl << 
         "<head>" << endl << 
         "<meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\" />" << endl << 
         "<title>Repro Proxy Login</title>" << endl << 
         "</head>" << endl << 

         "<body bgcolor=\"#ffffff\">" << endl << 
         "  <h1><a href=\"user.html\">Login</a></h1>" << endl << 
         "  <p>The default account is 'admin' with password 'admin', but if you're wise, you've already changed that using the command line</p>" << endl << 
         "</body>" << endl << 
         "</html>" << endl;
      
      s.flush();
   }
   return ret;
}


void
WebAdmin::buildPageOutlinePre(DataStream& s)
{
   s << 
#include "repro/webadmin/pageOutlinePre.ixx"
   ;
}


void
WebAdmin::buildPageOutlinePost(DataStream& s)
{
   s << 
#include "repro/webadmin/pageOutlinePost.ixx"   
   ;
}


Data 
WebAdmin::buildUserPage()
{ 
   Data ret;
   {
      DataStream s(ret);
      
      s <<  "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << endl
        <<    "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">" << endl
        <<    "" << endl
        <<    "<html xmlns=\"http://www.w3.org/1999/xhtml\">" << endl
        <<    "" << endl
        <<    "<head>" << endl
        <<    "<meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\" />" << endl
        <<    "<title>Repro Proxy</title>" << endl
        <<    "</head>" << endl
        <<    "" << endl
        <<    "<body bgcolor=\"#ffffff\">" << endl;
      
      //buildAddUserSubPage(s); // !cj! TODO - should do beter page here 
      
      s <<    "</body>" << endl
        <<    "" << endl
        <<    "</html>" << endl;
            
      s.flush();
   }
   return ret;
}



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
