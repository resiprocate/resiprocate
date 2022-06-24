#if !defined(Kurento_Event_hxx)
#define Kurento_Event_hxx

#include <memory>
#include <string>

#include "cajun/json/elements.h"

#include "Continuation.hxx"

namespace kurento
{

class Event
{
   public:
      static std::shared_ptr<Event> make_event(const std::string& eventType, const json::Object& message);
      virtual ~Event(); // FIXME


   protected:
      Event(const std::string& name);

   private:
      friend std::ostream& operator<<(std::ostream& strm, const Event& e);

      std::string mName;
};

inline std::ostream& operator<<(std::ostream& strm, const Event& e)
{
   strm << e.mName;
   return strm;
}

class OnIceCandidateFoundEvent : public Event
{
   public:
      static const std::string EVENT_NAME;

      OnIceCandidateFoundEvent(const json::Object& message);
      virtual ~OnIceCandidateFoundEvent();

      const std::string& getCandidate() const { return mCandidate; };
      unsigned int getLineIndex() const { return mLineIndex; };
      const std::string& getId() const { return mId; };

   private:
      std::string mCandidate;
      unsigned int mLineIndex;
      std::string mId;
};

class OnIceGatheringDoneEvent : public Event
{
   public:
      static const std::string EVENT_NAME;

      OnIceGatheringDoneEvent(const json::Object& message);
      virtual ~OnIceGatheringDoneEvent();
};

class OnConnectionStateChangedEvent : public Event
{
   public:
      static const std::string EVENT_NAME;

      OnConnectionStateChangedEvent(const json::Object& message);
      virtual ~OnConnectionStateChangedEvent();
};

class OnMediaStateChangedEvent : public Event
{
   public:
      static const std::string EVENT_NAME;

      OnMediaStateChangedEvent(const json::Object& message);
      virtual ~OnMediaStateChangedEvent();
};

class OnMediaTranscodingStateChangeEvent : public Event
{
   public:
      static const std::string EVENT_NAME;

      OnMediaTranscodingStateChangeEvent(const json::Object& message);
      virtual ~OnMediaTranscodingStateChangeEvent();
};

class OnMediaFlowInStateChangeEvent : public Event
{
   public:
      static const std::string EVENT_NAME;

      OnMediaFlowInStateChangeEvent(const json::Object& message);
      virtual ~OnMediaFlowInStateChangeEvent();
};

class OnMediaFlowOutStateChangeEvent : public Event
{
   public:
      static const std::string EVENT_NAME;

      OnMediaFlowOutStateChangeEvent(const json::Object& message);
      virtual ~OnMediaFlowOutStateChangeEvent();
};

class OnKeyframeRequiredEvent : public Event
{
   public:
      static const std::string EVENT_NAME;

      OnKeyframeRequiredEvent(const json::Object& message);
      virtual ~OnKeyframeRequiredEvent();
};

class OnErrorEvent : public Event
{
   public:
      static const std::string EVENT_NAME;

      OnErrorEvent(const json::Object& message);
      virtual ~OnErrorEvent();
};

class EventListener
{
   public:
      virtual ~EventListener() {};
      virtual void onEvent(std::shared_ptr<Event> event) = 0;
};

typedef std::function<void (std::shared_ptr<Event> event) >
ContinuationEvent;

class EventContinuation : public EventListener
{
   public:
      EventContinuation(ContinuationEvent c) : mContinuation(c) {};
      virtual ~EventContinuation() {};
      virtual void onEvent(std::shared_ptr<Event> event) override { mContinuation(event); };
   private:
      ContinuationEvent mContinuation;
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
