#if !defined(KurentoConnection_hxx)
#define KurentoConnection_hxx

#include <deque>
#include <string>
#include <memory>

#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include "cajun/json/elements.h"

#include "KurentoResponseHandler.hxx"

// from kms-jsonrpc JsonRpcConstants.hpp
#define JSON_RPC_PROTO "jsonrpc"
#define JSON_RPC_PROTO_VERSION "2.0"
#define JSON_RPC_ID "id"
#define JSON_RPC_METHOD "method"
#define JSON_RPC_PARAMS "params"
#define JSON_RPC_RESULT "result"
#define JSON_RPC_ERROR "error"
#define JSON_RPC_ERROR_CODE "code"
#define JSON_RPC_ERROR_MESSAGE "message"
#define JSON_RPC_ERROR_DATA "data"

#define JSON_RPC_OBJECT "object"
#define JSON_RPC_VALUE "value"
#define JSON_RPC_TYPE "type"
#define JSON_RPC_SESSION_ID "sessionId"
#define JSON_RPC_MEDIAPIPELINE "mediaPipeline"
#define JSON_RPC_SINK "sink"

namespace kurento
{

typedef websocketpp::client<websocketpp::config::asio_client> client;

class Object;

class KurentoConnection
{
   public:
      typedef std::shared_ptr<KurentoConnection> ptr;

      KurentoConnection(client& wSClient, websocketpp::connection_hdl hdl, std::string uri, unsigned int timeout, bool waitForResponse = true);
      virtual ~KurentoConnection();

      void onOpen(client* wSClient, websocketpp::connection_hdl h);
      void onFail(client* wSClient, websocketpp::connection_hdl h);
      void onClose(client* wSClient, websocketpp::connection_hdl h);
      void onMessage(client* wSClient, websocketpp::connection_hdl, client::message_ptr msg);

      std::string sendRequest(std::shared_ptr<KurentoResponseHandler> krh, const std::string& method, const json::Object& params);

      void registerObject(std::shared_ptr<KurentoResponseHandler> object);
      void unregisterObject(const std::string& objectId);

   private:

      void processSendQueue();
      void sendMessage(const std::string& msg);

      void onResponse(const std::string& id, std::shared_ptr<KurentoResponseHandler> krh, const json::Object& message);
      void onEvent(const std::string& eventName, const json::Object& message);

      client& mWSClient;
      websocketpp::connection_hdl mHandle;
      unsigned int mTimeout;
      bool mWaitForResponse;
      unsigned long mNextId = 1;
      unsigned long mLastResponse = 0;
      unsigned long mRequestSentCount = 0;
      unsigned long mResponseReceivedCount = 0;
      std::string mSessionId;
      std::deque<std::string> mSendQueue;  // FIXME: post events to queue from other threads?
      typedef std::map<std::string, std::shared_ptr<KurentoResponseHandler> > KurentoResponseHandlerMap;
      KurentoResponseHandlerMap mResponseHandlers;

      typedef std::map<std::string, std::shared_ptr<Object> > KurentoObjectMap;
      KurentoObjectMap mObjects;

};

}

#endif

/* ====================================================================

 Copyright (c) 2021, Software Freedom Institute https://softwarefreedom.institute
 Copyright (c) 2021, Daniel Pocock https://danielpocock.com
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are
 met:

 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 3. Neither the name of Plantronics nor the names of its contributors
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
