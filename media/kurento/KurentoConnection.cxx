
#include "cajun/json/writer.h"

#include "rutil/Logger.hxx"
#include "rutil/Subsystem.hxx"

#include "KurentoConnection.hxx"
#include "Object.hxx"

using namespace kurento;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::APP  // FIXME: MEDIA or KURENTO?

//#define PING_MSG "{\"id\":\"1\",\"method\":\"ping\",\"params\":{\"interval\":240000},\"jsonrpc\":\"2.0\"}"



KurentoConnection::KurentoConnection(kurento::client& wSClient, websocketpp::connection_hdl hdl, std::string uri, unsigned int timeout, bool waitForResponse)
   : mWSClient(wSClient),
     mHandle(hdl),
     mTimeout(timeout),  // FIXME - we don't use mTimeout yet
     mWaitForResponse(waitForResponse)
{

}

KurentoConnection::~KurentoConnection()
{
   // FIXME
}

void
KurentoConnection::onOpen(client* wSClient, websocketpp::connection_hdl h)
{
   InfoLog(<<"onOpen");
   processSendQueue();
   // FIXME
}

void
KurentoConnection::onFail(client* wSClient, websocketpp::connection_hdl h)
{
   ErrLog(<<"onFail");
   resip_assert(0);  // FIXME - reconnect?
}

void
KurentoConnection::onClose(client* wSClient, websocketpp::connection_hdl h)
{
   ErrLog(<<"onClose");
   resip_assert(0);  // FIXME - reconnect?
}

void
KurentoConnection::onMessage(client* wSClient, websocketpp::connection_hdl h, client::message_ptr msg)
{
   if (msg->get_opcode() != websocketpp::frame::opcode::text)
   {
      ErrLog(<<"received unknown message type, ignoring it");
      return;
   }

   const std::string& m = msg->get_payload();
   DebugLog(<<"received a message: " << m.c_str());

   json::Object message;
   std::stringstream stream;
   stream << m;
   try
   {
      json::Reader::Read(message, stream);
   }
   catch (json::Reader::ParseException& e)
   {
      // lines/offsets are zero-indexed, so bump them up by one for human presentation
      DebugLog(<<"Caught json::ParseException: " << e.what() << ", Line/offset: " << e.m_locTokenBegin.m_nLine + 1
               << '/' << e.m_locTokenBegin.m_nLineOffset + 1);
      //m_messages.push_back("<< " + msg->get_payload());
      return;
   }

   if(mSessionId.length() == 0)
   {
      if(message.Find(JSON_RPC_RESULT) != message.End())
      {
         json::Object& result = message[JSON_RPC_RESULT];
         if(result.Find(JSON_RPC_SESSION_ID) != result.End())
         {
            const json::String& sessionId = result[JSON_RPC_SESSION_ID];
            mSessionId = sessionId.Value();
            DebugLog(<<"registered new sessionId: " << mSessionId);
         }
      }
   }

   if(message.Find(JSON_RPC_ID) != message.End())
   {
      const json::String& idValue = message[JSON_RPC_ID];
      std::string id = idValue.Value();
      DebugLog(<<"has id = '" << id << "', response");

      const KurentoResponseHandlerMap::const_iterator it = mResponseHandlers.find(id);

      if(it != mResponseHandlers.end())
      {
         onResponse(id, it->second, message);
      }
      else
      {
         WarningLog(<<"unrecognised id = '" << id << "'");
      }

   }
   else
   {
      DebugLog(<<"has no id, checking if it is a notification");
      if(message.Find(JSON_RPC_METHOD) != message.End() && message[json::String(JSON_RPC_METHOD)] == json::String("onEvent"))
      {
         const json::String& eventName = message[JSON_RPC_PARAMS][JSON_RPC_VALUE][JSON_RPC_TYPE];
         const std::string& _eventName = eventName.Value();
         DebugLog(<<"received an event: " << _eventName);
         // the handler is probably in a different thread
         onEvent(_eventName, message);
      }
      else
      {
         DebugLog(<<"don't know how to handle the message: " << m);
      }
   }

   // FIXME
}

