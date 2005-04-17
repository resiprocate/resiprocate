
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
#include "repro/RouteAbstractDb.hxx"


using namespace resip;
using namespace repro;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO


// !cj! TODO - make all the removes on the web pages work 

WebAdmin::WebAdmin(  UserAbstractDb& userDb,  
                     RegistrationPersistenceManager& regDb,
                     RouteAbstractDb& routeDb,
                     Security& security,
                     bool noChal, 
                     int port, 
                     IpVersion version,
                     const Data& realm ):
   HttpBase( port, version, realm ),
   mUserDb(userDb),
   mRegDb(regDb),
   mRouteDb( routeDb ),
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
      ( pageName != Data("addUser.html") ) && 
      ( pageName != Data("addRoute.html") ) && 
      ( pageName != Data("showRegs.html") ) &&  
      ( pageName != Data("showRoutes.html") )&& 
      ( pageName != Data("showUsers.html")  ) &&
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
      Data dbA1 = mUserDb.getUserAuthInfo( pUser, Data::Empty );
      
      if ( dbA1.empty() ) // if the admin user does not exist, add it 
      { 
         mUserDb.addUser( pUser, // user
                          Data("localhost"), // domain 
                          Data::Empty, // realm 
                          Data("admin"), // password 
                          Data::Empty, // name 
                          Data::Empty ); // email 
         dbA1 = mUserDb.getUserAuthInfo( pUser, Data::Empty );
      }
      
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
            
            mUserDb.addUser(user,domain,realm,password,name,email);
         }
         
         if ( !routeDestination.empty() )
         {
            mRouteDb.add(routeMethod ,routeEvent ,routeUri, routeDestination, routeOrder );
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
         if ( pageName == Data("addUser.html")    ) buildAddUserSubPage(s);
         if ( pageName == Data("addRoute.html")   ) buildAddRouteSubPage(s);
         if ( pageName == Data("showRegs.html")   ) buildShowRegsSubPage(s);
         if ( pageName == Data("showRoutes.html") ) buildShowRoutesSubPage(s);
         if ( pageName == Data("showUsers.html")  ) buildShowUsersSubPage(s);

            buildPageOutlinePost(s);
            s.flush();
   }
   
   if ( !authenticatedUser.empty() )
   {
      // user only pages 
      if ( pageName == Data("user.html") ) page=buildUserPage();
   }

   assert( !authenticatedUser.empty() );
   assert( !page.empty() );
   
   setPage( page, pageNumber,200 );
}
  

void
WebAdmin::buildAddRouteSubPage(DataStream& s)
{
   s << 
      "<form id=\"addRouteFrom\" method=\"get\" action=\"input\" name=\"addRouteForm\">"
      "<table width=\"122\" border=\"1\" cellspacing=\"2\" cellpadding=\"0\">"
      "<tr>"
      "<td>URI</td>"
      "<td><input type=\"text\" name=\"routeUri\" size=\"24\"/></td>"
      "</tr>"
      "<tr>"
      "<td>Method</td>"
      "<td><input type=\"text\" name=\"routeMethod\" size=\"24\"/></td>"
      "</tr>"
      
      "<tr>"
      "<td>Event</td>"
      "<td><input type=\"text\" name=\"routeEvent\" size=\"24\"/></td>"
      "</tr>"
      
      "<tr>"
      "<td>Destination</td>"
      "<td><input type=\"text\" name=\"routeDestination\" size=\"24\"/></td>"
      "</tr>"
      
      "<tr>"
      "<td>Order</td>"
      "<td><input type=\"text\" name=\"routeOrder\" size=\"4\"/></td>"
      "</tr>"
      
      "</table>"
      "<p><input type=\"reset\"/ value=\"Cancel\" > <input type=\"submit\" name=\"routeAdd\" value=\"Add\"/></p>"
      "</form>"
      ;
}


