
#include "rutil/Logger.hxx"
#include "rutil/Subsystem.hxx"
#include "rutil/ResipAssert.h"

#include "Object.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::APP  // FIXME: MEDIA or KURENTO?

using namespace kurento;

using std::placeholders::_1;
using std::placeholders::_2;

Object::Object(const std::string& name, std::shared_ptr<KurentoConnection> connection)
   : mName(name),
     mConnection(connection)
{
}

Object::~Object()
{

}

std::string
Object::makeRpcCall(const std::string& methodName, const json::Object& params, ContinuationInternal c)
{
   std::string reqId = mConnection->sendRequest(
            KurentoResponseHandler::shared_from_this(),
            methodName, params);
   mContinuations[reqId] = c;
   return reqId;
}

void
Object::createObject(ContinuationVoid c, const json::Object& constructorParams)
{
   json::Object params;
   params[JSON_RPC_TYPE] = json::String(mName);
   params["constructorParams"] = constructorParams;
   params["properties"] = json::Object();
   ContinuationInternal ci = std::bind(&Object::onConstructorSuccess, this, c, _1);
   std::string reqId = makeRpcCall("create", params, ci);
}

void
Object::subscribe(const std::string& eventName, ContinuationVoid c)
{
   json::Object params;
   params[JSON_RPC_OBJECT] = json::String(mId);
   params[JSON_RPC_TYPE] = json::String(eventName);
   ContinuationInternal ci = std::bind(&Object::onSubscribeSuccess, this, c, _1);
   std::string reqId = makeRpcCall("subscribe", params, ci);
}

void
Object::onEvent(const std::string& eventType, const json::Object& message)
{
   if(mEventListeners.find(eventType) == mEventListeners.end())
   {
      WarningLog(<<"notification for event that we didn't subscribe to " << eventType);
      return;
   }

   if(eventType == OnIceGatheringDoneEvent::EVENT_NAME)
   {
      sendNotifications<OnIceGatheringDoneEvent>(eventType, message);
   }
   else
   {
      ErrLog(<<"unknown eventType " << eventType);
      return;
   }
}

template <typename T>
void
Object::sendNotifications(const std::string& eventType, const json::Object& message)
{
   std::shared_ptr<T> event = std::make_shared<T>(message);

   EventListenerList listeners = mEventListeners[eventType];
   EventListenerList::const_iterator it;
   for(it = listeners.begin(); it != listeners.end(); it++)
   {
      //std::shared_ptr<EventListener<T> > l = std::dynamic_pointer_cast<EventListener<T> >(*it);
      std::shared_ptr<EventListener> l = *it;
      l->onEvent(event);
   }
}

void
Object::addListener(const std::string& eventName, std::shared_ptr<EventListener> l, ContinuationVoid c)
{
   //resip_assert(std::is_base_of<Event,T>::value);  // FIXME uncomment

   //const std::string& eventName = T::EVENT_NAME;
   //std::shared_ptr<EventListener<Event> > _l = std::static_pointer_cast<EventListener<Event> >(l);
   mEventListeners[eventName].push_back(l);

   subscribe(eventName, c);
}

void
Object::invokeVoidMethod(const std::string& methodName, ContinuationVoid c, const json::Object& methodParams)
{
   json::Object params;
   params[JSON_RPC_OBJECT] = json::String(mId);
   params["operation"] = json::String(methodName);
   params["operationParams"] = methodParams;
   params["properties"] = json::Object();

   ContinuationInternal ci = std::bind(&Object::onVoidSuccess, this, c, _1);

   std::string reqId = makeRpcCall("invoke", params, ci);
}

void
Object::invokeStringMethod(const std::string& methodName, ContinuationString c, const json::Object& methodParams)
{
   json::Object params;
   params[JSON_RPC_OBJECT] = json::String(mId);
   params["operation"] = json::String(methodName);
   params["operationParams"] = methodParams;
   params["properties"] = json::Object();

   ContinuationInternal ci = std::bind(&Object::onStringSuccess, this, c, _1);

   std::string reqId = makeRpcCall("invoke", params, ci);
}

void
Object::processResponse(const std::string& id, std::shared_ptr<KurentoResponseHandler> krh, const json::Object& message)
{
   const ContinuationMap::const_iterator it = mContinuations.find(id);
   if(it == mContinuations.end())
   {
      ErrLog(<<"received response for unknown request");
      return; // FIXME
   }
   const ContinuationInternal& c = it->second;
   c(message);
   mContinuations.erase(id);
}

