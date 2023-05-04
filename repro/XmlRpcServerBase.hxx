#if !defined(XmlRpcServerBase_hxx)
#define XmlRpcServerBase_hxx 

#include <map>
#include <memory>
#include <rutil/Data.hxx>
#include <rutil/Socket.hxx>
#include <rutil/TransportType.hxx>
#include <rutil/Fifo.hxx>
#include <resip/stack/Tuple.hxx>
#include <rutil/SelectInterruptor.hxx>
#include <rutil/ThreadIf.hxx>

#ifdef BUILD_QPID_PROTON
#include "rutil/ProtonThreadBase.hxx"
#include <proton/message.hpp>
#endif

#include <memory>

/// This Class is used to implement a primitive form of RPC using loose XML formatting.
/// The XML formatting used is specific to this implementation and is NOT currently intended to 
/// be compatible with the XML-RPC protocol defined by the following:  http://www.xmlrpc.com/

namespace repro
{
class XmlRpcConnectionBase;

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

   unsigned int getConnectionId() const noexcept { return mConnectionId; }
   unsigned int getRequestId() const noexcept { return mRequestId; }
   const resip::Data& getResponseData() const noexcept { return mResponseData; }
   bool getIsFinal() const noexcept { return mIsFinal; }

private:
   unsigned int mConnectionId;
   unsigned int mRequestId;
   resip::Data mResponseData;
   bool mIsFinal;
};

class XmlRpcServerBase;

class XmlRpcHandler
{
public:
   XmlRpcHandler(std::shared_ptr<XmlRpcServerBase> rpc) : mRpc(rpc) {};
   virtual ~XmlRpcHandler();
   virtual void handleRequest(unsigned int connectionId,
                              unsigned int requestId,
                              const resip::Data& request) = 0;
   virtual void buildFdSet(resip::FdSet& fdset);
   void process(resip::FdSet& fdset);
   bool isSane();
protected:
   std::shared_ptr<XmlRpcServerBase> mRpc;
};

class XmlRpcServerBase
{
   friend class XmlRpcConnectionBase;

protected:
   XmlRpcServerBase(XmlRpcHandler& h);
   virtual void processIncoming(resip::FdSet& fdset) = 0;
   void setSane(bool sane) { mSane = sane; };

public:
   virtual ~XmlRpcServerBase();

   virtual void buildFdSet(resip::FdSet& fdset);
   void process(resip::FdSet& fdset);

   bool isSane();

   // thread safe - uses fifo
   virtual void sendResponse(unsigned int connectionId,
                     unsigned int requestId,
                     const resip::Data& responseData,
                     bool isFinal=true);

   // thread safe - uses fifo (use connectionId == 0 to send to all connections)
   virtual void sendEvent(unsigned int connectionId,
                  const resip::Data& eventData);

   virtual void handleRequest(unsigned int connectionId,
                              unsigned int requestId,
                              const resip::Data& request) { mHandler.handleRequest(connectionId, requestId, request); };

   XmlRpcHandler& mHandler;
   bool mSane;
   typedef std::map<unsigned int, XmlRpcConnectionBase*> ConnectionMap;
   ConnectionMap mConnections;
   resip::Fifo<ResponseInfo> mResponseFifo;
   resip::SelectInterruptor mSelectInterruptor;
};

class XmlRpcSocketServer : public XmlRpcServerBase
{
   friend class XmlRpcConnectionBase;

public:
   XmlRpcSocketServer(XmlRpcHandler& h, int port, resip::IpVersion version, resip::Data ipAddr = resip::Data::Empty);
   virtual ~XmlRpcSocketServer();

   virtual void buildFdSet(resip::FdSet& fdset);

   static void logSocketError(int e);

protected:
   void processIncoming(resip::FdSet& fdset);

private:
   static const unsigned int MaxConnections = 60;   // Note:  use caution if making this any bigger, default fd_set size in windows is 64
      
   resip::Socket mFd;
   resip::Tuple mTuple;

   void closeOldestConnection();
};

#ifdef BUILD_QPID_PROTON
class XmlRpcProtonServer : public XmlRpcServerBase,
                           public resip::ProtonThreadBase::ProtonReceiverBase,
                           public std::enable_shared_from_this<XmlRpcProtonServer>
{
   friend class XmlRpcProtonConnection;

public:
   XmlRpcProtonServer(XmlRpcHandler& h, const resip::Data& brokerUrl, bool broadcast);
   virtual ~XmlRpcProtonServer();

   virtual void buildFdSet(resip::FdSet& fdset) override;
   //void process(resip::FdSet& fdset);

protected:
   virtual void processIncoming(resip::FdSet& fdset) override;

private:
   resip::Data mBrokerUrl;
   bool mBroadcast;
   bool mReady;
   std::shared_ptr<resip::ProtonThreadBase> mQpidProtonThread;
   std::shared_ptr<resip::ProtonThreadBase::ProtonSenderBase> mProtonSender;

   unsigned int mConnectionId = 0;
   unsigned int mNextRequestId = 0;
   std::map<resip::Data, unsigned int> mQueueConnection;
};
#endif

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