void
WebAdmin::buildAddUserSubPage( DataStream& s)
{
      s << 
         "<form id=\"addUserForm\" action=\"input\"  method=\"get\" name=\"addUserForm\" enctype=\"application/x-www-form-urlencoded\">"
         "<table border=\"0\" cellspacing=\"2\" cellpadding=\"0\" align=\"left\">"
         "<tr>"
         "<td align=\"right\" valign=\"middle\">User Name:</td>"
         "<td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"user\" size=\"24\"/></td>"
         "</tr>"

         "<tr>"
         "<td align=\"right\" valign=\"middle\" >Realm:</td>"
         "<td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"realm\" size=\"24\"/></td>"
         "</tr>"

         "<tr>"
         "<td align=\"right\" valign=\"middle\" >Domain:</td>"
         "<td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"domain\" size=\"24\"/></td>"
         "</tr>"

         "<tr>"
         "<td align=\"right\" valign=\"middle\" >Password:</td>"
         "<td align=\"left\" valign=\"middle\"><input type=\"password\" name=\"password\" size=\"24\"/></td>"
         "</tr>"

         "<tr>"
         "<td align=\"right\" valign=\"middle\" >Full Name:</td>"
         "<td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"name\" size=\"24\"/></td>"
         "</tr>"
         "<tr>"
         "<td align=\"right\" valign=\"middle\" >Email:</td>"
         "<td align=\"left\" valign=\"middle\"><input type=\"text\" name=\"email\" size=\"24\"/></td>"
         "</tr>"
         "</table>"

         " <input type=\"reset\" value=\"Cancel\"/>"
         "    <input type=\"submit\" name=\"submit\" value=\"OK\"/>"


         "</form>"
         " ";
}


void
WebAdmin::buildShowRegsSubPage(DataStream& s)
{
      s << 
         "<h1>Registrations</h1>"
         "<form id=\"showReg\" method=\"get\" action=\"input\" name=\"showReg\" enctype=\"application/x-www-form-urlencoded\">"
         "<button name=\"removeAllReg\" value=\"\" type=\"button\">Remove All</button>"
         ""
         "<hr/>"
         "<table border=\"1\" cellspacing=\"2\" cellpadding=\"0\" align=\"left\">"
         "<tr>"
         "<td>AOR</td>"
         "<td>Contact</td>"
         "<td><button name=\"removeReg\" type=\"button\">Remove</button></td>"
         "</tr>";
      
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
            s << "<tr>"
              << "<td>";
            if (first) 
            { 
               s << uri;
               first = false;
            }
            s << "</td>"
              << "<td>";
            
            const RegistrationPersistenceManager::ContactPair& p = *i;
            const Uri& contact = p.first; 
         
            s << contact << " ";
            s <<"</td>"
              << "<td><input type=\"checkbox\" name=\"removeUser\" value=\""
              << uri
              << "\"/></td>"
              << "</tr>";
         }
      }
                  
      s << "</table>"
         "</form>"
         ;
}


void 
WebAdmin::buildShowUsersSubPage(DataStream& s)
{
      
      s << 
         "<h1>Users</h1>"
         "<form id=\"showUsers\" method=\"get\" action=\"input\" name=\"showUsers\" enctype=\"application/x-www-form-urlencoded\">"
         "<table width=\"196\" border=\"1\" cellspacing=\"2\" cellpadding=\"0\" align=\"left\">"
         "<tr>"
         "<td>User</td>"
         "<td>Realm</td>"
         "<td>Name</td>"
         "<td>Email</td>"
         "<td><button name=\"buttonName\" type=\"button\">Remove</button></td>"
         "</tr>" ;
      
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
      
      Data key = mUserDb.getFirstKey();
      while ( !key.empty() )
      {
         s << "<tr>"
           << "<td>" << key << "</td>"
           << "<td> </td>"
           << "<td> </td>"
           << "<td> </td>"
           << "<td><input type=\"checkbox\" name=\"removeUser\" value=\""
           << key
           << "\"/></td>"
           << "</tr>"
           << endl;
         
         key = mUserDb.getNextKey();
      }
      
      s << 
         "</table>"
         "</form>"
         ;
}