void
KurentoConnection::onResponse(const std::string& id, std::shared_ptr<KurentoResponseHandler> krh, const json::Object& message)
{
   DebugLog(<<"id: " << id);
   unsigned long _id = std::stol(id);
   mResponseHandlers.erase(id);
   krh->processResponse(id, krh, message);
   if(_id > mLastResponse)
   {
      mLastResponse = _id;
   }
   else
   {
      WarningLog(<<"mLastResponse = " << mLastResponse << " but received response for earlier request: " << _id);
   }
   mResponseReceivedCount++;
   DebugLog(<< "mRequestSentCount = " << mRequestSentCount
            << " mResponseReceivedCount = " << mResponseReceivedCount);
   processSendQueue();
}

void
KurentoConnection::onEvent(const std::string& eventName, const json::Object& message)
{
   DebugLog(<<"event: " << eventName);

   const json::Object& values = message[JSON_RPC_PARAMS][JSON_RPC_VALUE];

   const json::String& objectId = values["object"];
   const std::string& _objectId = objectId.Value();

   if(mObjects.find(_objectId) != mObjects.end())
   {
      std::shared_ptr<Object> object = mObjects[_objectId];
      DebugLog(<<"event is being routed to Object " << _objectId);
      object->onEvent(eventName, message);
   }
   else
   {
      WarningLog(<<"event is for unknown Object " << _objectId);
   }

   // FIXME - does every event have an Object?
}

void
KurentoConnection::sendMessage(const std::string& msg)
{
   mSendQueue.push_back(msg);
   DebugLog(<< "mRequestSentCount = " << mRequestSentCount
            << " mResponseReceivedCount = " << mResponseReceivedCount);
   processSendQueue();
}

void
KurentoConnection::processSendQueue()
{
   while(!mSendQueue.empty())
   {
      if(mWaitForResponse && mRequestSentCount > mResponseReceivedCount)
      {
         DebugLog(<<"new request to send but still waiting for response for a previous request");
         return;
      }
      if(mWSClient.get_con_from_hdl(mHandle)->get_state() == websocketpp::session::state::value::open)
      {
         std::string& msg = mSendQueue.front();
         websocketpp::lib::error_code ec;
         mWSClient.send(mHandle, msg.c_str(), websocketpp::frame::opcode::text, ec);
         if(ec)
         {
            ErrLog(<<"failed to send a message to Kurento"); // FIXME
            return;
         }
         else
         {
            StackLog(<<"message sent to Kurento, removing from queue: " << msg);
            mSendQueue.pop_front();
            mRequestSentCount++;
         }
      }
      else
      {
         ErrLog(<<"connection to Kurento is not in the open state, message has been queued"); // FIXME
         return;
      }
   }
}

std::string
KurentoConnection::sendRequest(std::shared_ptr<KurentoResponseHandler> krh, const std::string& method, const json::Object& params)
{
   const std::string id = std::to_string(mNextId++);
   StackLog(<<"generated ID " << id);
   json::Object request;
   json::Object _params = params;
   if(mSessionId.length() > 0)
   {
      _params[JSON_RPC_SESSION_ID] = json::String(mSessionId);
   }

   request[JSON_RPC_ID] = json::String(id);
   request[JSON_RPC_METHOD] = json::String(method);
   request[JSON_RPC_PARAMS] = _params;
   request[JSON_RPC_PROTO] = json::String(JSON_RPC_PROTO_VERSION);

   std::stringstream stream;
   json::Writer::Write(request, stream);

   mResponseHandlers[id] = krh;
   StackLog(<<"added ResponseHandler for id '" << id << "', handler count: " << mResponseHandlers.size());

   sendMessage(stream.str());

   return id;
}

void
KurentoConnection::registerObject(std::shared_ptr<KurentoResponseHandler> object)
{
   std::shared_ptr<Object> _object = std::static_pointer_cast<Object>(object);
   mObjects[_object->getId()] = _object;
}

// FIXME - we don't call this method from anywhere
void
KurentoConnection::unregisterObject(const std::string& objectId)
{
   mObjects.erase(objectId);
}

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
