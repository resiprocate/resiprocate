#if !defined(XmlRpcServerBase_hxx)
#define XmlRpcServerBase_hxx 

#include <map>
#include <rutil/Data.hxx>
#include <rutil/Socket.hxx>
#include <rutil/TransportType.hxx>
#include <rutil/Fifo.hxx>
#include <resip/stack/Tuple.hxx>
#include <rutil/SelectInterruptor.hxx>

namespace clicktocall
{
class XmlRpcConnection;

class ResponseInfo
{
public:
   ResponseInfo(unsigned int connectionId,
                unsigned int requestId,
                const resip::Data& responseData,
                bool isFinal) :
      mConnectionId(connectionId),
      mRequestId(requestId),
      mResponseData(responseData),
      mIsFinal(isFinal) {}

   ~ResponseInfo() {}

   unsigned int getConnectionId() const { return mConnectionId; }
   unsigned int getRequestId() const { return mRequestId; }
   const resip::Data& getResponseData() const { return mResponseData; }
   bool getIsFinal() const { return mIsFinal; }

private:
   unsigned int mConnectionId;
   unsigned int mRequestId;
   resip::Data mResponseData;
   bool mIsFinal;
};

class XmlRpcServerBase
{
   friend class XmlRpcConnection;
      
public:
   XmlRpcServerBase(int port, resip::IpVersion version);
   virtual ~XmlRpcServerBase();
      
   void buildFdSet(resip::FdSet& fdset);
   void process(resip::FdSet& fdset);

   bool isSane();
   static void logSocketError(int e);

   // thread safe - uses fifo
   void sendResponse(unsigned int connectionId,
                     unsigned int requestId, 
                     const resip::Data& responseData,
                     bool isFinal=true);

protected:
   virtual void handleRequest(unsigned int connectionId, 
                              unsigned int requestId, 
                              const resip::Data& request) = 0; 
      
private:
   static const unsigned int MaxConnections = 60;   // Note:  use caution if making this any bigger, default fd_set size in windows is 64
      
   resip::Socket mFd;
   resip::Tuple mTuple;
   bool mSane;

   typedef std::map<unsigned int, XmlRpcConnection*> ConnectionMap;
   ConnectionMap mConnections;
   void closeOldestConnection();

   resip::Fifo<ResponseInfo> mResponseFifo;
   resip::SelectInterruptor mSelectInterruptor;
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