void
WebAdmin::buildShowRoutesSubPage(DataStream& s)
{ 
      
      s <<
         "    <table border=\"0\" cellspacing=\"2\" cellpadding=\"0\" align=\"left\">"
         "      <tr>"
         "        <td>"
         "          <h1>Routes</h1>"
         "        </td>"
         "      </tr>"
         "      <tr>"
         "        <td>"
         "          <form id=\"showReg\" action=\"input\" method=\"get\" name=\"showReg\" enctype=\"application/x-www-form-urlencoded\">"
         "            <button name=\"removeAllRoute\" value=\"\" type=\"submit\">Remove All</button>"
         "            <hr/>"
         "            <table border=\"1\" cellspacing=\"2\" cellpadding=\"0\" align=\"left\">"
         "              <tr>"
         "                <td>URI</td>"
         "                <td>Method</td>"
         "                <td>Event</td>"
         "                <td>Destination</td>"
         "                <td>Order</td>"
         "                <td><button name=\"removeRoute\" type=\"submit\">Remove</button></td>"
         "              </tr>";
      
      RouteAbstractDb::RouteList routes = mRouteDb.getRoutes();
      for ( RouteAbstractDb::RouteList::const_iterator i = routes.begin();
            i != routes.end();
            i++ )
      {
         s <<  "<tr>"
            "<td>" << i->mMatchingPattern << "</td>"
            "<td>" << i->mMethod << "</td>"
            "<td>" << i->mEvent << "</td>"
            "<td>" << i->mRewriteExpression << "</td>"
            "<td>" << i->mOrder << "</td>"
            "<td><input type=\"checkbox\" name=\"removeRoute\" value=\"" << "TODO" << "\"/></td>"
            "</tr>";
      }
      
      s << 
         "            </table>"
         "          </form>"
         "        </td>"
         "      </tr>"
         "      <tr><td>"
         "          <hr noshade=\"noshade\"/>"
         "          <br>"
         "        </td></tr>";

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
      RouteAbstractDb::UriList routeList = mRouteDb.process( uri, Data("INVITE"), Data::Empty);
      
      s << 
         "      <tr>"
         "        <td>"
         "          <form id=\"testRoute\" action=\"showRoutes.html\" method=\"get\" name=\"testRoute\" enctype=\"application/x-www-form-urlencoded\">"
         "            <table border=\"0\" cellspacing=\"2\" cellpadding=\"0\">"
         "              "
         "              <tr>"
         "                <td align=\"right\">Input:</td>"
         "                <td><input type=\"text\" name=\"routeTestUri\" value=\"" << uri << "\" size=\"24\"/></td>"
         "                <td><input type=\"submit\" name=\"testRoute\" value=\"Test Route\"/></td>"
         "              </tr>";
      
      bool first=true;
      for ( RouteAbstractDb::UriList::const_iterator i=routeList.begin();
            i != routeList.end(); i++)
      {
         s<<"              <tr>";
         if (first)
         {
            first=false;
            s<<"             <td align=\"right\">Targets:</td>";
         }
         else
         {
            s<<"             <td align=\"right\"></td>";
         }
         s<<"                <td><label>" << *i << "</label></td>";
         s<<"                <td></td>";
         s<<"              </tr>";
      }
      
      s<<
         "            </table>"
         "          </form>"
         "        </td>"
         "      </tr>"
         "    </table>"
         ;
      
}

Data
WebAdmin::buildCertPage(const Data& domain)
{
   assert(!domain.empty());
   return mSecurity.getDomainCertDER(domain);
}


Data 
WebAdmin::buildDefaultPage()
{ 
   Data ret;
   {
      DataStream s(ret);
      
      s << 
         "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
         "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">"
         ""
         "<html xmlns=\"http://www.w3.org/1999/xhtml\">"
         ""
         "<head>"
         "<meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\" />"
         "<title>Repro Proxy</title>"
         "</head>"
         ""
         "<body bgcolor=\"#ffffff\">"
         "<h1>The default account is 'admin' with password 'admin'</h1>"
         "<p><a href=\"addUser.html\">add users</a></p>"
         "<p><a href=\"showRegs.html\">show registrations</a></p>"
         "<p><a href=\"showUsers.html\">show users</a></p>"
         "<p><a href=\"addRoute.html\">add route</a></p>"
         "<p><a href=\"showRoutes.html\">show routes</a></p>"
         "</body>"
         ""
         "</html>"
         " ";
      
      s.flush();
   }
   return ret;
}


