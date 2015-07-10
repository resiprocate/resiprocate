#include "rutil/ResipAssert.h"
#include <sstream>
#include <signal.h>

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
#include "repro/ReproRunner.hxx"
#include "repro/CommandServer.hxx"

using namespace repro;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

CommandServer::CommandServer(ReproRunner& reproRunner,
                             Data ipAddr,
                             int port, 
                             IpVersion version) :
   XmlRpcServerBase(port, version, ipAddr),
   mReproRunner(reproRunner)
{
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
      ss << responseData.xmlCharDataEncode();
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

      if(!mReproRunner.getProxy())
      {
         sendResponse(connectionId, requestId, Data::Empty, 400, "Proxy not running.");
         return;
      }

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
      else if(isEqualNoCase(xml.getTag(), "Shutdown"))
      {
         handleShutdownRequest(connectionId, requestId, xml);
      }
      else if(isEqualNoCase(xml.getTag(), "GetProxyConfig"))
      {
         handleGetProxyConfigRequest(connectionId, requestId, xml);
      }
      else if(isEqualNoCase(xml.getTag(), "Restart"))
      {
         handleRestartRequest(connectionId, requestId, xml);
      }
      else if(isEqualNoCase(xml.getTag(), "AddTransport"))
      {
         handleAddTransportRequest(connectionId, requestId, xml);
      }
      else if(isEqualNoCase(xml.getTag(), "RemoveTransport"))
      {
         handleRemoveTransportRequest(connectionId, requestId, xml);
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
   mReproRunner.getProxy()->getStack().dump(strm);
   strm.flush();

   sendResponse(connectionId, requestId, buffer, 200, "Stack info retrieved.");
}

void 
CommandServer::handleGetStackStatsRequest(unsigned int connectionId, unsigned int requestId, XMLCursor& xml)
{
   InfoLog(<< "CommandServer::handleGetStackStatsRequest");

   Lock lock(mStatisticsWaitersMutex);
   mStatisticsWaiters.push_back(std::make_pair(connectionId, requestId));

   if(!mReproRunner.getProxy()->getStack().pollStatistics())
   {
      sendResponse(connectionId, requestId, Data::Empty, 400, "Statistics Manager is not enabled.");
   }
}

void 
CommandServer::handleStatisticsMessage(resip::StatisticsMessage &statsMessage)
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
}

void 
CommandServer::handleResetStackStatsRequest(unsigned int connectionId, unsigned int requestId, XMLCursor& xml)
{
   InfoLog(<< "CommandServer::handleResetStackStatsRequest");

   mReproRunner.getProxy()->getStack().zeroOutStatistics();
   sendResponse(connectionId, requestId, Data::Empty, 200, "Stack stats reset.");
}

void 
CommandServer::handleLogDnsCacheRequest(unsigned int connectionId, unsigned int requestId, XMLCursor& xml)
{
   InfoLog(<< "CommandServer::handleLogDnsCacheRequest");

   mReproRunner.getProxy()->getStack().logDnsCache();
   sendResponse(connectionId, requestId, Data::Empty, 200, "DNS cache logged.");
}

void 
CommandServer::handleClearDnsCacheRequest(unsigned int connectionId, unsigned int requestId, XMLCursor& xml)
{
   InfoLog(<< "CommandServer::handleQueryDnsCacheRequest");

   mReproRunner.getProxy()->getStack().clearDnsCache();
   sendResponse(connectionId, requestId, Data::Empty, 200, "DNS cache cleared.");
}

void 
CommandServer::handleGetDnsCacheRequest(unsigned int connectionId, unsigned int requestId, XMLCursor& xml)
{
   InfoLog(<< "CommandServer::handleGetDnsCacheRequest");

   mReproRunner.getProxy()->getStack().getDnsCacheDump(make_pair((unsigned long)connectionId, (unsigned long)requestId), this);
   // Note: Response will be sent when callback is invoked
}

