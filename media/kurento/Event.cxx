#include "Event.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::APP  // FIXME: MEDIA or KURENTO?

using namespace kurento;

const std::string OnIceCandidateFoundEvent::EVENT_NAME = "IceCandidateFound";
const std::string OnIceGatheringDoneEvent::EVENT_NAME = "IceGatheringDone";
const std::string OnConnectionStateChangedEvent::EVENT_NAME = "ConnectionStateChanged";
const std::string OnMediaStateChangedEvent::EVENT_NAME = "MediaStateChanged";
const std::string OnMediaTranscodingStateChangeEvent::EVENT_NAME = "MediaTranscodingStateChange";
const std::string OnMediaFlowInStateChangeEvent::EVENT_NAME = "MediaFlowInStateChange";
const std::string OnMediaFlowOutStateChangeEvent::EVENT_NAME = "MediaFlowOutStateChange";
const std::string OnKeyframeRequiredEvent::EVENT_NAME = "KeyframeRequired";
const std::string OnErrorEvent::EVENT_NAME = "Error";

Event::Event(const std::string& name)
   : mName(name)
{
}

Event::~Event()
{
}

std::shared_ptr<Event>
Event::make_event(const std::string& eventType, const json::Object& message)
{
   Event* event = 0;

   if(eventType == OnIceCandidateFoundEvent::EVENT_NAME) {
      event = new OnIceCandidateFoundEvent(message);
   } else if(eventType == OnIceGatheringDoneEvent::EVENT_NAME) {
      event = new OnIceGatheringDoneEvent(message);
   } else if(eventType == OnConnectionStateChangedEvent::EVENT_NAME) {
      event = new OnConnectionStateChangedEvent(message);
   } else if(eventType == OnMediaStateChangedEvent::EVENT_NAME) {
      event = new OnMediaStateChangedEvent(message);
   } else if(eventType == OnMediaTranscodingStateChangeEvent::EVENT_NAME) {
      event = new OnMediaTranscodingStateChangeEvent(message);
   } else if(eventType == OnMediaFlowInStateChangeEvent::EVENT_NAME) {
      event = new OnMediaFlowInStateChangeEvent(message);
   } else if(eventType == OnMediaFlowOutStateChangeEvent::EVENT_NAME) {
      event = new OnMediaFlowOutStateChangeEvent(message);
   } else if(eventType == OnKeyframeRequiredEvent::EVENT_NAME) {
      event = new OnKeyframeRequiredEvent(message);
   } else if(eventType == OnErrorEvent::EVENT_NAME) {
      event = new OnErrorEvent(message);
   }
   return std::shared_ptr<Event>(event);
}

OnIceCandidateFoundEvent::OnIceCandidateFoundEvent(const json::Object& message)
   : Event(EVENT_NAME)
{
}

OnIceCandidateFoundEvent::~OnIceCandidateFoundEvent()
{
}

OnIceGatheringDoneEvent::OnIceGatheringDoneEvent(const json::Object& message)
   : Event(EVENT_NAME)
{
}

OnIceGatheringDoneEvent::~OnIceGatheringDoneEvent()
{
}

OnConnectionStateChangedEvent::OnConnectionStateChangedEvent(const json::Object& message)
   : Event(EVENT_NAME)
{
}

OnConnectionStateChangedEvent::~OnConnectionStateChangedEvent()
{
}

OnMediaStateChangedEvent::OnMediaStateChangedEvent(const json::Object& message)
   : Event(EVENT_NAME)
{
}

OnMediaStateChangedEvent::~OnMediaStateChangedEvent()
{
}

OnMediaTranscodingStateChangeEvent::OnMediaTranscodingStateChangeEvent(const json::Object& message)
   : Event(EVENT_NAME)
{
}

OnMediaTranscodingStateChangeEvent::~OnMediaTranscodingStateChangeEvent()
{
}

OnMediaFlowInStateChangeEvent::OnMediaFlowInStateChangeEvent(const json::Object& message)
   : Event(EVENT_NAME)
{
}

OnMediaFlowInStateChangeEvent::~OnMediaFlowInStateChangeEvent()
{
}

OnMediaFlowOutStateChangeEvent::OnMediaFlowOutStateChangeEvent(const json::Object& message)
   : Event(EVENT_NAME)
{
}

OnMediaFlowOutStateChangeEvent::~OnMediaFlowOutStateChangeEvent()
{
}

OnKeyframeRequiredEvent::OnKeyframeRequiredEvent(const json::Object& message)
   : Event(EVENT_NAME)
{
}

OnKeyframeRequiredEvent::~OnKeyframeRequiredEvent()
{
}

OnErrorEvent::OnErrorEvent(const json::Object& message)
   : Event(EVENT_NAME)
{
}

OnErrorEvent::~OnErrorEvent()
{
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
