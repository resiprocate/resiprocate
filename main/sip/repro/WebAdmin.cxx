#ifdef WIN32
#include <db_cxx.h>
#else 
#include <db4/db_cxx.h>
#endif

#include <cassert>

#include "resiprocate/Symbols.hxx"

#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/TransportType.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Tuple.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/os/ParseBuffer.hxx"
#include "resiprocate/os/MD5Stream.hxx"

//#include "resiprocate/dum/ServerAuthManager.hxx"
#include "resiprocate/dum/RegistrationPersistenceManager.hxx"

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


// !cj! TODO - make all the removes on the web pages work 

WebAdmin::WebAdmin(  Store& store,
                     RegistrationPersistenceManager& regDb,
                     Security* security,
                     bool noChal, 
                     int port, 
                     IpVersion version,
                     const Data& realm ):
   HttpBase( port, version, realm ),
   mStore(store),
   mRegDb(regDb),
   mSecurity(security),
   mNoWebChallenges( noChal ) 
{
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
      ( pageName != Data("certs") ) && 
      ( pageName != Data("userTest.html") ) && 
      ( pageName != Data("domains.html")  ) &&
      ( pageName != Data("acls.html")  ) &&
      ( pageName != Data("addUser.html") ) && 
      ( pageName != Data("showUsers.html")  ) &&
      ( pageName != Data("addRoute.html") ) && 
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
   if ( pageName == Data("cert") )
   {
      if ( !mRealm.empty() ) // !cj! this is goofy using the realm 
      {
         setPage( buildCertPage(mRealm), pageNumber, 200 );
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
      // all pages after this, user must authenticate  
      if ( pUser.empty() )
      {  
         setPage( resip::Data::Empty, pageNumber,401 );
         return;
      }
      
      // check that authentication is correct 
      Data dbA1 = mStore.mUserStore.getUserAuthInfo( pUser, Data::Empty );
      
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

      assert( !dbA1.empty() );
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
            InfoLog( << "user " << pUser << " failed to authneticate to web server" );
            setPage( resip::Data::Empty, pageNumber,401 );
            return;
         }
      }
      else
      {
         // !cj! TODO - fix this one up 
         ErrLog( << "user " << pUser << " failed creation of inital account" );
         assert(0);
      }
   }
   
   // parse any URI tags from form entry 
   Data domain;
   if (!pb.eof())
   {
      pb.skipChar('?');
      
      // keys to add user
      Data user;
      Data realm;
      Data password;
      Data name;
      Data email;

      // keys to add route 
      Data routeUri; // TODO !cj! name suck - this should be route match pattern 
      Data routeMethod;
      Data routeEvent;
      Data routeDestination; // name suck - this should be route rewrite
                             // expression 
      int routeOrder;
      
      Data domainUri;
      int  domainTlsPort;
      
      Data aclUri;
      
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
           
         DebugLog (<< "  key=" << key << " value=" << value << " & unencoded form: " << value.charHttpUnencoded() );

         if ( key == Data("routeTestUri") )
         {
            routeTestUri = value.charHttpUnencoded();
         }

         if ( key == Data("user") )
         {
            user = value.charHttpUnencoded();
         }
         if ( key == Data("password") )
         {
            password = value.charHttpUnencoded();
         }
         if ( key == Data("realm") )
         {
            realm = value.charHttpUnencoded();
         }
         if ( key == Data("name") )
         {
            name = value.charHttpUnencoded();
         }
         if ( key == Data("email") )
         {
            email = value.charHttpUnencoded();
         }
         if ( key == Data("domain") )
         {
            domain = value.charHttpUnencoded();
         }

         if ( key == Data("routeUri") )
         {
            routeUri = value.charHttpUnencoded();
         }
         if ( key == Data("routeMethod") )
         {
            routeMethod = value.charHttpUnencoded();
         }
         if ( key == Data("routeEvent") )
         {
            routeEvent = value.charHttpUnencoded();
         }
         if ( key == Data("routeDestination") )
         {
            routeDestination = value.charHttpUnencoded();
         }
         if ( key == Data("routeOrder") )
         {
            routeOrder = value.convertInt();
         }
         
         if ( key == Data("aclUri") )
         {
            aclUri = value.charHttpUnencoded();
         }
         if ( key == Data("domainUri") )
         {
            domainUri = value.charHttpUnencoded();
         }
         if ( key == Data("domainTlsPort") )
         {
            domainTlsPort = value.convertInt();
         }
      }

      // must be admin to do this 
      if ( authenticatedUser == "admin")
      {
         if ( !user.empty() )
         {
            if ( realm.empty() )
            {
               realm = domain;
            }
            
            mStore.mUserStore.addUser(user,domain,realm,password,name,email);
         }
         
         if ( !routeDestination.empty() )
         {
            mStore.mRouteStore.add(routeMethod ,
                                   routeEvent ,
                                   routeUri, 
                                   routeDestination, 
                                   routeOrder );
         }

         if ( !domainUri.empty() )
         {
            mStore.mConfigStore.add(domainUri,domainTlsPort);
         }

         if ( !aclUri.empty() )
         {
            mStore.mAclStore.add(aclUri);
         }
      }
   }
   
   if (  pageName == Data("input")  )
   {
      setPage( resip::Data::Empty, pageNumber, 301 );
      return;
   }

   DebugLog( << "building page for user=" << authenticatedUser  );

   Data page;
   if ( authenticatedUser == Data("admin") )
   {
      DataStream s(page);
      buildPageOutlinePre(s);
      
      // admin only pages 
      if ( pageName == Data("user.html")    ) ; /* do nothing */ 
      if ( pageName == Data("domains.html")    ) buildDomainsSubPage(s);
      if ( pageName == Data("acls.html")       ) buildAclsSubPage(s);
      
      if ( pageName == Data("addUser.html")    ) buildAddUserSubPage(s);
      if ( pageName == Data("showUsers.html")  ) buildShowUsersSubPage(s);
      
      if ( pageName == Data("addRoute.html")   ) buildAddRouteSubPage(s);
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
   }

   assert( !authenticatedUser.empty() );
   assert( !page.empty() );
   
   setPage( page, pageNumber,200 );
}
  

