#include "rutil/ResipAssert.h"
#include <time.h>

#include <resip/stack/Symbols.hxx>
#include <resip/stack/Tuple.hxx>
#include <rutil/Data.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/Logger.hxx>
#include <rutil/MD5Stream.hxx>
#include <rutil/ParseBuffer.hxx>
#include <rutil/Socket.hxx>
#include <rutil/Timer.hxx>
#include <rutil/TransportType.hxx>

#include "AppSubsystem.hxx"
#include "HttpBase.hxx"
#include "HttpConnection.hxx"
#include "WebAdmin.hxx"
#include "Server.hxx"

using namespace clicktocall;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM AppSubsystem::CLICKTOCALL


WebAdmin::WebAdmin(Server& server,
                   bool noChal,  
                   const Data& realm, // this realm is used for http challenges
                   const Data& adminPassword,
                   int port, 
                   IpVersion version ):
   HttpBase(port, version, realm),
   mServer(server),
   mNoWebChallenges(noChal),
   mAdminPassword(adminPassword)
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
   if (pageName != Data("index.html") &&
       pageName != Data("clicktocall.html"))
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
  
   Data authenticatedUser;
   if (mNoWebChallenges)
   {
      // don't do authentication - give everyone admin privilages
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
      
      MD5Stream dba1;
      dba1 << "admin" // username
           << Symbols::COLON
           << Data::Empty // realm
           << Symbols::COLON
           << mAdminPassword;
      Data dbA1 = dba1.getHex();

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
           
         if ( !key.empty() && !value.empty() ) // make sure both exist
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
      if ( pageName == Data("clicktocall.html")) buildClickToCallPage(s);
      
      buildPageOutlinePost(s);
      s.flush();
   }
   
   resip_assert( !authenticatedUser.empty() );
   resip_assert( !page.empty() );
   
   setPage( page, pageNumber,200 );
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
         "<title>ClickToCall Server Login</title>" << endl << 
         "</head>" << endl << 

         "<body bgcolor=\"#ffffff\">" << endl << 
         "  <p>Click-To-Call HTML Server</p>" << endl << 
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
      "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << endl << 
      "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">" << endl << 
      "<html xmlns=\"http://www.w3.org/1999/xhtml\">" << endl << 
      "<head>" << endl << 
      "<meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\" />" << endl << 
      "<title>ClickToCall Server</title>" << endl << 
      "</head>" << endl << 

      "<body bgcolor=\"#ffffff\">" << endl;
}

void
WebAdmin::buildPageOutlinePost(DataStream& s)
{
   s << "</body>" << endl << 
        "</html>" << endl;
}

void
WebAdmin::buildClickToCallPage(DataStream& s)
{
   Data initiator;
   Data destination;
   bool anchor = false;
   Dictionary::iterator pos;
   pos = mHttpParams.find("initiator");
   if (pos != mHttpParams.end()) 
   {
      initiator = pos->second;
   }
   pos = mHttpParams.find("destination");
   if (pos != mHttpParams.end()) 
   {
      destination = pos->second;
   }
   pos = mHttpParams.find("anchor");
   if (pos != mHttpParams.end())
   {
      if(isEqualNoCase(pos->second, "true"))
      {
         anchor = true;
      }
   }

   s << "<p><em>ClickToCall:</em> initiator=" << initiator << ", destination=" << destination << (anchor ? " (anchored)" : "") << "</p>" << endl;
   if(!initiator.empty() && !destination.empty())
   {
      Data initiatorTranslation;
      Data destinationTranslation;

      if(mServer.translateAddress(initiator, initiatorTranslation, true /* fail if no rule */) &&
         mServer.translateAddress(destination, destinationTranslation, true /* fail if no rule */))
      {
         try
         {
            Uri initiatorUri(initiatorTranslation);
            Uri destinationUri(destinationTranslation);

            mServer.clickToCall(initiatorUri, destinationUri, anchor);
            s << "<p>Initiating ClickToCall from " << initiatorUri << "(" << initiator << ") to " << destinationUri << "(" << destination << (anchor ? ") (anchored)" : ") (unanchored)") << "...</p>" << endl;
         }
         catch(resip::BaseException& e)
         {
            s << "<p>Invalid URI format in initiator " << initiatorTranslation << "(" << initiator << ") or destination " << destinationTranslation << "(" << destination << "): " << e << "</p>" << endl;
         }
      }
      else
      {
         s << "<p>No translation rule for either initiator " << initiator << " or destination " << destination << ".</p>" << endl;
      }
   }
   else
   {
      s << "<p>Missing initiator or destination URL parameters.</p>" << endl;
   }
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
