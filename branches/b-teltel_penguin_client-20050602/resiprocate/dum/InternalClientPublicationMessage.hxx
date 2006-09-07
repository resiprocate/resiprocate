// ==========================================================================================================
// InternalClientPublicationMessage.hxx                                                   2006 @ TelTel
// ==========================================================================================================
// Hanldes public methods asynchronizely in ClientPublication.
// ==========================================================================================================
#ifndef RESIP_InternalClientPublicationMessage_hxx
#define RESIP_InternalClientPublicationMessage_hxx

#include <memory>

#include "resiprocate/dum/InternalDumAsyncMessageBase.hxx"
#include "resiprocate/dum/ClientPublication.hxx"

namespace resip
{
   // ---------------------------------------------------------------------------
   class InternalClientPublicationMessage_Refresh : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientPublicationMessage_Refresh);
      InternalClientPublicationMessage_Refresh(ClientPublicationHandle& h, unsigned int expiration=0) //0 means the last value of Expires will be used.
         : mClientPublicationHandle(h), mExpiration(expiration) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientPublicationMessage_Refresh"; }

   private: // methods.
      virtual void execute()
      {
         if (mClientPublicationHandle.isValid())
         {
            mClientPublicationHandle->refresh(mExpiration);
         }
      }
   private: // members.
      ClientPublicationHandle mClientPublicationHandle;
      unsigned int            mExpiration;
   };
   // ---------------------------------------------------------------------------
   class InternalClientPublicationMessage_Update : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientPublicationMessage_Update);
      InternalClientPublicationMessage_Update(ClientPublicationHandle& h, std::auto_ptr<Contents> contents)
         : mClientPublicationHandle(h), mContents(contents) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientPublicationMessage_Update"; }

   private: // methods.
      virtual void execute()
      {
         if (mClientPublicationHandle.isValid())
         {
            mClientPublicationHandle->update(mContents.get());
         }
      }
   private: // members.
      ClientPublicationHandle mClientPublicationHandle;
      std::auto_ptr<Contents> mContents;
   };
   // ---------------------------------------------------------------------------
   class InternalClientPublicationMessage_End : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientPublicationMessage_End);
      InternalClientPublicationMessage_End(ClientPublicationHandle& h) : mClientPublicationHandle(h) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientPublicationMessage_End"; }

   private: // methods.
      virtual void execute()
      {
         if (mClientPublicationHandle.isValid())
         {
            mClientPublicationHandle->end();
         }
      }
   private: // members.
      ClientPublicationHandle  mClientPublicationHandle;
   };
   // ---------------------------------------------------------------------------
   class InternalClientPublicationMessage_SipMsg : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientPublicationMessage_SipMsg);
      InternalClientPublicationMessage_SipMsg(ClientPublicationHandle& h, const SipMessage& sipMsg)
         : mClientPublicationHandle(h), mSipMsg(sipMsg) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientPublicationMessage_SipMsg"; }

   private: // methods.
      virtual void execute()
      {
         if (mClientPublicationHandle.isValid())
         {
            mClientPublicationHandle->dispatch(mSipMsg);
         }
      }
   private: // members.
      ClientPublicationHandle mClientPublicationHandle;
      SipMessage              mSipMsg;
   };
   // ---------------------------------------------------------------------------
   class InternalClientPublicationMessage_TimeoutMsg : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientPublicationMessage_TimeoutMsg);
      InternalClientPublicationMessage_TimeoutMsg(ClientPublicationHandle& h, const DumTimeout& timeoutMsg)
         : mClientPublicationHandle(h), mTimeoutMsg(timeoutMsg) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientPublicationMessage_TimeoutMsg"; }

   private: // methods.
      virtual void execute()
      {
         if (mClientPublicationHandle.isValid())
         {
            mClientPublicationHandle->dispatch(mTimeoutMsg);
         }
      }
   private: // members.
      ClientPublicationHandle mClientPublicationHandle;
      DumTimeout              mTimeoutMsg;
   };
   // ---------------------------------------------------------------------------
} // resip

#endif // RESIP_InternalClientPublicationMessage_hxx

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