void
WebAdmin::buildAclsSubPage(DataStream& s)
{ 
   s << 
      "      <form id=\"addRouteFrom\" method=\"get\" action=\"input\" name=\"addRouteForm\">" << endl <<
      "        <table cellspacing=\"2\" cellpadding=\"0\">" << endl <<
      "          <tr>" << endl <<
      "            <td align=\"right\">Host or IP:</td>" << endl <<
      "            <td><input type=\"text\" name=\"aclUri\" size=\"24\"/></td>" << endl <<
      //    "            <td><input type=\"text\" name=\"aclMask\" size=\"2\"/></td>" << endl <<
      "            <td><input type=\"submit\" name=\"aclAdd\" value=\"Add\"/></td>" << endl <<
      "          </tr>" << endl <<
      "        </table>" << endl <<
      "      </form>" << endl <<
      "      <div class=sace>" << endl <<
      "        <br />" << endl <<
      "      </div>" << endl <<
      "      <table border=\"1\" cellspacing=\"2\" cellpadding=\"2\">" << endl <<
      "        <thead>" << endl <<
      "          <tr>" << endl <<
      "            <td>Host</td>" << endl <<
      //  "            <td align=\"center\">Mask</td>" << endl <<
      "          </tr>" << endl <<
      "        </thead>" << endl <<
      "        <tbody>" << endl;
   
   AbstractDb::AclRecordList list = mStore.mAclStore.getAcls();
   
   for (AbstractDb::AclRecordList::iterator i = list.begin();
        i != list.end(); i++ )
   {
      s << 
         "          <tr>" << endl <<
         "            <td>" << i->mMachine << "</td>" << endl <<
         //   "            <td align=\"center\">" <<  "</td>" << endl <<
         "          </tr>" << endl;
   }
   
   s <<  
      "          </tr>" << endl <<
      "        </tbody>" << endl <<
      "      </table>" << endl;
}


