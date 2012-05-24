#if !defined(XmlRpcServer_hxx)
#define XmlRpcServer_hxx 

#include <rutil/Data.hxx>
#include <rutil/TransportType.hxx>
#include <resip/stack/Tuple.hxx>
#include <rutil/XMLCursor.hxx>

#include "XmlRpcServerBase.hxx"

namespace resip
{
class DataStream;
}

namespace clicktocall
{
class Server;
class XmlRpcServer;

class XmlRpcInfo
{
public:
   XmlRpcInfo(unsigned int connectionId, unsigned int requestId, XmlRpcServer* xmlRpcServer) :
      mConnectionId(connectionId), mRequestId(requestId), mXmlRpcServer(xmlRpcServer) {}
   XmlRpcInfo() : mConnectionId(0), mRequestId(0), mXmlRpcServer(0) {}
   unsigned int mConnectionId;
   unsigned int mRequestId;
   XmlRpcServer* mXmlRpcServer;
};

class XmlRpcServer: public XmlRpcServerBase
{
public:
   XmlRpcServer(Server& server,
                int port, 
                resip::IpVersion version);

   // thread safe
   virtual void sendResponse(unsigned int connectionId, 
                             unsigned int requestId, 
                             const resip::Data& responseData, 
                             unsigned int resultCode, 
                             const resip::Data& resultText,
                             const resip::Data& leg = resip::Data::Empty);

protected:
   virtual void handleRequest(unsigned int connectionId, 
                              unsigned int requestId, 
                              const resip::Data& request); 

private: 
   void handleClickToCallRequest(unsigned int connectionId, unsigned int requestId, resip::XMLCursor& xml);

   Server& mServer;
};

}

#endif  


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

