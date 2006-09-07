// ==========================================================================================================
// InternalServerPagerMessage.hxx                                                   2006 @ TelTel
// ==========================================================================================================
// Hanldes public methods asynchronizely in ServerOutOfDialog.
// ==========================================================================================================
#ifndef RESIP_InternalServerPagerMessage_hxx
#define RESIP_InternalServerPagerMessage_hxx

#include <memory>

#include "resiprocate/dum/InternalDumAsyncMessageBase.hxx"
#include "resiprocate/dum/ServerPagerMessage.hxx"

namespace resip
{
   // ---------------------------------------------------------------------------
   class InternalServerPagerMessage_End : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerPagerMessage_End);
      InternalServerPagerMessage_End(ServerPagerMessageHandle& h) : mServerPagerMessageHandle(h) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerPagerMessage_End"; }

   private: // methods.
      virtual void execute()
      {
         if (mServerPagerMessageHandle.isValid())
         {
            mServerPagerMessageHandle->end();
         }
      }
   private: // members.
      ServerPagerMessageHandle   mServerPagerMessageHandle;
   };
   // ---------------------------------------------------------------------------
   class InternalServerPagerMessage_Send : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerPagerMessage_Send);
      enum AcceptRejectFlag
      {
         ar_NotUsed = 0,
         ar_Accept  = 1,
         ar_Reject  = 2
      };

      InternalServerPagerMessage_Send(ServerPagerMessageHandle& h, const SipMessage& msg)
         : mServerPagerMessageHandle(h), mFlag(ar_NotUsed) {/*Empty*/}
      InternalServerPagerMessage_Send(ServerPagerMessageHandle& h, bool accept, int statusCode) 
         : mServerPagerMessageHandle(h), mFlag(accept ? ar_Accept : ar_Reject), mStatusCode(statusCode) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerPagerMessage_Send"; }

   private: // methods.
      virtual void execute()
      {
         if (mServerPagerMessageHandle.isValid())
         {
            switch(mFlag)
            {
            case ar_NotUsed:
               mServerPagerMessageHandle->send(mMsg);
               break;
            case ar_Accept:
               mServerPagerMessageHandle->send(mServerPagerMessageHandle->accept(mStatusCode));
               break;
            case ar_Reject:
               mServerPagerMessageHandle->send(mServerPagerMessageHandle->reject(mStatusCode));
               break;
            }
         }
      }
   private: // members.
      ServerPagerMessageHandle  mServerPagerMessageHandle;
      SipMessage                mMsg;
      AcceptRejectFlag          mFlag;
      int                       mStatusCode;
   };
   // ---------------------------------------------------------------------------
   class InternalServerPagerMessage_DispatchSipMsg : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerPagerMessage_DispatchSipMsg);
      InternalServerPagerMessage_DispatchSipMsg(ServerPagerMessageHandle& h, const SipMessage& sipMsg)
         : mServerPagerMessageHandle(h), mSipMsg(sipMsg) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerPagerMessage_DispatchSipMsg"; }

   private: // methods.
      virtual void execute()
      {
         if (mServerPagerMessageHandle.isValid())
         {
            mServerPagerMessageHandle->dispatch(mSipMsg);
         }
      }
   private: // members.
      ServerPagerMessageHandle   mServerPagerMessageHandle;
      SipMessage                 mSipMsg;
   };
   // ---------------------------------------------------------------------------
   class InternalServerPagerMessage_DispatchTimeoutMsg : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerPagerMessage_DispatchTimeoutMsg);
      InternalServerPagerMessage_DispatchTimeoutMsg(ServerPagerMessageHandle& h, const DumTimeout& timeoutMsg)
         : mServerPagerMessageHandle(h), mTimeoutMsg(timeoutMsg) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerPagerMessage_DispatchTimeoutMsg"; }

   private: // methods.
      virtual void execute()
      {
         if (mServerPagerMessageHandle.isValid())
         {
            mServerPagerMessageHandle->dispatch(mTimeoutMsg);
         }
      }
   private: // members.
      ServerPagerMessageHandle   mServerPagerMessageHandle;
      DumTimeout                 mTimeoutMsg;
   };
   // ---------------------------------------------------------------------------
} // resip

#endif // RESIP_InternalServerPagerMessage_hxx

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