void
WebAdmin::buildDomainsSubPage(DataStream& s)
{ 
   s <<
      "     <form id=\"addRouteFrom\" method=\"get\" action=\"input\" name=\"addRouteForm\">" << endl <<
      "        <table cellspacing=\"2\" cellpadding=\"0\">" << endl <<
      "          <tr>" << endl <<
      "            <td align=\"right\">New Domain:</td>" << endl <<
      "            <td><input type=\"text\" name=\"domainUri\" size=\"24\"/></td>" << endl <<
      "            <td><input type=\"text\" name=\"domainTlsPort\" size=\"4\"/></td>" << endl <<
      "            <td><input type=\"submit\" name=\"domainAdd\" value=\"Add\"/></td>" << endl <<
      "          </tr>" << endl <<
      "        </table>" << endl <<
      "      </form>" << endl <<
      "      <div class=sace>" << endl <<
      "        <br />" << endl <<
      "      </div>" << endl <<
      "      <table border=\"1\" cellspacing=\"2\" cellpadding=\"2\">" << endl <<
      "        <thead>" << endl <<
      "          <tr>" << endl <<
      "            <td>Domain</td>" << endl <<
      "            <td align=\"center\">TLS Port</td>" << endl <<
      "          </tr>" << endl <<
      "        </thead>" << endl <<
      "        <tbody>" << endl;
   
   AbstractDb::ConfigRecordList list = mStore.mConfigStore.getConfigs();
   
   for (AbstractDb::ConfigRecordList::iterator i = list.begin();
        i != list.end(); i++ )
   {
      s << 
         "          <tr>" << endl <<
         "            <td>" << i->mDomain << "</td>" << endl <<
         "            <td align=\"center\">" << i->mTlsPort << "</td>" << endl <<
         "          </tr>" << endl;
   }
   
   s <<
      "        </tbody>" << endl <<
      "      </table>" << endl;
}

void
WebAdmin::buildAddRouteSubPage(DataStream& s)
{
   s << 
      "<form id=\"addRouteFrom\" method=\"get\" action=\"input\" name=\"addRouteForm\">" << endl << 
      "<table width=\"122\" border=\"1\" cellspacing=\"2\" cellpadding=\"0\">" << endl << 

      "<tr>" << endl << 
      "<td>URI</td>" << endl << 
      "<td><input type=\"text\" name=\"routeUri\" size=\"24\"/></td>" << endl << 
      "</tr>" << endl << 

      "<tr>" << endl << 
      "<td>Method</td>" << endl << 
      "<td><input type=\"text\" name=\"routeMethod\" size=\"24\"/></td>" << endl << 
      "</tr>" << endl << 
      
      "<tr>" << endl << 
      "<td>Event</td>" << endl << 
      "<td><input type=\"text\" name=\"routeEvent\" size=\"24\"/></td>" << endl << 
      "</tr>" << endl << 
      
      "<tr>" << endl << 
      "<td>Destination</td>" << endl << 
      "<td><input type=\"text\" name=\"routeDestination\" size=\"24\"/></td>" << endl << 
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
      "</form>" << endl;
}


