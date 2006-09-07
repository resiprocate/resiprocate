// ==========================================================================================================
// InternalServerSubscriptionMessage.hxx                                                   2006 @ TelTel
// ==========================================================================================================
// Hanldes public methods asynchronizely in ServerSubscription.
// ==========================================================================================================
#ifndef RESIP_InternalServerSubscriptionMessage_hxx
#define RESIP_InternalServerSubscriptionMessage_hxx

#include "resiprocate/dum/InternalDumAsyncMessageBase.hxx"
#include "resiprocate/dum/ServerSubscription.hxx"

namespace resip
{
   // ---------------------------------------------------------------------------
   class InternalServerSubscriptionMessage_End : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerSubscriptionMessage_End);
      InternalServerSubscriptionMessage_End(ServerSubscriptionHandle& h) : mServerSubscriptionHandle(h) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerSubscriptionMessage_End"; }

   private: // methods.
      virtual void execute()
      {
         if (mServerSubscriptionHandle.isValid())
         {
            mServerSubscriptionHandle->end();
         }
      }
   private: // members.
      ServerSubscriptionHandle  mServerSubscriptionHandle;
   };
   // ---------------------------------------------------------------------------
   class InternalServerSubscriptionMessage_Send : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerSubscriptionMessage_Send);
      InternalServerSubscriptionMessage_Send(ServerSubscriptionHandle& h, const SipMessage& msg)
         : mServerSubscriptionHandle(h), mFlag(ar_NotUsed) {/*Empty*/}
      InternalServerSubscriptionMessage_Send(ServerSubscriptionHandle& h, bool accept, int statusCode) 
         : mServerSubscriptionHandle(h), mFlag(accept ? ar_Accept : ar_Reject), mStatusCode(statusCode) {/*Empty*/}
      InternalServerSubscriptionMessage_Send(ServerSubscriptionHandle& h, std::auto_ptr<Contents> updateContent)
         : mServerSubscriptionHandle(h), mUpdateContent(updateContent), mFlag(ar_Update) {/*Empty*/}
      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerSubscriptionMessage_Send"; }

   private: // methods.
      enum AcceptRejectFlag
      {
         ar_NotUsed = 0,
         ar_Accept  = 1,
         ar_Reject  = 2,
         ar_Update  = 3
      };
      virtual void execute()
      {
         if (mServerSubscriptionHandle.isValid())
         {
            switch(mFlag)
            {
            case ar_NotUsed:
               mServerSubscriptionHandle->send(mMsg);
               break;
            case ar_Accept:
               mServerSubscriptionHandle->send(mServerSubscriptionHandle->accept(mStatusCode));
               break;
            case ar_Reject:
               mServerSubscriptionHandle->send(mServerSubscriptionHandle->reject(mStatusCode));
               break;
            case ar_Update:
               mServerSubscriptionHandle->send(mServerSubscriptionHandle->update(mUpdateContent.get()));
               break;
            }
         }
      }
   private: // members.
      ServerSubscriptionHandle  mServerSubscriptionHandle;
      std::auto_ptr<Contents>   mUpdateContent;
      SipMessage                mMsg;
      AcceptRejectFlag          mFlag;
      int                       mStatusCode;
   };
   // ---------------------------------------------------------------------------
   class InternalServerSubscriptionMessage_DispatchSipMsg : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerSubscriptionMessage_DispatchSipMsg);
      InternalServerSubscriptionMessage_DispatchSipMsg(ServerSubscriptionHandle& h, const SipMessage& sipMsg)
         : mServerSubscriptionHandle(h), mSipMsg(sipMsg) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerSubscriptionMessage_DispatchSipMsg"; }

   private: // methods.
      virtual void execute()
      {
         if (mServerSubscriptionHandle.isValid())
         {
            mServerSubscriptionHandle->dispatch(mSipMsg);
         }
      }
   private: // members.
      ServerSubscriptionHandle mServerSubscriptionHandle;
      SipMessage              mSipMsg;
   };
   // ---------------------------------------------------------------------------
   class InternalServerSubscriptionMessage_DispatchTimeoutMsg : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerSubscriptionMessage_DispatchTimeoutMsg);
      InternalServerSubscriptionMessage_DispatchTimeoutMsg(ServerSubscriptionHandle& h, const DumTimeout& timeoutMsg)
         : mServerSubscriptionHandle(h), mTimeoutMsg(timeoutMsg) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerSubscriptionMessage_DispatchTimeoutMsg"; }

   private: // methods.
      virtual void execute()
      {
         if (mServerSubscriptionHandle.isValid())
         {
            mServerSubscriptionHandle->dispatch(mTimeoutMsg);
         }
      }
   private: // members.
      ServerSubscriptionHandle mServerSubscriptionHandle;
      DumTimeout              mTimeoutMsg;
   };
   // ---------------------------------------------------------------------------
   class InternalServerSubscriptionMessage_EndReason : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerSubscriptionMessage_EndReason);
      InternalServerSubscriptionMessage_EndReason(
         ServerSubscriptionHandle& h,
         TerminateReason reason,
         std::auto_ptr<Contents> document) : mServerSubscriptionHandle(h), mTerminateReason(reason), mDocument(document) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerSubscriptionMessage_EndReason"; }

   private: // methods.
      virtual void execute()
      {
         if (mServerSubscriptionHandle.isValid())
         {
            mServerSubscriptionHandle->end(mTerminateReason, mDocument.get());
         }
      }
   private: // members.
      ServerSubscriptionHandle  mServerSubscriptionHandle;
      TerminateReason           mTerminateReason;
      std::auto_ptr<Contents>   mDocument;
   };
   // ---------------------------------------------------------------------------
} // resip

#endif // RESIP_InternalServerSubscriptionMessage_hxx

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