void 
CommandServer::onDnsCacheDumpRetrieved(std::pair<unsigned long, unsigned long> key, const Data& dnsCache)
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

   CongestionManager* congestionManager = mReproRunner.getProxy()->getStack().getCongestionManager();
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

   GeneralCongestionManager* congestionManager = dynamic_cast<GeneralCongestionManager*>(mReproRunner.getProxy()->getStack().getCongestionManager());
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

void 
CommandServer::handleShutdownRequest(unsigned int connectionId, unsigned int requestId, XMLCursor& xml)
{
   InfoLog(<< "CommandServer::handleShutdownRequest");

   sendResponse(connectionId, requestId, Data::Empty, 200, "Shutdown initiated.");
   raise(SIGTERM);
}

void 
CommandServer::handleGetProxyConfigRequest(unsigned int connectionId, unsigned int requestId, resip::XMLCursor& xml)
{
   InfoLog(<< "CommandServer::handleGetProxyConfigRequest");

   Data buffer;
   DataStream strm(buffer);
   strm << mReproRunner.getProxy()->getConfig();

   sendResponse(connectionId, requestId, buffer, 200, "Proxy config retrieved.");
}

void 
CommandServer::handleRestartRequest(unsigned int connectionId, unsigned int requestId, resip::XMLCursor& xml)
{
   InfoLog(<< "CommandServer::handleRestartRequest");

   mReproRunner.restart();
   if(mReproRunner.getProxy())
   {
      sendResponse(connectionId, requestId, Data::Empty, 200, "Restart completed.");
   }
   else
   {
      sendResponse(connectionId, requestId, Data::Empty, 200, "Restart failed.");
   }
}

