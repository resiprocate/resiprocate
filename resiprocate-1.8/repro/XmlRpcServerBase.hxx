#if !defined(XmlRpcServerBase_hxx)
#define XmlRpcServerBase_hxx 

#include <map>
#include <rutil/Data.hxx>
#include <rutil/Socket.hxx>
#include <rutil/TransportType.hxx>
#include <rutil/Fifo.hxx>
#include <resip/stack/Tuple.hxx>
#include <rutil/SelectInterruptor.hxx>

/// This Class is used to implement a primitive form of RPC using loose XML formatting.
/// The XML formatting used is specific to this implementation and is NOT currently intended to 
/// be compatible with the XML-RPC protocol defined by the following:  http://www.xmlrpc.com/

namespace repro
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
   XmlRpcServerBase(int port, resip::IpVersion version, resip::Data ipAddr = resip::Data::Empty);
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

   // thread safe - uses fifo (use connectionId == 0 to send to all connections)
   void sendEvent(unsigned int connectionId,
                  const resip::Data& eventData);

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

