#include "rutil/ResipAssert.h"

#include <rutil/Data.hxx>
#include <rutil/Socket.hxx>
#include <resip/stack/Symbols.hxx>
#include <rutil/TransportType.hxx>
#include <rutil/Logger.hxx>
#include <resip/stack/Tuple.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/ParseBuffer.hxx>

#include "repro/XmlRpcServerBase.hxx"
#include "repro/XmlRpcConnection.hxx"

using namespace repro;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

std::atomic<unsigned int> XmlRpcConnectionBase::NextConnectionId(1);

XmlRpcConnectionBase::XmlRpcConnectionBase(XmlRpcServerBase& server) :
   mXmlRcpServer(server),
   mConnectionId(NextConnectionId++),
   mNextRequestId(1)
{
}

XmlRpcConnectionBase::~XmlRpcConnectionBase()
{
}

XmlRpcSocketConnection::XmlRpcSocketConnection(XmlRpcSocketServer& server, resip::Socket sock):
   XmlRpcConnectionBase(server),
   mSock(sock)
{
   resip_assert(mSock > 0);
}


XmlRpcSocketConnection::~XmlRpcSocketConnection()
{
   resip_assert(mSock > 0);
#ifdef WIN32
   closesocket(mSock); 
#else
   close(mSock);
#endif
   mSock=0;
}

      
void 
XmlRpcSocketConnection::buildFdSet(FdSet& fdset)
{
   if (!mTxBuffer.empty())
   {
      fdset.setWrite(mSock);
   }
   fdset.setRead(mSock);
}


bool 
XmlRpcSocketConnection::process(FdSet& fdset)
{
   if (fdset.hasException(mSock))
   {
      int errNum = 0;
      int errNumSize = sizeof(errNum);
      getsockopt(mSock,SOL_SOCKET,SO_ERROR,(char *)&errNum,(socklen_t *)&errNumSize);
      InfoLog (<< "XmlRpcSocketConnection::process: Exception reading from socket "
               << (int)mSock << " code: " << errNum << "; closing connection");
      return false;
   }
   
   if (fdset.readyToRead(mSock))
   {
      bool ok = processSomeReads();
      if (!ok)
      {
         return false;
      }
   }
   if ((!mTxBuffer.empty()) && fdset.readyToWrite(mSock))
   {
      bool ok = processSomeWrites();
      if (!ok)
      {
         return false;
      }
   }

   return true;
}

bool
XmlRpcSocketConnection::processSomeReads()
{
   const int bufSize = 8000;
   char buf[bufSize];
   
 
#if defined(WIN32)
   int bytesRead = ::recv(mSock, buf, bufSize, 0);
#else
   int bytesRead = ::read(mSock, buf, bufSize);
#endif

   if (bytesRead == INVALID_SOCKET)
   {
      int e = getErrno();
      XmlRpcSocketServer::logSocketError(e);
      InfoLog (<< "XmlRpcSocketConnection::processSomeReads: Failed read on " << (int)mSock);
      return false;
   }
   else if(bytesRead == 0)
   {
      DebugLog (<< "XmlRpcSocketConnection::processSomeReads: Connection closed by remote");
      return false;
   }

   //DebugLog (<< "XmlRpcSocketConnection::processSomeReads: read=" << bytesRead);

   mRxBuffer += Data( buf, bytesRead );
   
   while(tryParse());
   
   return true;
}


bool 
XmlRpcSocketConnection::tryParse()
{
   ParseBuffer pb(mRxBuffer);
   Data initialTag;
   const char* start = pb.position();
   pb.skipWhitespace();
   pb.skipToChar('<');   
   if(!pb.eof())
   {
      pb.skipChar();
      const char* anchor = pb.position();
      pb.skipToChar('>');
      if(!pb.eof())
      {
         initialTag = pb.data(anchor);
         // Find end of initial tag
         pb.skipToChars("</" + initialTag + ">");
         if (!pb.eof())
         {
            pb.skipN((int)initialTag.size() + 3);  // Skip past </InitialTag>            
            unsigned int requestId = getNextRequestId();
            mRequests[requestId] = pb.data(start);
            mXmlRcpServer.handleRequest(getConnectionId(), requestId, mRequests[requestId]);

            // Remove processed data from RxBuffer
            pb.skipWhitespace();
            if(!pb.eof())
            {
               anchor = pb.position();
               pb.skipToEnd();
               mRxBuffer = pb.data(anchor);
               return true;
            }
            else
            {
               mRxBuffer.clear();
            }
         }   
      }
   }
   return false;
}

