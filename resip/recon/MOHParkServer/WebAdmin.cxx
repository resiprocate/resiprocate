#include "rutil/ResipAssert.h"
#include <time.h>

#include <resip/recon/UserAgent.hxx>

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
#include "ActiveCallInfo.hxx"
#include "Server.hxx"

using namespace resip;
using namespace mohparkserver;
using namespace std;

#define RESIPROCATE_SUBSYSTEM AppSubsystem::MOHPARKSERVER


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

WebAdmin::WebAdmin(  Server& server,
                     bool noChal,  
                     const Data& realm, // this realm is used for http challenges
                     const Data& adminPassword,
                     int port, 
                     IpVersion version ):
   HttpBase( port, version, realm ),
   mServer(server),
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
   if (pageName != Data("activecalls.html"))
   { 
      setPage( resip::Data::Empty, pageNumber, 301 );
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
   
   Data page;
   DataStream s(page);
   buildPageOutlinePre(s);
      
   if ( pageName == Data("activecalls.html"))
   {
       buildActiveCallsSubPage(s);
   }
   else
   {
       resip_assert(false);
   }
   buildPageOutlinePost(s);
   s.flush();

   resip_assert( !page.empty() );   
   setPage( page, pageNumber,200 );
}
  

void
WebAdmin::buildActiveCallsSubPage(DataStream& s)
{
   if (!mRemoveSet.empty())
   {
      int j = 0;
      for (set<RemoveKey>::iterator i = mRemoveSet.begin(); i != mRemoveSet.end(); ++i)
      {
         Uri aor(i->mKey1);
         ContactInstanceRecord rec;
         size_t bar1 = i->mKey2.find("|");
         size_t bar2 = i->mKey2.find("|",bar1+1);
         
         if(bar1==Data::npos || bar2 == Data::npos)
         {
            InfoLog(<< "Registration removal key was malformed: " << i->mKey2);
            continue;
         }
         
         try
         {
            resip::Data rawNameAddr = i->mKey2.substr(0,bar1).urlDecoded();
            rec.mContact = NameAddr(rawNameAddr);
            rec.mInstance = i->mKey2.substr(bar1+1,bar2-bar1-1).urlDecoded();
            rec.mRegId = i->mKey2.substr(bar2+1,Data::npos).convertInt();
            //mRegDb.lockRecord(aor);   // TODO
            //mRegDb.removeContact(aor, rec);
            //mRegDb.unlockRecord(aor);
            ++j;
         }
         catch(resip::ParseBuffer::Exception& e)
         {
            InfoLog(<< "Registration removal key was malformed: " << e <<
                     " Key was: " << i->mKey2);
         }
         
      }
      s << "<p><em>Removed:</em> " << j << " records</p>" << endl;
   }

   s << 
       "<form id=\"showReg\" method=\"get\" action=\"registrations.html\" name=\"showReg\" enctype=\"application/x-www-form-urlencoded\">" << endl << 
      //"<button name=\"removeAllReg\" value=\"\" type=\"button\">Remove All</button>" << endl << 
      //"<hr/>" << endl << 

      "<table border=\"1\" cellspacing=\"2\" cellpadding=\"0\" align=\"left\">" << endl << 

      "<tr>" << endl << 
      "  <td>Held URI</td>" << endl << 
      "  <td>Invoking URI</td>" << endl << 
      "  <td>Hold Type</td>" << endl << 
      "  <td>Participant ID</td>" << endl << 
      "  <td>Conversation ID</td>" << endl << 
      //"  <td><input type=\"submit\" value=\"Remove\"/></td>" << endl <<   // TODO
      "</tr>" << endl;
  
   CallInfoList callInfos;
   mServer.getActiveCallsInfo(callInfos);
   for (CallInfoList::iterator it = callInfos.begin(); it != callInfos.end(); ++it )
   {
      s << "<tr>" << endl
        << "  <td>" << it->mHeldUri << "</td>" << endl
        << "  <td>" << it->mInvokingUri << "</td>" << endl
        << "  <td>" << it->mHoldType << "</td>" << endl
        << "  <td>" << it->mParticipantId << "</td>" << endl
        << "  <td>" << it->mConversationId << "</td>" << endl
        //<< "  <td><input type=\"checkbox\" name=\"remove." << it->mHeldUri << "\" value=\"" << it->mParticipantId << "\"/></td>" << endl  // TODO
        << "</tr>" << endl;
   }
                  
   s << "</table>" << endl << 
      "</form>" << endl;
}

void
WebAdmin::buildPageOutlinePre(DataStream& s)
{
s << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
s << "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">\n";
s << "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n";
s << "  <head>\n";
s << "    <meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\" />\n";
s << "    <title>MOHParkServer Active Call List</title>\n";
s << "  </head>\n";
s << "  <style>\n";
s << "body         { bgcolor: white; font-size: 90%; font-family: Arial, Helvetica, sans-serif }\n";
s << "h1           { font-size: 200%; font-weight: bold }\n";
s << "h2           { font-size: 100%; font-weight: bold; text-transform: uppercase }\n";
s << "h3           { font-size: 100%; font-weight: normal }\n";
s << "h4           { font-size: 100%; font-style: oblique; font-weight: normal }          \n";
s << "hr           { line-height: 2px; margin-top: 0; margin-bottom: 0; padding-top: 0; padding-bottom: 0; height: 10px }\n";
s << "div.title    { color: white; background-color: #395af6;  padding-top: 10px; padding-bottom: 10px; padding-left: 10px }\n";
s << "div.title h1 { text-transform: uppercase; margin-top: 0; margin-bottom: 0 }  \n";
s << "div.menu     { color: black; background-color: #ff8d09;  padding: 0 10px 10px; \n";
s << "               width: 9em; float: left; clear: none; overflow: hidden }\n";
s << "div.menu p   { font-weight: bold; text-transform: uppercase; list-style-type: none; \n";
s << "               margin-top: 0; margin-bottom: 0; margin-left: 10px }\n";
s << "div.menu h2  { margin-top: 10px; margin-bottom: 0 ; text-transform: uppercase; }\n";
s << "div.main     { color: black; background-color: #dae1ed; margin-left: 11em }\n";
s << "div.space    { font-size: 5px; height: 10px }\n";
s << "  </style>\n";
s << "  <body>\n";
}


void
WebAdmin::buildPageOutlinePost(DataStream& s)
{
s << "  </body>\n";
s << "</html>\n";
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