void
Object::onConstructorSuccess(ContinuationVoid c, const json::Object& message)
{
   json::String idValue = message[JSON_RPC_RESULT][JSON_RPC_VALUE];
   mId = idValue.Value();
   DebugLog(<<"Object: " << mName << " got Kurento Object ID: " << mId);
   mConnection->registerObject(shared_from_this());
   c();
}

void
Object::onSubscribeSuccess(ContinuationVoid c, const json::Object& message)
{
   // FIXME - do we need anything from the message?
   c();
}

void
Object::onVoidSuccess(ContinuationVoid c, const json::Object& message)
{
   DebugLog(<<"successfully executed RPC method without a return value");
   c();
}

void
Object::onStringSuccess(ContinuationString c, const json::Object& message)
{
   DebugLog(<<"successfully executed RPC method returning string");
   json::String value = message[JSON_RPC_RESULT][JSON_RPC_VALUE];
   std::string& _value = value.Value();
   DebugLog(<<"return value: " << _value);
   c(_value);
}

MediaPipeline::MediaPipeline(std::shared_ptr<KurentoConnection> connection)
   : Object("MediaPipeline", connection)
{
   //create();
}

MediaPipeline::~MediaPipeline()
{
}

Element::Element(const std::string& name, std::shared_ptr<MediaPipeline> mediaPipeline)
   : Object(name, mediaPipeline->getConnection()),
     mMediaPipeline(mediaPipeline)
{
}

Element::~Element()
{
}

void
Element::create(ContinuationVoid c)
{
   json::Object params;
   params[JSON_RPC_MEDIAPIPELINE] = json::String(mMediaPipeline->getId());
   createObject(c, params);
}

void
Element::connect(ContinuationVoid c, Element& element)
{
   json::Object params;
   params[JSON_RPC_SINK] = json::String(element.getId());
   invokeVoidMethod("connect", c, params);
}

void
Element::disconnect(ContinuationVoid c, Element& element)
{
   json::Object params;
   params[JSON_RPC_SINK] = json::String(element.getId());
   invokeVoidMethod("disconnect", c, params);
}

BaseRtpEndpoint::BaseRtpEndpoint(const std::string& name, std::shared_ptr<MediaPipeline> mediaPipeline)
   : Element(name, mediaPipeline)
{
}

BaseRtpEndpoint::~BaseRtpEndpoint()
{
}

void
BaseRtpEndpoint::setExternalIPv4(ContinuationVoid c, const std::string& addr)
{
   json::Object params;
   params["externalIPv4"] = json::String(addr);
   invokeVoidMethod("setExternalIPv4", c, params);
}

void
BaseRtpEndpoint::setExternalIPv6(ContinuationVoid c, const std::string& addr)
{
   json::Object params;
   params["externalIPv6"] = json::String(addr);
   invokeVoidMethod("setExternalIPv6", c, params);
}

void
BaseRtpEndpoint::generateOffer(ContinuationString c)
{
   invokeStringMethod("generateOffer", c);
}

void
BaseRtpEndpoint::processOffer(ContinuationString c, const std::string& sdp)
{
   json::Object params;
   params["offer"] = json::String(sdp);
   invokeStringMethod("processOffer", c, params);
}

void
BaseRtpEndpoint::getLocalSessionDescriptor(ContinuationString c)
{
   invokeStringMethod("getLocalSessionDescriptor", c);
}

RtpEndpoint::RtpEndpoint(std::shared_ptr<MediaPipeline> mediaPipeline)
   : BaseRtpEndpoint("RtpEndpoint", mediaPipeline)
{
}

RtpEndpoint::~RtpEndpoint()
{
}

WebRtcEndpoint::WebRtcEndpoint(std::shared_ptr<MediaPipeline> mediaPipeline)
   : BaseRtpEndpoint("WebRtcEndpoint", mediaPipeline)
{
   //create();
}

WebRtcEndpoint::~WebRtcEndpoint()
{
}

void
WebRtcEndpoint::gatherCandidates(ContinuationVoid c)
{
   invokeVoidMethod("gatherCandidates", c);
}

/*void
WebRtcEndpoint::addIceCandidate(ContinuationVoid c, const std::string& candidate)
{
   invokeVoidMethod("addIceCandidate", c);
}*/

void
WebRtcEndpoint::addOnIceGatheringDoneListener(std::shared_ptr<EventListener> l, ContinuationVoid c)
{
   addListener(OnIceGatheringDoneEvent::EVENT_NAME, l, c);
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
