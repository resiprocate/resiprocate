// ==========================================================================================================
// InternalServerOutOfDialogReqMessage.hxx                                                2006 @ TelTel
// ==========================================================================================================
// Hanldes public methods asynchronizely in ServerOutOfDialog.
// ==========================================================================================================
#ifndef RESIP_InternalServerOutOfDialogReqMessage_hxx
#define RESIP_InternalServerOutOfDialogReqMessage_hxx

#include "resiprocate/dum/InternalDumAsyncMessageBase.hxx"
#include "resiprocate/dum/ServerOutOfDialogReq.hxx"

namespace resip
{
   // ---------------------------------------------------------------------------
   class InternalServerOutOfDialogReqMessage_End : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerOutOfDialogReqMessage_End);
      InternalServerOutOfDialogReqMessage_End(ServerOutOfDialogReqHandle& h) : mServerOutOfDialogHandle(h) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerOutOfDialogReqMessage_End"; }

   private: // methods.
      virtual void execute()
      {
         if (mServerOutOfDialogHandle.isValid())
         {
            mServerOutOfDialogHandle->end();
         }
      }
   private: // members.
      ServerOutOfDialogReqHandle  mServerOutOfDialogHandle;
   };
   // ---------------------------------------------------------------------------
   class InternalServerOutOfDialogReqMessage_SipMsg : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerOutOfDialogReqMessage_SipMsg);
      InternalServerOutOfDialogReqMessage_SipMsg(ServerOutOfDialogReqHandle& h, const SipMessage& sipMsg)
         : mServerOutOfDialogHandle(h), mSipMsg(sipMsg) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerOutOfDialogReqMessage_SipMsg"; }

   private: // methods.
      virtual void execute()
      {
         if (mServerOutOfDialogHandle.isValid())
         {
            mServerOutOfDialogHandle->dispatch(mSipMsg);
         }
      }
   private: // members.
      ServerOutOfDialogReqHandle mServerOutOfDialogHandle;
      SipMessage                 mSipMsg;
   };
   // ---------------------------------------------------------------------------
   class InternalServerOutOfDialogReqMessage_TimeoutMsg : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerOutOfDialogReqMessage_TimeoutMsg);
      InternalServerOutOfDialogReqMessage_TimeoutMsg(ServerOutOfDialogReqHandle& h, const DumTimeout& timeoutMsg)
         : mServerOutOfDialogHandle(h), mTimeoutMsg(timeoutMsg) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerOutOfDialogReqMessage_TimeoutMsg"; }

   private: // methods.
      virtual void execute()
      {
         if (mServerOutOfDialogHandle.isValid())
         {
            mServerOutOfDialogHandle->dispatch(mTimeoutMsg);
         }
      }
   private: // members.
      ServerOutOfDialogReqHandle mServerOutOfDialogHandle;
      DumTimeout                 mTimeoutMsg;
   };
   // ---------------------------------------------------------------------------
   class InternalServerOutOfDialogReqMessage_Send : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerOutOfDialogReqMessage_Send);
      InternalServerOutOfDialogReqMessage_Send(ServerOutOfDialogReqHandle& h, const SipMessage& sipMsg)
         : mServerOutOfDialogHandle(h), mSipMsg(sipMsg) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerOutOfDialogReqMessage_Send"; }

   private: // methods.
      virtual void execute()
      {
         if (mServerOutOfDialogHandle.isValid())
         {
            mServerOutOfDialogHandle->send(mSipMsg);
         }
      }
   private: // members.
      ServerOutOfDialogReqHandle mServerOutOfDialogHandle;
      SipMessage                 mSipMsg;
   };
   // ---------------------------------------------------------------------------
} // resip

#endif // RESIP_InternalServerOutOfDialogReqMessage_hxx

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