void 
CommandServer::handleAddTransportRequest(unsigned int connectionId, unsigned int requestId, resip::XMLCursor& xml)
{
   InfoLog(<< "CommandServer::handleAddTransportRequest");

   TransportType type = UDP;
   int port = 0;
   IpVersion ipVersion=V4;
   Data ipInterface;
   Data recordRouteUri;
   UInt32 rcvBufLen = 0;
   bool stunEnabled = false;
   Data sipDomainname;
   SecurityTypes::SSLType sslType = SecurityTypes::TLSv1;
   UInt32 transportFlags = 0;
   Data certificateFilename;
   Data privateKeyFilename;
   SecurityTypes::TlsClientVerificationMode cvm = SecurityTypes::None;
   bool useEmailAsSIP = false;

   // Check for Parameters
   if(xml.firstChild())
   {
      if(isEqualNoCase(xml.getTag(), "request"))
      {
         if(xml.firstChild())
         {
            while(true)
            {
               if(isEqualNoCase(xml.getTag(), "type"))
               {
                  if(xml.firstChild())
                  {
                     type = toTransportType(xml.getValue());
                     xml.parent();
                  }
               }
               else if(isEqualNoCase(xml.getTag(), "port"))
               {
                  if(xml.firstChild())
                  {
                     port = xml.getValue().convertInt();
                     xml.parent();
                  }
               }
               else if(isEqualNoCase(xml.getTag(), "ipversion"))
               {
                  if(xml.firstChild())
                  {
                     if(isEqualNoCase(xml.getValue(), "v6") || xml.getValue() == "6")  // be a little leanient with this, allow just a 6
                     {
                         ipVersion = V6;
                     }
                     xml.parent();
                  }
               }
               else if(isEqualNoCase(xml.getTag(), "interface"))
               {
                  if(xml.firstChild())
                  {
                     ipInterface = xml.getValue();
                     xml.parent();
                  }
               }
               else if(isEqualNoCase(xml.getTag(), "rruri"))
               {
                  if(xml.firstChild())
                  {
                     recordRouteUri = xml.getValue();
                     xml.parent();
                  }
               }
               else if(isEqualNoCase(xml.getTag(), "udprcvbuflen"))
               {
                  if(xml.firstChild())
                  {
                     rcvBufLen = xml.getValue().convertUnsignedLong();
                     xml.parent();
                  }
               }
               else if(isEqualNoCase(xml.getTag(), "stun"))
               {
                  if(xml.firstChild())
                  {
                     if(isEqualNoCase(xml.getValue(), "yes"))
                     {
                         stunEnabled = true;
                     }
                     xml.parent();
                  }
               }
               else if(isEqualNoCase(xml.getTag(), "flags"))
               {
                  if(xml.firstChild())
                  {
                     transportFlags = xml.getValue().convertUnsignedLong();
                     xml.parent();
                  }
               }
               else if(isEqualNoCase(xml.getTag(), "domain"))
               {
                  if(xml.firstChild())
                  {
                     sipDomainname = xml.getValue();
                     xml.parent();
                  }
               }
               else if(isEqualNoCase(xml.getTag(), "ssltype"))
               {
                  if(xml.firstChild())
                  {
                     if(isEqualNoCase(xml.getValue(), "SSLv23"))
                     {
                         sslType = SecurityTypes::SSLv23;
                     }
                     xml.parent();
                  }
               }
               else if(isEqualNoCase(xml.getTag(), "certfile"))
               {
                  if(xml.firstChild())
                  {
                     certificateFilename = xml.getValue();
                     xml.parent();
                  }
               }
               else if(isEqualNoCase(xml.getTag(), "keyfile"))
               {
                  if(xml.firstChild())
                  {
                     privateKeyFilename = xml.getValue();
                     xml.parent();
                  }
               }
               else if(isEqualNoCase(xml.getTag(), "tlscvm"))
               {
                  if(xml.firstChild())
                  {
                     if(isEqualNoCase(xml.getValue(), "opt"))
                     {
                         cvm = SecurityTypes::Optional;
                     }
                     else if(isEqualNoCase(xml.getValue(), "man"))
                     {
                         cvm = SecurityTypes::Mandatory;
                     }
                     xml.parent();
                  }
               }
               else if(isEqualNoCase(xml.getTag(), "tlsuseemail"))
               {
                  if(xml.firstChild())
                  {
                     if(isEqualNoCase(xml.getValue(), "yes"))
                     {
                         useEmailAsSIP = true;
                     }
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

   // We need at least port and transport type to be valid to add
   if(port == 0)
   {
      sendResponse(connectionId, requestId, Data::Empty, 400, "Invalid port specified: must be non-zero.");
      return;
   }
      
   if(type == UNKNOWN_TRANSPORT)
   {
      Data errorString("Invalid transport specified: must one of UDP, TCP");
#if defined( USE_SSL )
      errorString += ", TLS, WS, WSS";
#endif
#if defined( USE_DTLS )
      errorString += ", DTLS";
#endif
      sendResponse(connectionId, requestId, Data::Empty, 400, errorString);
      return;
   }

   // Build RecordRoute URI if needed
   NameAddr rr;
   if(!recordRouteUri.empty())
   {
      try
      {
         if(isEqualNoCase(recordRouteUri, "auto")) // auto generated record route uri
         {
            if(isSecure(type))
            {
               rr.uri().host()=sipDomainname;
               rr.uri().port()=port;
               rr.uri().param(resip::p_transport)=resip::Tuple::toDataLower(type);
               InfoLog (<< "Transport specific record-route enabled (generated): " << rr);
            }
            else
            {
               rr.uri().host()=ipInterface;
               rr.uri().port()=port;
               rr.uri().param(resip::p_transport)=resip::Tuple::toDataLower(type);
               InfoLog (<< "Transport specific record-route enabled (generated): " << rr);
            }
         }
         else
         {
            NameAddr rrtemp(recordRouteUri);
            rr = rrtemp;
            InfoLog (<< "Transport specific record-route enabled: " << rr);
         }
      }
      catch(BaseException& e)
      {
         ErrLog (<< "Invalid uri provided in rruri setting (ignoring): " << e);
      }
   }

   // Build transport info text for response message
   Data transportInfoText("type=");
   transportInfoText += getTransportNameFromType(type).c_str();
   transportInfoText += ", port=" + Data(port);
   transportInfoText += ", ipversion=" + Data((ipVersion == V4 ? "V4" : "V6"));
   if(!ipInterface.empty())
   {
      transportInfoText += ", interface=" + ipInterface;
   }
   if(!recordRouteUri.empty())
   {
      transportInfoText += ", rruri=" + Data::from(rr);
   }
   transportInfoText += ", stun=" + Data((stunEnabled ? "YES" : "NO"));
   transportInfoText += ", flags=" + Data((UInt32)transportFlags);
   if(rcvBufLen)
   {
      transportInfoText += ", udprcvbuflen=" + Data((UInt32)rcvBufLen);
   }
   if(isSecure(type))
   {
      if(!sipDomainname.empty())
      {
         transportInfoText += ", domain=" + sipDomainname;
      }
      transportInfoText += ", ssltype=" + Data((sslType == SecurityTypes::TLSv1 ? "TLSv1" : "SSLv23"));
      if(!certificateFilename.empty())
      {
         transportInfoText += ", cert=" + certificateFilename;
      }
      if(!privateKeyFilename.empty())
      {
         transportInfoText += ", key=" + privateKeyFilename;
      }
      transportInfoText += ", tlscvm=" + Data((cvm == SecurityTypes::None ? "NONE" : (cvm == SecurityTypes::Optional ? "OPT" : "MAN")));
      transportInfoText += ", tlsuseemail=" + Data((stunEnabled ? "YES" : "NO"));
   }

   try
   {
      Transport* transport = mReproRunner.getProxy()->getStack().addTransport(
          type, port, ipVersion, stunEnabled ? StunEnabled : StunDisabled, ipInterface, 
          sipDomainname, Data::Empty, sslType, transportFlags, certificateFilename, 
          privateKeyFilename, cvm, useEmailAsSIP);

      if(transport)
      {
         if(!rr.uri().host().empty())
         {
            mReproRunner.getProxy()->addTransportRecordRoute(transport->getKey(), rr);
         }
         if(rcvBufLen)
         {
            transport->setRcvBufLen(rcvBufLen);
         }

         transportInfoText += ", transportKey=" + Data(transport->getKey());

         Data text("Transport add requested: ");
         text += transportInfoText;
         sendResponse(connectionId, requestId, Data::Empty, 200, text);
      }
      else
      {
         Data text("Unknown error adding transport: ");
         text += transportInfoText;
         sendResponse(connectionId, requestId, Data::Empty, 500, text);
      }
   }
   catch(BaseException& e) 
   {
      Data text("Exception adding transport: ");
      text += transportInfoText;
      text += " - " + e.getMessage();
      sendResponse(connectionId, requestId, Data::Empty, 500, text);
   }
}

void 
CommandServer::handleRemoveTransportRequest(unsigned int connectionId, unsigned int requestId, resip::XMLCursor& xml)
{
   unsigned int transportKey = 0;

   InfoLog(<< "CommandServer::handleRemoveTransportRequest");

   // Check for Parameters
   if(xml.firstChild())
   {
      if(isEqualNoCase(xml.getTag(), "request"))
      {
         if(xml.firstChild())
         {
            while(true)
            {
               if(isEqualNoCase(xml.getTag(), "key"))
               {
                  if(xml.firstChild())
                  {
                     transportKey = xml.getValue().convertUnsignedLong();
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

   // We need transport key to remove
   if(transportKey == 0)
   {
      sendResponse(connectionId, requestId, Data::Empty, 400, "Invalid transport key specified: must be non-zero.");
      return;
   }
   mReproRunner.getProxy()->getStack().removeTransport(transportKey);
   mReproRunner.getProxy()->removeTransportRecordRoute(transportKey);

   Data text("Transport remove requested: transportKey=");
   text += Data(transportKey);
   sendResponse(connectionId, requestId, Data::Empty, 200, text);
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
