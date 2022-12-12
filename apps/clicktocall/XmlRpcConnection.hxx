#if !defined(XmlRpcConnection_hxx)
#define XmlRpcConnection_hxx 

#include <map>
#include <rutil/Data.hxx>
#include <rutil/Socket.hxx>
#include <rutil/TransportType.hxx>
#include <resip/stack/Tuple.hxx>

#include "XmlRpcServerBase.hxx"

namespace clicktocall
{

class XmlRpcConnection
{
   friend class XmlRpcServerBase;
      
public:
   XmlRpcConnection(XmlRpcServerBase& server, resip::Socket sock);
   virtual ~XmlRpcConnection();
   
   unsigned int getConnectionId() const { return mConnectionId; }
   void buildFdSet(resip::FdSet& fdset);
   bool process(resip::FdSet& fdset);

   virtual bool sendResponse(unsigned int requestId, const resip::Data& responseData, bool isFinal);

private:
   bool processSomeReads();
   bool processSomeWrites();
   bool tryParse(); // returns true if we processed something and there is more data in the buffer
            
   XmlRpcServerBase& mXmlRcpServer;
   const unsigned int mConnectionId;
   static unsigned int NextConnectionId;

   unsigned int mNextRequestId;
   typedef std::map<unsigned int, resip::Data> RequestMap;
   RequestMap mRequests;
           
   resip::Socket mSock;
   resip::Data mRxBuffer;
   resip::Data mTxBuffer;
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

