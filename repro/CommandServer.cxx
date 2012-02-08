#include <cassert>
#include <sstream>

#include <resip/stack/Symbols.hxx>
#include <resip/stack/Tuple.hxx>
#include <resip/stack/SipStack.hxx>
#include <rutil/GeneralCongestionManager.hxx>
#include <rutil/Data.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/Logger.hxx>
#include <rutil/ParseBuffer.hxx>
#include <rutil/Socket.hxx>
#include <rutil/TransportType.hxx>
#include <rutil/Timer.hxx>

#include "repro/XmlRpcServerBase.hxx"
#include "repro/XmlRpcConnection.hxx"
#include "repro/CommandServer.hxx"

using namespace repro;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO


CommandServer::CommandServer(resip::SipStack& sipStack,
                             int port, 
                             IpVersion version) :
   XmlRpcServerBase(port, version),
   mSipStack(sipStack)
{
   mSipStack.setExternalStatsHandler(this);
}

CommandServer::~CommandServer()
{
}

void 
CommandServer::sendResponse(unsigned int connectionId, 
                           unsigned int requestId, 
                           const Data& responseData, 
                           unsigned int resultCode, 
                           const Data& resultText)
{
   std::stringstream ss;
   ss << Symbols::CRLF << "    <Result Code=\"" << resultCode << "\"";
   ss << ">" << resultText.xmlCharDataEncode() << "</Result>" << Symbols::CRLF;
   if(!responseData.empty())
   { 
      ss << "    <Data>" << Symbols::CRLF;
      ss << responseData;
      ss << "    </Data>" << Symbols::CRLF;
   }
   XmlRpcServerBase::sendResponse(connectionId, requestId, ss.str().c_str(), resultCode >= 200 /* isFinal */);
}

void 
CommandServer::handleRequest(unsigned int connectionId, unsigned int requestId, const resip::Data& request)
{
   DebugLog (<< "CommandServer::handleRequest:  connectionId=" << connectionId << ", requestId=" << requestId << ", request=\r\n" << request);

   try
   {
      ParseBuffer pb(request);
      XMLCursor xml(pb);

      if(isEqualNoCase(xml.getTag(), "GetStackInfo"))
      {
         handleGetStackInfoRequest(connectionId, requestId, xml);
      }
      else if(isEqualNoCase(xml.getTag(), "GetStackStats"))
      {
         handleGetStackStatsRequest(connectionId, requestId, xml);
      }
      else if(isEqualNoCase(xml.getTag(), "ResetStackStats"))
      {
         handleResetStackStatsRequest(connectionId, requestId, xml);
      }      
      else if(isEqualNoCase(xml.getTag(), "LogDnsCache"))
      {
         handleLogDnsCacheRequest(connectionId, requestId, xml);
      }
      else if(isEqualNoCase(xml.getTag(), "ClearDnsCache"))
      {
         handleClearDnsCacheRequest(connectionId, requestId, xml);
      }
      else if(isEqualNoCase(xml.getTag(), "GetDnsCache"))
      {
         handleGetDnsCacheRequest(connectionId, requestId, xml);
      }
      else if(isEqualNoCase(xml.getTag(), "GetCongestionStats"))
      {
         handleGetCongestionStatsRequest(connectionId, requestId, xml);
      }
      else if(isEqualNoCase(xml.getTag(), "SetCongestionTolerance"))
      {
         handleSetCongestionToleranceRequest(connectionId, requestId, xml);
      }
      else 
      {
         WarningLog(<< "CommandServer::handleRequest: Received XML message with unknown method: " << xml.getTag());
         sendResponse(connectionId, requestId, Data::Empty, 400, "Unknown method");
      }
   }
   catch(resip::BaseException& e)
   {
      WarningLog(<< "CommandServer::handleRequest: ParseException: " << e);
      sendResponse(connectionId, requestId, Data::Empty, 400, "Parse error");
   }
}

void 
CommandServer::handleGetStackInfoRequest(unsigned int connectionId, unsigned int requestId, XMLCursor& xml)
{
   InfoLog(<< "CommandServer::handleGetStackInfoRequest");

   Data buffer;
   DataStream strm(buffer);
   mSipStack.dump(strm);
   strm.flush();

   sendResponse(connectionId, requestId, buffer, 200, "Stack info retrieved.");
}

void 
CommandServer::handleGetStackStatsRequest(unsigned int connectionId, unsigned int requestId, XMLCursor& xml)
{
   InfoLog(<< "CommandServer::handleGetStackStatsRequest");

   Lock lock(mStatisticsWaitersMutex);
   mStatisticsWaiters.push_back(std::make_pair(connectionId, requestId));

   if(!mSipStack.pollStatistics())
   {
      sendResponse(connectionId, requestId, Data::Empty, 400, "Statistics Manager is not enabled.");
   }
}

void 
CommandServer::handleResetStackStatsRequest(unsigned int connectionId, unsigned int requestId, XMLCursor& xml)
{
   InfoLog(<< "CommandServer::handleResetStackStatsRequest");

   mSipStack.zeroOutStatistics();
   sendResponse(connectionId, requestId, Data::Empty, 200, "Stack stats reset.");
}

void 
CommandServer::handleLogDnsCacheRequest(unsigned int connectionId, unsigned int requestId, XMLCursor& xml)
{
   InfoLog(<< "CommandServer::handleLogDnsCacheRequest");

   mSipStack.logDnsCache();
   sendResponse(connectionId, requestId, Data::Empty, 200, "DNS cache logged.");
}