bool
XmlRpcSocketConnection::processSomeWrites()
{
   if (mTxBuffer.empty())
   {
      return true;
   }
   
   //DebugLog (<< "XmlRpcSocketConnection::processSomeWrites: Writing " << mTxBuffer );

#if defined(WIN32)
   int bytesWritten = ::send(mSock, mTxBuffer.data(), (int)mTxBuffer.size(), 0);
#else
   int bytesWritten = ::write(mSock, mTxBuffer.data(), mTxBuffer.size() );
#endif

   if (bytesWritten == INVALID_SOCKET)
   {
      int e = getErrno();
      XmlRpcSocketServer::logSocketError(e);
      InfoLog (<< "XmlRpcSocketConnection::processSomeWrites - failed write on " << mSock << " " << strerror(e));

      return false;
   }
   
   if (bytesWritten == (int)mTxBuffer.size())
   {
      DebugLog (<< "XmlRpcSocketConnection::processSomeWrites - Wrote it all" );
      mTxBuffer = Data::Empty;

      //return false; // return false causes connection to close and clean up
      return true;  // keep connection up
   }
   else
   {
      Data rest = mTxBuffer.substr(bytesWritten);
      mTxBuffer = rest;
      DebugLog( << "XmlRpcSocketConnection::processSomeWrites - Wrote " << bytesWritten << " bytes - still need to do " << mTxBuffer );
   }
   
   return true;
}

bool
XmlRpcConnectionBase::sendResponse(unsigned int requestId, const Data& responseData, bool isFinal)
{
   RequestMap::iterator it = mRequests.find(requestId);
   if(it != mRequests.end())
   {
      Data& request = it->second;
      Data response(request.size() + responseData.size() + 30, Data::Preallocate);
      ParseBuffer pb(request);

      // A response is formed by starting with the request and inserting the 
      // ResponseData between <Response> tags at the same level as the <Request> tags
      const char* start = pb.position();      
      pb.skipToChars("</Request>");
      if (!pb.eof())
      {
         pb.skipN(10);  // Skip past </Request>
         pb.skipWhitespace();
   
         // Response starts with request message up to end of Request tag
         response = pb.data(start);
   
         // Add in response data
         response += Symbols::CRLF;
         response += "  <Response>" + responseData + "  </Response>";
         response += Symbols::CRLF;

         // Add remainder of request message
         start = pb.position();
         pb.skipToEnd();
         response += pb.data(start);
      }
      else
      {
         // No Request in message - just send bare response
         response = "<Response>" + responseData + "</Response>";
      }
      sendResponse(response);
      if(isFinal)
      {
          mRequests.erase(it);
      }
      return true;
   }
   return false;
}

void
XmlRpcSocketConnection::sendResponse(const resip::Data& response)
{
   mTxBuffer += response;
}

void
XmlRpcSocketConnection::sendEvent(const Data& eventData)
{
   mTxBuffer += eventData;
}

#ifdef BUILD_QPID_PROTON

XmlRpcProtonConnection::XmlRpcProtonConnection(XmlRpcProtonServer& server, const resip::Data& replyTo,
                                               std::shared_ptr<ProtonThreadBase::ProtonSenderBase> sender) :
   XmlRpcConnectionBase(server),
   mReplyTo(replyTo),
   mSender(sender)
{
}

XmlRpcProtonConnection::~XmlRpcProtonConnection()
{
}

void
XmlRpcProtonConnection::onRequest(unsigned int requestId, const resip::Data& request)
{
   mRequests[requestId] = request;
}

void
XmlRpcProtonConnection::buildFdSet(resip::FdSet& fdset)
{
   // no need to do anything here, Qpid Proton has its own threads
}

bool
XmlRpcProtonConnection::process(resip::FdSet& fdset)
{
   // no need to do anything here, Qpid Proton has its own threads
   StackLog(<<"process invoked");
   return true;
}

void
XmlRpcProtonConnection::sendResponse(const resip::Data& responseData)
{
   StackLog(<<"sending via Proton: " << responseData);
   mSender->sendMessage(responseData);
}

void
XmlRpcProtonConnection::sendEvent(const resip::Data& eventData)
{
   StackLog(<<"sending via Proton: " << eventData);
   mSender->sendMessage(eventData);
}

#endif


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * Copyright (c) 2010 SIP Spectrum, Inc.  All rights reserved.
 * Copyright (c) 2022 Daniel Pocock https://danielpocock.com
 * Copyright (c) 2022 Software Freedom Institute SA https://softwarefreedom.institute
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

