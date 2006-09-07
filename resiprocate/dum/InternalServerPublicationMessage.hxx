// ==========================================================================================================
// InternalServerPublicationMessage.hxx                                                   2006 @ TelTel
// ==========================================================================================================
// Hanldes public methods asynchronizely in ServerOutOfDialog.
// ==========================================================================================================
#ifndef RESIP_InternalServerPublicationMessage_hxx
#define RESIP_InternalServerPublicationMessage_hxx

#include "resiprocate/dum/InternalDumAsyncMessageBase.hxx"
#include "resiprocate/dum/ServerPublication.hxx"

namespace resip
{
   // ---------------------------------------------------------------------------
   class InternalServerPublicationMessage_End : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerPublicationMessage_End);
      InternalServerPublicationMessage_End(ServerPublicationHandle& h) : mServerPublicationHandle(h) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerPublicationMessage_End"; }

   private: // methods.
      virtual void execute()
      {
         if (mServerPublicationHandle.isValid())
         {
            mServerPublicationHandle->end();
         }
      }
   private: // members.
      ServerPublicationHandle  mServerPublicationHandle;
   };
   // ---------------------------------------------------------------------------
   class InternalServerPublicationMessage_DispatchSipMsg : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerPublicationMessage_DispatchSipMsg);
      InternalServerPublicationMessage_DispatchSipMsg(ServerPublicationHandle& h, const SipMessage& sipMsg)
         : mServerPublicationHandle(h), mSipMsg(sipMsg) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerPublicationMessage_DispatchSipMsg"; }

   private: // methods.
      virtual void execute()
      {
         if (mServerPublicationHandle.isValid())
         {
            mServerPublicationHandle->dispatch(mSipMsg);
         }
      }
   private: // members.
      ServerPublicationHandle mServerPublicationHandle;
      SipMessage              mSipMsg;
   };
   // ---------------------------------------------------------------------------
   class InternalServerPublicationMessage_DispatchTimeoutMsg : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerPublicationMessage_DispatchTimeoutMsg);
      InternalServerPublicationMessage_DispatchTimeoutMsg(ServerPublicationHandle& h, const DumTimeout& timeoutMsg)
         : mServerPublicationHandle(h), mTimeoutMsg(timeoutMsg) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerPublicationMessage_DispatchTimeoutMsg"; }

   private: // methods.
      virtual void execute()
      {
         if (mServerPublicationHandle.isValid())
         {
            mServerPublicationHandle->dispatch(mTimeoutMsg);
         }
      }
   private: // members.
      ServerPublicationHandle mServerPublicationHandle;
      DumTimeout              mTimeoutMsg;
   };
   // ---------------------------------------------------------------------------
   class InternalServerPublicationMessage_Send : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerPublicationMessage_Send);
      enum AcceptRejectFlag
      {
         ar_NotUsed = 0,
         ar_Accept  = 1,
         ar_Reject  = 2
      };

      InternalServerPublicationMessage_Send(ServerPublicationHandle& h, const SipMessage& msg)
         : mServerPublicationHandle(h), mFlag(ar_NotUsed) {/*Empty*/}
      InternalServerPublicationMessage_Send(ServerPublicationHandle& h, bool accept, int statusCode) 
         : mServerPublicationHandle(h), mFlag(accept ? ar_Accept : ar_Reject), mStatusCode(statusCode) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerPublicationMessage_Send"; }

   private: // methods.
      virtual void execute()
      {
         if (mServerPublicationHandle.isValid())
         {
            switch(mFlag)
            {
            case ar_NotUsed:
               mServerPublicationHandle->send(mMsg);
               break;
            case ar_Accept:
               mServerPublicationHandle->send(mServerPublicationHandle->accept(mStatusCode));
               break;
            case ar_Reject:
               mServerPublicationHandle->send(mServerPublicationHandle->reject(mStatusCode));
               break;
            }
         }
      }
   private: // members.
      ServerPublicationHandle mServerPublicationHandle;
      SipMessage              mMsg;
      AcceptRejectFlag        mFlag;
      int                     mStatusCode;
   };
   // ---------------------------------------------------------------------------
} // resip

#endif // RESIP_InternalServerPublicationMessage_hxx

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
