// ==========================================================================================================
// InternalClientRegistrationMessage.hxx                                                   2006 @ TelTel
// ==========================================================================================================
// Hanldes public methods asynchronizely in ClientRegistration.
// ==========================================================================================================
#ifndef RESIP_InternalClientRegistrationMessage_hxx
#define RESIP_InternalClientRegistrationMessage_hxx

#include "resiprocate/dum/InternalDumAsyncMessageBase.hxx"
#include "resiprocate/dum/ClientRegistration.hxx"

namespace resip
{
   // ---------------------------------------------------------------------------
   class InternalClientRegistrationMessage_AddBinding : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientRegistrationMessage_AddBinding);
      InternalClientRegistrationMessage_AddBinding(ClientRegistrationHandle& h, const NameAddr& contact)
         : mClientRegistrationHandle(h), mContact(contact) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientRegistrationMessage_AddBinding"; }

   private: // methods.
      virtual void execute()
      {
         if (mClientRegistrationHandle.isValid())
         {
            mClientRegistrationHandle->addBinding(mContact);
         }
      }
   private: // members.
      ClientRegistrationHandle mClientRegistrationHandle;
      NameAddr                 mContact;
   };
   // ---------------------------------------------------------------------------
   class InternalClientRegistrationMessage_AddBindingWithExpire : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientRegistrationMessage_AddBindingWithExpire);
      InternalClientRegistrationMessage_AddBindingWithExpire(ClientRegistrationHandle& h, const NameAddr& contact, int expireTime)
         : mClientRegistrationHandle(h), mContact(contact), mExpireTime(expireTime) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientRegistrationMessage_AddBindingWithExpire"; }

   private: // methods.
      virtual void execute()
      {
         if (mClientRegistrationHandle.isValid())
         {
            mClientRegistrationHandle->addBinding(mContact, mExpireTime);
         }
      }
   private: // members.
      ClientRegistrationHandle mClientRegistrationHandle;
      NameAddr                 mContact;
      int                      mExpireTime;
   };
   // ---------------------------------------------------------------------------
   class InternalClientRegistrationMessage_RemoveBinding : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientRegistrationMessage_RemoveBinding);
      InternalClientRegistrationMessage_RemoveBinding(ClientRegistrationHandle& h, const NameAddr& contact)
         : mClientRegistrationHandle(h), mContact(contact) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientRegistrationMessage_RemoveBinding"; }

   private: // methods.
      virtual void execute()
      {
         if (mClientRegistrationHandle.isValid())
         {
            mClientRegistrationHandle->removeBinding(mContact);
         }
      }
   private: // members.
      ClientRegistrationHandle mClientRegistrationHandle;
      NameAddr                 mContact;
   };
   // ---------------------------------------------------------------------------
   class InternalClientRegistrationMessage_RemoveAll : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientRegistrationMessage_RemoveAll);
      InternalClientRegistrationMessage_RemoveAll(ClientRegistrationHandle& h, bool stopRegisteringWhenDone=false)
         : mClientRegistrationHandle(h), mStopRegisteringWhenDone(stopRegisteringWhenDone) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientRegistrationMessage_RemoveAll"; }

   private: // methods.
      virtual void execute()
      {
         if (mClientRegistrationHandle.isValid())
         {
            mClientRegistrationHandle->removeAll(mStopRegisteringWhenDone);
         }
      }
   private: // members.
      ClientRegistrationHandle mClientRegistrationHandle;
      bool                     mStopRegisteringWhenDone;
   };
   // ---------------------------------------------------------------------------
   class InternalClientRegistrationMessage_RemoveMyBindings : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientRegistrationMessage_RemoveMyBindings);
      InternalClientRegistrationMessage_RemoveMyBindings(ClientRegistrationHandle& h, bool stopRegisteringWhenDone=false)
         : mClientRegistrationHandle(h), mStopRegisteringWhenDone(stopRegisteringWhenDone) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientRegistrationMessage_RemoveMyBindings"; }

   private: // methods.
      virtual void execute()
      {
         if (mClientRegistrationHandle.isValid())
         {
            mClientRegistrationHandle->removeMyBindings(mStopRegisteringWhenDone);
         }
      }
   private: // members.
      ClientRegistrationHandle mClientRegistrationHandle;
      bool                     mStopRegisteringWhenDone;
   };
   // ---------------------------------------------------------------------------
   class InternalClientRegistrationMessage_Refresh : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientRegistrationMessage_Refresh);
      InternalClientRegistrationMessage_Refresh(ClientRegistrationHandle& h, int expires = -1)
         : mClientRegistrationHandle(h), mExpires(expires) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientRegistrationMessage_Refresh"; }

   private: // methods.
      virtual void execute()
      {
         if (mClientRegistrationHandle.isValid())
         {
            mClientRegistrationHandle->requestRefresh(mExpires);
         }
      }
   private: // members.
      ClientRegistrationHandle mClientRegistrationHandle;
      int                      mExpires;
   };
   // ---------------------------------------------------------------------------
   class InternalClientRegistrationMessage_StopRegistering : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientRegistrationMessage_StopRegistering);
      InternalClientRegistrationMessage_StopRegistering(ClientRegistrationHandle& h)
         : mClientRegistrationHandle(h) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientRegistrationMessage_StopRegistering"; }

   private: // methods.
      virtual void execute()
      {
         if (mClientRegistrationHandle.isValid())
         {
            mClientRegistrationHandle->stopRegistering();
         }
      }
   private: // members.
      ClientRegistrationHandle mClientRegistrationHandle;
   };
   // ---------------------------------------------------------------------------
   class InternalClientRegistrationMessage_End : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientRegistrationMessage_End);
      InternalClientRegistrationMessage_End(ClientRegistrationHandle& h) : mClientRegistrationHandle(h) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientRegistrationMessage_End"; }

   private: // methods.
      virtual void execute()
      {
         if (mClientRegistrationHandle.isValid())
         {
            mClientRegistrationHandle->end();
         }
      }
   private: // members.
      ClientRegistrationHandle  mClientRegistrationHandle;
   };
   // ---------------------------------------------------------------------------
   class InternalClientRegistrationMessage_DispatchSipMsg : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientRegistrationMessage_DispatchSipMsg);
      InternalClientRegistrationMessage_DispatchSipMsg(ClientRegistrationHandle& h, const SipMessage& sipMsg)
         : mClientRegistrationHandle(h), mSipMsg(sipMsg) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientRegistrationMessage_DispatchSipMsg"; }

   private: // methods.
      virtual void execute()
      {
         if (mClientRegistrationHandle.isValid())
         {
            mClientRegistrationHandle->dispatch(mSipMsg);
         }
      }
   private: // members.
      ClientRegistrationHandle mClientRegistrationHandle;
      SipMessage              mSipMsg;
   };
   // ---------------------------------------------------------------------------
   class InternalClientRegistrationMessage_DispatchTimeoutMsg : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientRegistrationMessage_DispatchTimeoutMsg);
      InternalClientRegistrationMessage_DispatchTimeoutMsg(ClientRegistrationHandle& h, const DumTimeout& timeoutMsg)
         : mClientRegistrationHandle(h), mTimeoutMsg(timeoutMsg) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientRegistrationMessage_DispatchTimeoutMsg"; }

   private: // methods.
      virtual void execute()
      {
         if (mClientRegistrationHandle.isValid())
         {
            mClientRegistrationHandle->dispatch(mTimeoutMsg);
         }
      }
   private: // members.
      ClientRegistrationHandle mClientRegistrationHandle;
      DumTimeout              mTimeoutMsg;
   };
   // ---------------------------------------------------------------------------
} // resip

#endif // RESIP_InternalClientRegistrationMessage_hxx

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