void 
CommandServer::handleClearDnsCacheRequest(unsigned int connectionId, unsigned int requestId, XMLCursor& xml)
{
   InfoLog(<< "CommandServer::handleQueryDnsCacheRequest");

   mSipStack.clearDnsCache();
   sendResponse(connectionId, requestId, Data::Empty, 200, "DNS cache cleared.");
}

void 
CommandServer::handleGetDnsCacheRequest(unsigned int connectionId, unsigned int requestId, XMLCursor& xml)
{
   InfoLog(<< "CommandServer::handleGetDnsCacheRequest");

   mSipStack.getDnsCacheDump(make_pair(connectionId, requestId), this);
   // Note: Response will be sent when callback is invoked
}

void 
CommandServer::onDnsCacheDumpRetrieved(std::pair<unsigned int, unsigned int> key, const Data& dnsCache)
{
   if(dnsCache.empty())
   {
      sendResponse(key.first, key.second, "empty\r\n", 200, "DNS cache retrieved.");
   }
   else
   {
      sendResponse(key.first, key.second, dnsCache, 200, "DNS cache retrieved.");
   }
}

void 
CommandServer::handleGetCongestionStatsRequest(unsigned int connectionId, unsigned int requestId, XMLCursor& xml)
{
   InfoLog(<< "CommandServer::handleGetCongestionStatsRequest");

   CongestionManager* congestionManager = mSipStack.getCongestionManager();
   if(congestionManager != 0)
   {
      Data buffer;
      DataStream strm(buffer);
      congestionManager->encodeCurrentState(strm);

      sendResponse(connectionId, requestId, buffer, 200, "Congestion stats retrieved.");
   }
   else
   {
      sendResponse(connectionId, requestId, Data::Empty, 400, "Congestion Manager is not enabled.");
   }
}

void 
CommandServer::handleSetCongestionToleranceRequest(unsigned int connectionId, unsigned int requestId, XMLCursor& xml)
{
   InfoLog(<< "CommandServer::handleSetCongestionToleranceRequest");

   Data fifoDescription;
   Data metricData;
   GeneralCongestionManager::MetricType metric;
   unsigned long maxTolerance=0;

   GeneralCongestionManager* congestionManager = dynamic_cast<GeneralCongestionManager*>(mSipStack.getCongestionManager());
   if(congestionManager != 0)
   {
      // Check for Parameters
      if(xml.firstChild())
      {
         if(isEqualNoCase(xml.getTag(), "request"))
         {
            if(xml.firstChild())
            {
               while(true)
               {
                  if(isEqualNoCase(xml.getTag(), "fifoDescription"))
                  {
                     if(xml.firstChild())
                     {
                        fifoDescription = xml.getValue();
                        xml.parent();
                     }
                  }
                  else if(isEqualNoCase(xml.getTag(), "metric"))
                  {
                     if(xml.firstChild())
                     {
                        metricData = xml.getValue();
                        xml.parent();
                     }
                  }
                  else if(isEqualNoCase(xml.getTag(), "maxtolerance"))
                  {
                     if(xml.firstChild())
                     {
                        maxTolerance = xml.getValue().convertUnsignedLong();
                        xml.parent();
                     }
                  }
                  if(!xml.nextSibling())
                  {
                     // break on no more sibilings
                     break;
                  }
               }
               xml.parent();
            }
         }
         xml.parent();
      }

      if(isEqualNoCase(metricData, "WAIT_TIME"))
      {
         metric = GeneralCongestionManager::WAIT_TIME;
      }
      else if(isEqualNoCase(metricData, "TIME_DEPTH"))
      {
         metric = GeneralCongestionManager::TIME_DEPTH;
      }
      else if(isEqualNoCase(metricData, "SIZE"))
      {
         metric = GeneralCongestionManager::SIZE;
      }
      else 
      {
         sendResponse(connectionId, requestId, Data::Empty, 400, "Invalid metric specified: must be SIZE, TIME_DEPTH or WAIT_TIME.");
         return;
      }

      if(maxTolerance == 0)
      {
         sendResponse(connectionId, requestId, Data::Empty, 400, "Invalid MaxTolerance specified: must be greater than 0.");
         return;
      }

      if(congestionManager->updateFifoTolerances(fifoDescription, metric, maxTolerance))
      {
         sendResponse(connectionId, requestId, Data::Empty, 200, "Congestion Tolerance set.");
      }
      else
      {
         sendResponse(connectionId, requestId, Data::Empty, 400, "Invalid fifo description provided.");
      }
   }
   else
   {
      sendResponse(connectionId, requestId, Data::Empty, 400, "Congestion Manager is not enabled.");
   }
}

bool 
CommandServer::operator()(resip::StatisticsMessage &statsMessage)
{
   Lock lock(mStatisticsWaitersMutex);
   if(mStatisticsWaiters.size() > 0)
   {
      Data buffer;
      DataStream strm(buffer);
      StatisticsMessage::Payload payload;
      statsMessage.loadOut(payload);  // !slg! could optimize by providing stream operator on StatisticsMessage
      strm << payload << endl;

      StatisticsWaitersList::iterator it = mStatisticsWaiters.begin();
      for(; it != mStatisticsWaiters.end(); it++)
      {
         sendResponse(it->first, it->second, buffer, 200, "Stack stats retrieved.");
      }
   }
   return true;
}

/*
void 
CommandServer::streamContactInstanceRecord(std::stringstream& ss, const ContactInstanceRecord& rec)
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
}*/


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
