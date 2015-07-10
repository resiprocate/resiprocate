#include "rutil/ResipAssert.h"
#include <sstream>

#include <resip/stack/Symbols.hxx>
#include <resip/stack/Tuple.hxx>
#include <rutil/Data.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/Logger.hxx>
#include <rutil/ParseBuffer.hxx>
#include <rutil/Socket.hxx>
#include <rutil/TransportType.hxx>

#include "AppSubsystem.hxx"
#include "XmlRpcServerBase.hxx"
#include "XmlRpcConnection.hxx"
#include "XmlRpcServer.hxx"
#include "Server.hxx"

using namespace clicktocall;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM AppSubsystem::CLICKTOCALL


XmlRpcServer::XmlRpcServer(Server& server,
                           int port, 
                           IpVersion version ) :
   XmlRpcServerBase(port, version),
   mServer(server)
{
}

void 
XmlRpcServer::sendResponse(unsigned int connectionId, 
                           unsigned int requestId, 
                           const Data& responseData, 
                           unsigned int resultCode, 
                           const Data& resultText,
                           const Data& leg)
{
   std::stringstream ss;
   ss << Symbols::CRLF << responseData << "    <Result Code=\"" << resultCode << "\"";
   if(!leg.empty())
   {
      ss << " Leg=\"" << leg << "\"";
   }
   ss << ">" << resultText << "</Result>" << Symbols::CRLF;
   bool isFinal = resultCode >= 300 || (leg=="Destination" && resultCode >= 200);
   XmlRpcServerBase::sendResponse(connectionId, requestId, ss.str().c_str(), isFinal);
}

void 
XmlRpcServer::handleRequest(unsigned int connectionId, unsigned int requestId, const resip::Data& request)
{
   InfoLog (<< "XmlRpcServer::handleRequest:  connectionId=" << connectionId << ", requestId=" << requestId << ", request=" << request);

   try
   {
      ParseBuffer pb(request);
      XMLCursor xml(pb);

      if(isEqualNoCase(xml.getTag(), "ClickToCall"))
      {
         handleClickToCallRequest(connectionId, requestId, xml);
      }
      else 
      {
         InfoLog(<< "XmlRpcServer::handleRequest: Received XML message with unknown method: " << xml.getTag());
         sendResponse(connectionId, requestId, Data::Empty, 400, "Unknown method");
      }
   }
   catch(resip::BaseException& e)
   {
      InfoLog(<< "XmlRpcServer::handleRequest: ParseException: " << e);
      sendResponse(connectionId, requestId, Data::Empty, 400, "Parse error");
   }
}
  
void 
XmlRpcServer::handleClickToCallRequest(unsigned int connectionId, unsigned int requestId, XMLCursor& xml)
{
   Data initiator;
   Data destination;
   bool anchor = false;

   if(xml.firstChild())
   {
      if(isEqualNoCase(xml.getTag(), "Request"))
      {
         if(xml.firstChild())
         {
            do
            {
               if(isEqualNoCase(xml.getTag(), "Initiator"))
               {
                  if(xml.firstChild())
                  {
                     initiator = xml.getValue();
                     xml.parent();
                  }
               }
               else if(isEqualNoCase(xml.getTag(), "Destination"))
               {
                  if(xml.firstChild())
                  {
                     destination = xml.getValue();
                     xml.parent();
                  }
               }
               else if(isEqualNoCase(xml.getTag(), "AnchorCall"))
               {
                  if(xml.firstChild())
                  {
                     if(isEqualNoCase(xml.getValue(), "true"))
                     {
                        anchor = true;
                     }
                     xml.parent();
                  }
               }
            } while(xml.nextSibling());
            xml.parent();
         }
      }
      xml.parent();
   }
   
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
            XmlRpcInfo info(connectionId, requestId, this);
            mServer.clickToCall(initiatorUri, destinationUri, anchor, &info);

            std::stringstream ss;
            ss << "    <TranslatedInitiator>" << initiatorUri << "</TranslatedInitiator>" << Symbols::CRLF;
            ss << "    <TranslatedDestination>" << destinationUri << "</TranslatedDestination>" << Symbols::CRLF;
            sendResponse(connectionId, requestId, ss.str().c_str(), 100, "In progress");
         }
         catch(resip::BaseException& e)
         {
            std::stringstream ss;
            ss << "Invalid URI format in initiator " << initiatorTranslation << "(" << initiator << ") or destination " << destinationTranslation << "(" << destination << "): " << e;
            sendResponse(connectionId, requestId, Data::Empty, 400, ss.str().c_str());
         }
      }
      else
      {
         std::stringstream ss;
         ss << "No translation rule for either initiator " << initiator << " or destination " << destination;
         sendResponse(connectionId, requestId, Data::Empty, 400, ss.str().c_str());
      }
   }
   else
   {
      sendResponse(connectionId, requestId, Data::Empty, 400, "Missing initiator and/or destination tags");
   }
}

/* ====================================================================

 Copyright (c) 2009, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
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

