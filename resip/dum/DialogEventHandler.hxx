#if !defined(RESIP_DialogEventHandler_HXX)
#define RESIP_DialogEventHandler_HXX

#include "resip/dum/DialogEventInfo.hxx"
#include "resip/dum/InviteSessionHandler.hxx"

namespace resip
{

class DialogEvent
{
public:
   DialogEvent() {}
   DialogEvent(const DialogEvent& rhs) {}
   virtual ~DialogEvent() {}

   enum DialogEventType
   {
      DialogEventType_Trying,
      DialogEventType_Proceeding,
      DialogEventType_Early,
      DialogEventType_Confirmed,
      DialogEventType_Terminated,
      DialogEventType_MultipleEvents
   };
   virtual DialogEventType getType() const = 0;
};

class TryingDialogEvent : public DialogEvent
{
public:
   TryingDialogEvent(const DialogEventInfo& info, const SipMessage& initialInvite)
      : mEventInfo(info), mInitialInvite(initialInvite)
   {}
   TryingDialogEvent(const TryingDialogEvent& rhs)
      : mEventInfo(rhs.mEventInfo), mInitialInvite(rhs.mInitialInvite)
   {}
   virtual ~TryingDialogEvent() {}

   const DialogEventInfo& getEventInfo() const { return mEventInfo; }
   const SipMessage& getInitialInvite() const { return mInitialInvite; }
   DialogEventType getType() const { return DialogEvent::DialogEventType_Trying; }

private:
   DialogEventInfo mEventInfo;
   SipMessage mInitialInvite;
};

class ProceedingDialogEvent : public DialogEvent
{
public:
   ProceedingDialogEvent(const DialogEventInfo& info) 
      : mEventInfo(info)
   {}
   ProceedingDialogEvent(const ProceedingDialogEvent& rhs)
      : mEventInfo(rhs.mEventInfo)
   {}
   virtual ~ProceedingDialogEvent() {}

   const DialogEventInfo& getEventInfo() const { return mEventInfo; }
   DialogEventType getType() const { return DialogEvent::DialogEventType_Proceeding; }

private:
   DialogEventInfo mEventInfo;
};

class EarlyDialogEvent : public DialogEvent
{
public:
   EarlyDialogEvent(const DialogEventInfo& info)
      : mEventInfo(info)
   {}
   EarlyDialogEvent(const EarlyDialogEvent& rhs)
      : mEventInfo(rhs.mEventInfo)
   {
   }
   virtual ~EarlyDialogEvent() {}

   const DialogEventInfo& getEventInfo() const { return mEventInfo; }
   DialogEventType getType() const { return DialogEvent::DialogEventType_Early; }

private:
   DialogEventInfo mEventInfo;
};

class ConfirmedDialogEvent : public DialogEvent
{
public:
   ConfirmedDialogEvent(const DialogEventInfo& info) 
      : mEventInfo(info)
   {}
   ConfirmedDialogEvent(const ConfirmedDialogEvent& rhs)
      : mEventInfo(rhs.mEventInfo)
   {}
   virtual ~ConfirmedDialogEvent() {}

   const DialogEventInfo& getEventInfo() const { return mEventInfo; }
   DialogEventType getType() const { return DialogEvent::DialogEventType_Confirmed; }

private:
   DialogEventInfo mEventInfo;
};

class TerminatedDialogEvent : public DialogEvent
{
public:
   TerminatedDialogEvent(const DialogEventInfo& info, InviteSessionHandler::TerminatedReason reason, int code)
      : mEventInfo(info), mReason(reason), mCode(code)
   {}
   TerminatedDialogEvent(const TerminatedDialogEvent& rhs)
      : mEventInfo(rhs.mEventInfo), mReason(rhs.mReason), mCode(rhs.mCode)
   {}
   virtual ~TerminatedDialogEvent() {}

   const DialogEventInfo& getEventInfo() const { return mEventInfo; }
   InviteSessionHandler::TerminatedReason getTerminatedReason() const { return mReason; }
   int getResponseCode() const { return mCode; }
   DialogEventType getType() const { return DialogEvent::DialogEventType_Terminated; }

private:
   DialogEventInfo mEventInfo;
   InviteSessionHandler::TerminatedReason mReason;
   int mCode;
};

class MultipleEventDialogEvent : public DialogEvent
{
public:
   typedef std::vector< SharedPtr<DialogEvent> > EventVector;
   MultipleEventDialogEvent(EventVector& events) 
      : mEvents(events)
   {}
   MultipleEventDialogEvent(const MultipleEventDialogEvent& rhs)
      : mEvents(rhs.mEvents)
   {}
   virtual ~MultipleEventDialogEvent() {}

   const EventVector& getEvents() const { return mEvents; }
   DialogEventType getType() const { return DialogEvent::DialogEventType_MultipleEvents; }

private:
   EventVector mEvents;
};

class DialogEventHandler
{
   public:   
      virtual ~DialogEventHandler() {}
      virtual void onTrying(const TryingDialogEvent& evt)=0;
      virtual void onProceeding(const ProceedingDialogEvent& evt)=0;
      virtual void onEarly(const EarlyDialogEvent& evt)=0;
      virtual void onConfirmed(const ConfirmedDialogEvent& evt)=0; 
      virtual void onTerminated(const TerminatedDialogEvent& evt)=0;
      virtual void onMultipleEvents(const MultipleEventDialogEvent& evt)=0;
};
      

}

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
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