void
WebAdmin::buildAddUserSubPage( DataStream& s)
{
      s << 
         "<form id=\"addUserForm\" action=\"input\"  method=\"get\" name=\"addUserForm\" enctype=\"application/x-www-form-urlencoded\">" << endl << 
         "<table border=\"0\" cellspacing=\"2\" cellpadding=\"0\" align=\"left\">" << endl << 
         "<tr>" << endl << 
         "  <td align=\"right\" valign=\"middle\">User Name:</td>" << endl << 
         "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"user\" size=\"24\"/></td>" << endl << 
         "</tr>" << endl << 

         //"<tr>" << endl << 
         //"<td align=\"right\" valign=\"middle\" >Realm:</td>" << endl << 
         //"<td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"realm\" size=\"24\"/></td>" << endl << 
         //"</tr>" << endl << 

         "<tr>" << endl << 
         "  <td align=\"right\" valign=\"middle\" >Domain:</td>" << endl << 
         "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"domain\" size=\"24\"/></td>" << endl << 
         "</tr>" << endl << 

         "<tr>" << endl << 
         "  <td align=\"right\" valign=\"middle\" >Password:</td>" << endl << 
         "  <td align=\"left\" valign=\"middle\"><input type=\"password\" name=\"password\" size=\"24\"/></td>" << endl << 
         "</tr>" << endl << 

         "<tr>" << endl << 
         "  <td align=\"right\" valign=\"middle\" >Full Name:</td>" << endl << 
         "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"name\" size=\"24\"/></td>" << endl << 
         "</tr>" << endl << 

         "<tr>" << endl << 
         "  <td align=\"right\" valign=\"middle\" >Email:</td>" << endl << 
         "  <td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"email\" size=\"24\"/></td>" << endl << 
         "</tr>" << endl << 

         "<tr>" << endl << 
         "  <td colspan=\"2\" align=\"right\" valign=\"middle\">" << endl << 
         "    <input type=\"reset\" value=\"Cancel\"/>" << endl << 
         "    <input type=\"submit\" name=\"submit\" value=\"OK\"/>" << endl << 
         "  </td>" << endl << 
         "</tr>" << endl << 
         
         "</table>" << endl << 
         "</form>" << endl << 
         " ";
}


void
WebAdmin::buildRegistrationsSubPage(DataStream& s)
{
   s << 
      "<form id=\"showReg\" method=\"get\" action=\"input\" name=\"showReg\" enctype=\"application/x-www-form-urlencoded\">" << endl << 
      //"<button name=\"removeAllReg\" value=\"\" type=\"button\">Remove All</button>" << endl << 
      //"<hr/>" << endl << 

      "<table border=\"1\" cellspacing=\"2\" cellpadding=\"0\" align=\"left\">" << endl << 

      "<tr>" << endl << 
      "  <td>AOR</td>" << endl << 
      "  <td>Contact</td>" << endl << 
      "  <td><button name=\"removeReg\" type=\"button\">Remove</button></td>" << endl << 
      "</tr>" << endl;
   
      RegistrationPersistenceManager::UriList aors = mRegDb.getAors();
      for ( RegistrationPersistenceManager::UriList::const_iterator 
               aor = aors.begin(); aor != aors.end(); ++aor )
      {
         Uri uri = *aor;
         RegistrationPersistenceManager::ContactPairList 
            contacts = mRegDb.getContacts(uri);
         
         bool first = true;
         for (RegistrationPersistenceManager::ContactPairList::const_iterator i 
                 = contacts.begin();
              i != contacts.end(); ++i )
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
            
            const RegistrationPersistenceManager::ContactPair& p = *i;
            const Uri& contact = p.first; 
         
            s << contact;
            s <<"</td>" << endl 
              << "  <td><input type=\"checkbox\" name=\"removeUser\" value=\""
              << uri
              << "\"/></td>" << endl
              << "</tr>" << endl;
         }
      }
                  
      s << "</table>" << endl << 
         "</form>" << endl;
}


