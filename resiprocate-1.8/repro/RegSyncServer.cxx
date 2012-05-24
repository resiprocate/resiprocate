#include <cassert>
#include <sstream>

#include <resip/stack/Symbols.hxx>
#include <resip/stack/Tuple.hxx>
#include <rutil/Data.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/Logger.hxx>
#include <rutil/ParseBuffer.hxx>
#include <rutil/Socket.hxx>
#include <rutil/TransportType.hxx>
#include <rutil/Timer.hxx>

#include "repro/XmlRpcServerBase.hxx"
#include "repro/XmlRpcConnection.hxx"
#include "repro/RegSyncServer.hxx"

using namespace repro;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO


RegSyncServer::RegSyncServer(resip::InMemorySyncRegDb* regDb,
                             int port, 
                             IpVersion version) :
   XmlRpcServerBase(port, version),
   mRegDb(regDb)
{
   assert(mRegDb);
   mRegDb->setHandler(this);
}

RegSyncServer::~RegSyncServer()
{
   mRegDb->setHandler(0);
}

void 
RegSyncServer::sendResponse(unsigned int connectionId, 
                           unsigned int requestId, 
                           const Data& responseData, 
                           unsigned int resultCode, 
                           const Data& resultText)
{
   std::stringstream ss;
   ss << Symbols::CRLF << responseData << "    <Result Code=\"" << resultCode << "\"";
   ss << ">" << resultText.xmlCharDataEncode() << "</Result>" << Symbols::CRLF;
   XmlRpcServerBase::sendResponse(connectionId, requestId, ss.str().c_str(), resultCode >= 200 /* isFinal */);
}

void 
RegSyncServer::sendRegistrationModifiedEvent(unsigned int connectionId, const resip::Uri& aor)
{
   ContactList contacts;

   mRegDb->getContacts(aor, contacts);
   sendRegistrationModifiedEvent(connectionId, aor, contacts);
}

void 
RegSyncServer::sendRegistrationModifiedEvent(unsigned int connectionId, const resip::Uri& aor, const ContactList& contacts)
{
   std::stringstream ss;
   bool infoFound = false;

   ss << "<reginfo>" << Symbols::CRLF;
   ss << "   <aor>" << Data::from(aor).xmlCharDataEncode() << "</aor>" << Symbols::CRLF;
   ContactList::const_iterator cit = contacts.begin();
   for(; cit != contacts.end(); cit++)
   {
      const ContactInstanceRecord& rec = *cit;
      if(!rec.mReceivedFrom.onlyUseExistingConnection)
      {
          streamContactInstanceRecord(ss, rec);
          infoFound = true;
      }
   }
   ss << "</reginfo>" << Symbols::CRLF;

   if(infoFound)
   {
      sendEvent(connectionId, ss.str().c_str());
   }
}

void 
RegSyncServer::handleRequest(unsigned int connectionId, unsigned int requestId, const resip::Data& request)
{
   DebugLog (<< "RegSyncServer::handleRequest:  connectionId=" << connectionId << ", requestId=" << requestId << ", request=" << request);

   try
   {
      ParseBuffer pb(request);
      XMLCursor xml(pb);

      if(isEqualNoCase(xml.getTag(), "InitialSync"))
      {
         handleInitialSyncRequest(connectionId, requestId, xml);
      }
      else 
      {
         WarningLog(<< "RegSyncServer::handleRequest: Received XML message with unknown method: " << xml.getTag());
         sendResponse(connectionId, requestId, Data::Empty, 400, "Unknown method");
      }
   }
   catch(resip::BaseException& e)
   {
      WarningLog(<< "RegSyncServer::handleRequest: ParseException: " << e);
      sendResponse(connectionId, requestId, Data::Empty, 400, "Parse error");
   }
}
  
void 
RegSyncServer::handleInitialSyncRequest(unsigned int connectionId, unsigned int requestId, XMLCursor& xml)
{
   InfoLog(<< "RegSyncServer::handleInitialSyncRequest");

   // Check for correct Version
   unsigned int version = 0;
   if(xml.firstChild())
   {
      if(isEqualNoCase(xml.getTag(), "request"))
      {
         if(xml.firstChild())
         {
            if(isEqualNoCase(xml.getTag(), "version"))
            {
               if(xml.firstChild())
               {
                  version = xml.getValue().convertUnsignedLong();
                  xml.parent();
               }
            }
            xml.parent();
         }
      }
      xml.parent();
   }

   if(version == REGSYNC_VERSION)
   {
      mRegDb->initialSync(connectionId);
      sendResponse(connectionId, requestId, Data::Empty, 200, "Initial Sync Completed.");
   }
   else
   {
      sendResponse(connectionId, requestId, Data::Empty, 505, "Version not supported.");
   }
}

void 
RegSyncServer::streamContactInstanceRecord(std::stringstream& ss, const ContactInstanceRecord& rec)
{
    UInt64 now = Timer::getTimeSecs();

    ss << "   <contactinfo>" << Symbols::CRLF;
    ss << "      <contacturi>" << Data::from(rec.mContact.uri()).xmlCharDataEncode() << "</contacturi>" << Symbols::CRLF;
    // If contact is expired or removed, then pass expires time as 0, otherwise send number of seconds until expirey
    ss << "      <expires>" << (((rec.mRegExpires == 0) || (rec.mRegExpires <= now)) ? 0 : (rec.mRegExpires-now)) << "</expires>" << Symbols::CRLF;
    ss << "      <lastupdate>" << now-rec.mLastUpdated << "</lastupdate>" << Symbols::CRLF;
    if(rec.mReceivedFrom.getPort() != 0)
    {
        resip::Data binaryFlowToken;
        Tuple::writeBinaryToken(rec.mReceivedFrom,binaryFlowToken);            
        ss << "      <receivedfrom>" << binaryFlowToken.base64encode() << "</receivedfrom>" << Symbols::CRLF;
    }
    if(rec.mPublicAddress.getType() != UNKNOWN_TRANSPORT)
    {
        resip::Data binaryFlowToken;
        Tuple::writeBinaryToken(rec.mPublicAddress,binaryFlowToken);            
        ss << "      <publicaddress>" << binaryFlowToken.base64encode() << "</publicaddress>" << Symbols::CRLF;
    }
    NameAddrs::const_iterator naIt = rec.mSipPath.begin();
    for(; naIt != rec.mSipPath.end(); naIt++)
    {
        ss << "      <sippath>" << Data::from(naIt->uri()).xmlCharDataEncode() << "</sippath>" << Symbols::CRLF;
    }
    if(!rec.mInstance.empty())
    {
        ss << "      <instance>" << rec.mInstance.xmlCharDataEncode() << "</instance>" << Symbols::CRLF;
    }
    if(rec.mRegId != 0)
    {
        ss << "      <regid>" << rec.mRegId << "</regid>" << Symbols::CRLF;
    }
    ss << "   </contactinfo>" << Symbols::CRLF;
}

void 
RegSyncServer::onAorModified(const resip::Uri& aor, const ContactList& contacts)
{
   sendRegistrationModifiedEvent(0, aor, contacts);
}

void 
RegSyncServer::onInitialSyncAor(unsigned int connectionId, const resip::Uri& aor, const ContactList& contacts)
{
   sendRegistrationModifiedEvent(connectionId, aor, contacts);
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * Copyright (c) 2010 SIP Spectrum, Inc.  All rights reserved.
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