void
WebAdmin::buildPageOutlinePre(DataStream& s)
{
   s << 
      "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
      "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">"
      ""
      "<html xmlns=\"http://www.w3.org/1999/xhtml\">"
      ""
      "	<head>"
      "		<meta http-equiv=\"content-type\" content=\"text/html;charset=iso-8859-1\"/>"
      "		<meta name=\"generator\" content=\"Adobe GoLive\"/>"
      "		<title>Repro Proxy</title>"
      "		<style type=\"text/css\" media=\"screen\"><!--"
      "p    {  color: black; background-color: white; }"
      "h1        { font-size: 28px; font-weight: bold; margin: 5px 0 }"
      "h2        { font-size: 14px; font-weight: bold; margin: 5px 0 }"
      "h3        { font-size: 14px; font-weight: normal; margin: 2px 0 }"
      "h4     { font-size: 14px; font-style: oblique; font-weight: bold; margin: 2px 0; position: relative }"
      "body  { font-size: 14px; font-family: Arial, Helvetica, sans-serif }"
      "--></style>"
      "	</head>"
      ""
      "	<body>"
      "		<table border=\"0\" cellspacing=\"0\" cellpadding=\"0\" align=\"left\">"
      "			<tr height=\"42\">"
      "				<td colspan=\"3\" bgcolor=\"#395af6\" >"
      "					<h1><font color=\"white\">Repro</font></h1>"
      "				</td>"
      "			</tr>"
      "			<tr height=\"10\">"
      "				<td colspan=\"3\" ></td>"
      "			</tr>"
      "			<tr>"
      "				<td>"
      "					<table border=\"0\" cellspacing=\"0\" cellpadding=\"1\" bgcolor=\"#ff8d09\">"
      "						<tr>"
      "							<td>"
      "								<h2>&nbsp;Users&nbsp;</h2>"
      "							</td>"
      "						</tr>"
      "						<tr>"
      "							<td>"
      "								<h3>&nbsp;&nbsp;&nbsp;<a href=\"addUser.html\">Add&nbsp;Users</a>&nbsp;</h3>"
      "							</td>"
      "						</tr>"
      "						<tr>"
      "							<td>"
      "								<h3>&nbsp;&nbsp;&nbsp;<a href=\"showUsers.html\">Show&nbsp;Users</a>&nbsp;</h3>"
      "							</td>"
      "						</tr>"
      "						<tr>"
      "							<td>"
      "								<h3>&nbsp;&nbsp;&nbsp;<a href=\"showRegs.html\">Registrations</a>&nbsp;</h3>"
      "							</td>"
      "						</tr>"
      "						<tr>"
      "							<td>"
      "								"
      "							</td>"
      "						</tr>"
      "						<tr>"
      "							<td>"
      "								<h2>&nbsp;Routes&nbsp;</h2>"
      "							</td>"
      "						</tr>"
      "						<tr>"
      "							<td>"
      "								<h3>&nbsp;&nbsp;&nbsp;<a href=\"addRoute.html\">Add&nbsp;Route</a>&nbsp;</h3>"
      "							</td>"
      "						</tr>"
      "						<tr>"
      "							<td>"
      "								<h3>&nbsp;&nbsp;&nbsp;<a href=\"showRoutes.html\">Show&nbsp;Routes</a>&nbsp;</h3>"
      "							</td>"
      "						</tr>"
      "					</table>"
      "				</td>"
      "				<td width=\"10\"></td>"
      "				<td align=\"left\" valign=\"top\">"
      ;
}


void
WebAdmin::buildPageOutlinePost(DataStream& s)
{
   s << 
      "				</td>"
      "			</tr>"
      "		</table>"
      "	</body>"
      ""
      "</html>"
      ;
}


Data 
WebAdmin::buildUserPage()
{ 
   Data ret;
   {
      DataStream s(ret);
      
      s << 
         "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
         "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">"
         ""
         "<html xmlns=\"http://www.w3.org/1999/xhtml\">"
         ""
         "<head>"
         "<meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\" />"
         "<title>Repro Proxy</title>"
         "</head>"
         ""
         "<body bgcolor=\"#ffffff\">"
         "<h1>user page </h1>"
         "<p><a href=\"addUser.html\">add users</a></p>"
         "<p><a href=\"showRegs.html\">show registrations</a></p>"
         "<p><a href=\"showUsers.html\">show users</a></p>"
         "<p><a href=\"addRoute.html\">add route</a></p>"
         "<p><a href=\"showRoutes.html\">show routes</a></p>"
         "</body>"
         ""
         "</html>"
         " ";
      
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
