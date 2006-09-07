// ==========================================================================================================
// InternalServerRegistrationMessage.hxx                                                   2006 @ TelTel
// ==========================================================================================================
// Hanldes public methods asynchronizely in ServerRegistration.
// ==========================================================================================================
#ifndef RESIP_InternalServerRegistrationMessage_hxx
#define RESIP_InternalServerRegistrationMessage_hxx

#include "resiprocate/dum/InternalDumAsyncMessageBase.hxx"
#include "resiprocate/dum/ServerRegistration.hxx"

namespace resip
{
   // ---------------------------------------------------------------------------
   class InternalServerRegistrationMessage_Accept : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerRegistrationMessage_Accept);
      InternalServerRegistrationMessage_Accept(ServerRegistrationHandle& h, const SipMessage& acceptMsg)
         : mServerRegistrationHandle(h), mAcceptMsg(acceptMsg), mStatusCode(-1) {/*Empty*/}
      InternalServerRegistrationMessage_Accept(ServerRegistrationHandle& h, int statusCode = 200)
         : mServerRegistrationHandle(h), mStatusCode(statusCode) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerRegistrationMessage_Accept"; }

   private: // methods.
      virtual void execute()
      {
         if (mServerRegistrationHandle.isValid())
         {
            if(mStatusCode != -1)
            {
               mServerRegistrationHandle->accept(mStatusCode);
            }
            else
            {
               mServerRegistrationHandle->accept(mAcceptMsg);
            }
         }
      }
   private: // members.
      ServerRegistrationHandle  mServerRegistrationHandle;
      SipMessage                mAcceptMsg;
      int                       mStatusCode;
   };
   // ---------------------------------------------------------------------------
   class InternalServerRegistrationMessage_Reject : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerRegistrationMessage_Reject);
      InternalServerRegistrationMessage_Reject(ServerRegistrationHandle& h, int statusCode) 
         : mServerRegistrationHandle(h), mStatusCode(statusCode) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerRegistrationMessage_Reject"; }

   private: // methods.
      virtual void execute()
      {
         if (mServerRegistrationHandle.isValid())
         {
            mServerRegistrationHandle->reject(mStatusCode);
         }
      }
   private: // members.
      ServerRegistrationHandle  mServerRegistrationHandle;
      int                       mStatusCode;
   };
   // ---------------------------------------------------------------------------
   class InternalServerRegistrationMessage_End : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerRegistrationMessage_End);
      InternalServerRegistrationMessage_End(ServerRegistrationHandle& h) : mServerRegistrationHandle(h) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerRegistrationMessage_End"; }

   private: // methods.
      virtual void execute()
      {
         if (mServerRegistrationHandle.isValid())
         {
            mServerRegistrationHandle->end();
         }
      }
   private: // members.
      ServerRegistrationHandle  mServerRegistrationHandle;
   };
   // ---------------------------------------------------------------------------
   class InternalServerRegistrationMessage_DispatchSipMsg : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerRegistrationMessage_DispatchSipMsg);
      InternalServerRegistrationMessage_DispatchSipMsg(ServerRegistrationHandle& h, const SipMessage& sipMsg)
         : mServerRegistrationHandle(h), mSipMsg(sipMsg) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerRegistrationMessage_DispatchSipMsg"; }

   private: // methods.
      virtual void execute()
      {
         if (mServerRegistrationHandle.isValid())
         {
            mServerRegistrationHandle->dispatch(mSipMsg);
         }
      }
   private: // members.
      ServerRegistrationHandle mServerRegistrationHandle;
      SipMessage              mSipMsg;
   };
   // ---------------------------------------------------------------------------
   class InternalServerRegistrationMessage_DispatchTimeoutMsg : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerRegistrationMessage_DispatchTimeoutMsg);
      InternalServerRegistrationMessage_DispatchTimeoutMsg(ServerRegistrationHandle& h, const DumTimeout& timeoutMsg)
         : mServerRegistrationHandle(h), mTimeoutMsg(timeoutMsg) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerRegistrationMessage_DispatchTimeoutMsg"; }

   private: // methods.
      virtual void execute()
      {
         if (mServerRegistrationHandle.isValid())
         {
            mServerRegistrationHandle->dispatch(mTimeoutMsg);
         }
      }
   private: // members.
      ServerRegistrationHandle mServerRegistrationHandle;
      DumTimeout              mTimeoutMsg;
   };
   // ---------------------------------------------------------------------------
} // resip

#endif // RESIP_InternalServerRegistrationMessage_hxx

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