void 
WebAdmin::buildShowUsersSubPage(DataStream& s)
{
      
      s << 
         //"<h1>Users</h1>" << endl << 
         "<form id=\"showUsers\" method=\"get\" action=\"input\" name=\"showUsers\" enctype=\"application/x-www-form-urlencoded\">" << endl << 
         "<table width=\"196\" border=\"1\" cellspacing=\"2\" cellpadding=\"0\" align=\"left\">" << endl << 
         "<tr>" << endl << 
         "  <td>User</td>" << endl << 
         "  <td>Realm</td>" << endl << 
         "  <td>Name</td>" << endl << 
         "  <td>Email</td>" << endl << 
         "  <td><button name=\"buttonName\" type=\"button\">Remove</button></td>" << endl << 
         "</tr>" << endl;
      
/*
  "<tr>"
  "<td>fluffy</td>"
  "<td>example.com</td>"
  "<td>Cullen Jennings</td>"
  "<td>fluffy@example.com</td>"
  "<td><input type=\"checkbox\" name=\"removeUser\" value=\"removeUser\"/></td>"
  "</tr>"
*/
      s << endl;
      
      Data key = mStore.mUserStore.getFirstKey();
      while ( !key.empty() )
      {
         s << "<tr>" << endl 
           << "  <td>" << key << "</td>" << endl
           << "  <td> </td>" << endl
           << "  <td> </td>" << endl
           << "  <td> </td>" << endl
           << "  <td><input type=\"checkbox\" name=\"removeUser\" value=\""
           << key
           << "\"/></td>" << endl 
           << "</tr>" << endl;
         
         key = mStore.mUserStore.getNextKey();
      }
      
      s << 
         "</table>" << endl << 
         "</form>" << endl;
         ;
}


void
WebAdmin::buildShowRoutesSubPage(DataStream& s)
{ 
      
      s <<
         "    <table border=\"0\" cellspacing=\"2\" cellpadding=\"0\" align=\"left\">" << endl << 
         //"      <tr>" << endl << 
         // "        <td>" << endl << 
         //"          <h1>Routes</h1>" << endl << 
         //"        </td>" << endl << 
         //"      </tr>" << endl <<  
         "      <tr>" << endl << 
         "        <td>" << endl <<  
         "          <form id=\"showReg\" action=\"input\" method=\"get\" name=\"showReg\" enctype=\"application/x-www-form-urlencoded\">" << endl << 
         "            <button name=\"removeAllRoute\" value=\"\" type=\"submit\">Remove All</button>" << endl << 
         "            <hr/>" << endl << 
         "            <table border=\"1\" cellspacing=\"2\" cellpadding=\"0\" align=\"left\">" << endl << 
         "              <thead><tr>" << endl << 
         "                <td>URI</td>" << endl << 
         "                <td>Method</td>" << endl << 
         "                <td>Event</td>" << endl << 
         "                <td>Destination</td>" << endl << 
         "                <td>Order</td>" << endl << 
         "                <td><button name=\"removeRoute\" type=\"submit\">Remove</button></td>" << endl << 
         "              </tr></thead>" << endl << 
         "              <tbody>" << endl;
      
      AbstractDb::RouteRecordList routes = mStore.mRouteStore.getRoutes();
      for ( AbstractDb::RouteRecordList::const_iterator i = routes.begin();
            i != routes.end();
            i++ )
      {
         s <<  "<tr>" << endl << 
            "<td>" << i->mMatchingPattern << "</td>" << endl << 
            "<td>" << i->mMethod << "</td>" << endl << 
            "<td>" << i->mEvent << "</td>" << endl << 
            "<td>" << i->mRewriteExpression << "</td>" << endl << 
            "<td>" << i->mOrder << "</td>" << endl << 
            "<td><input type=\"checkbox\" name=\"removeRoute\" value=\"" << "TODO" << "\"/></td>" << endl << 
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

      Uri uri;
      try 
      {
         uri = Uri(routeTestUri);
      }
      catch( BaseException& e )
      {
         try 
         {
            uri = Uri( Data("sip:")+routeTestUri );
         }
         catch( BaseException& e )
         {
         }
      }

      // !cj! - TODO - shoudl input method and envent type to test 
      RouteStore::UriList routeList = mStore.mRouteStore.process( uri, Data("INVITE"), Data::Empty);
      
      s << 
         "      <tr>" << endl << 
         "        <td>" << endl << 
         "          <form id=\"testRoute\" action=\"showRoutes.html\" method=\"get\" name=\"testRoute\" enctype=\"application/x-www-form-urlencoded\">" << endl << 
         "            <table border=\"0\" cellspacing=\"2\" cellpadding=\"0\">" << endl << 
         "              "
         "              <tr>" << endl << 
         "                <td align=\"right\">Input:</td>" << endl << 
         "                <td><input type=\"text\" name=\"routeTestUri\" value=\"" << uri << "\" size=\"24\"/></td>" << endl << 
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
         "  <p>The default account is 'admin' with password 'admin'</p>" << endl << 
         "</body>" << endl << 
         "</html>" << endl;
      
      s.flush();
   }
   return ret;
}


void
WebAdmin::buildPageOutlinePre(DataStream& s)
{
   s << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << endl
     << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">" << endl
     << "<html xmlns=\"http://www.w3.org/1999/xhtml\">" << endl
     << "  <head>" << endl
     << "    <meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\" />" << endl
     << "    <title>Repro Proxy</title>" << endl
     << "  </head>" << endl
     << "  <style>" << endl
     << "body         { bgcolor: white; font-size: 90%; font-family: Arial, Helvetica, sans-serif }" << endl
     << "h1           { font-size: 200%; font-weight: bold }" << endl
     << "h2           { font-size: 100%; font-weight: bold; text-transform: uppercase }" << endl
     << "h3           { font-size: 100%; font-weight: normal }" << endl
     << "h4           { font-size: 100%; font-style: oblique; font-weight: normal }          " << endl
     << "hr           { line-height: 2px; margin-top: 0; margin-bottom: 0; padding-top: 0; padding-bottom: 0; height: 10px }" << endl
     << "div.title    { color: white; background-color: #395af6;  padding-top: 10px; padding-bottom: 10px; padding-left: 10px }" << endl
     << "div.title h1 { text-transform: uppercase; margin-top: 0; margin-bottom: 0 }  " << endl
     << "div.menu     { color: black; background-color: #ff8d09;  padding: 0 10px 10px; " << endl
     << "               width: 9em; float: left; clear: none; overflow: hidden }" << endl
     << "div.menu p   { font-weight: bold; text-transform: uppercase; list-style-type: none; " << endl
     << "               margin-top: 0; margin-bottom: 0; margin-left: 10px }" << endl
     << "div.menu h2  { margin-top: 10px; margin-bottom: 0 ; text-transform: uppercase; }" << endl
     << "div.main     { color: black; background-color: #dae1ed; margin-left: 11em }" << endl
     << "div.space    { font-size: 5px; height: 10px }" << endl
     << "  </style>" << endl
     << "  <body>" << endl
     << "    <div class=\"title\" >" << endl
     << "      <h1>Repro</h1>" << endl
     << "    </div>" << endl
     << "    <div class=\"space\">" << endl
     << "      <br />" << endl
     << "    </div>" << endl
     << "    <div class=\"menu\" >" << endl
     << "      <h2>Configure</h2>" << endl
     << "        <p><a href=\"domains.html\">Domains</a></p>" << endl
     << "        <p><a href=\"acls.html\">ACLs</a></p>" << endl
     << "      <h2>Users</h2>" << endl
     << "        <p><a href=\"addUser.html\">Add User</a></p>" << endl
     << "        <p><a href=\"showUsers.html\">Show Users</a></p>" << endl
     << "      <h2>Routes</h2>" << endl
     << "        <p><a href=\"addRoute.html\">Add Route</a></p>" << endl
     << "        <p><a href=\"showRoutes.html\">Show Routes</a></p>" << endl
     << "      <h2>Statistics</h2>" << endl
     << "        <p><a href=\"registrations.html\">Registrations</a></p>" << endl
     << "    </div>" << endl
     << "    <div class=\"main\">" << endl;
}


void
WebAdmin::buildPageOutlinePost(DataStream& s)
{
   s << "     </div>" << endl
     << "  </body>" << endl
     << "</html>" << endl;
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
      
      buildAddUserSubPage(s); // !cj! TODO - should do beter page here 
      
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
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
