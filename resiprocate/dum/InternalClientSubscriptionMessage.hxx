// ==========================================================================================================
// InternalClientSubscriptionMessage.hxx                                                   2006 @ TelTel
// ==========================================================================================================
// Hanldes public methods asynchronizely in ClientSubscription.
// ==========================================================================================================
#ifndef RESIP_InternalClientSubscriptionMessage_hxx
#define RESIP_InternalClientSubscriptionMessage_hxx

#include "resiprocate/dum/InternalDumAsyncMessageBase.hxx"
#include "resiprocate/dum/ClientSubscription.hxx"

namespace resip
{
   // ---------------------------------------------------------------------------
   class InternalClientSubscriptionMessage_AcceptUpdate : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientSubscriptionMessage_AcceptUpdate);
      InternalClientSubscriptionMessage_AcceptUpdate(ClientSubscriptionHandle& h, int statusCode = 200)
         : mClientSubscriptionHandle(h), mStatusCode(statusCode) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientSubscriptionMessage_AcceptUpdate"; }

   private: // methods.
      virtual void execute()
      {
         if (mClientSubscriptionHandle.isValid())
         {
            mClientSubscriptionHandle->acceptUpdate(mStatusCode);
         }
      }
   private: // members.
      ClientSubscriptionHandle mClientSubscriptionHandle;
      int                      mStatusCode;
   };
   // ---------------------------------------------------------------------------
   class InternalClientSubscriptionMessage_RejectUpdate : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientSubscriptionMessage_RejectUpdate);
      InternalClientSubscriptionMessage_RejectUpdate(ClientSubscriptionHandle& h, int statusCode = 400, const Data& reasonPhrase = Data::Empty)
         : mClientSubscriptionHandle(h), mStatusCode(statusCode), mReason(reasonPhrase) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientSubscriptionMessage_RejectUpdate"; }

   private: // methods.
      virtual void execute()
      {
         if (mClientSubscriptionHandle.isValid())
         {
            mClientSubscriptionHandle->rejectUpdate(mStatusCode, mReason);
         }
      }
   private: // members.
      ClientSubscriptionHandle mClientSubscriptionHandle;
      int                      mStatusCode;
      Data                     mReason;
   };
   // ---------------------------------------------------------------------------
   class InternalClientSubscriptionMessage_Refresh : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientSubscriptionMessage_Refresh);
      InternalClientSubscriptionMessage_Refresh(ClientSubscriptionHandle& h, int expires = -1)
         : mClientSubscriptionHandle(h), mExpires(expires) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientSubscriptionMessage_Refresh"; }

   private: // methods.
      virtual void execute()
      {
         if (mClientSubscriptionHandle.isValid())
         {
            mClientSubscriptionHandle->requestRefresh(mExpires);
         }
      }
   private: // members.
      ClientSubscriptionHandle mClientSubscriptionHandle;
      int                      mExpires;
   };
   // ---------------------------------------------------------------------------
   class InternalClientSubscriptionMessage_End : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientSubscriptionMessage_End);
      InternalClientSubscriptionMessage_End(ClientSubscriptionHandle& h) : mClientSubscriptionHandle(h) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientSubscriptionMessage_End"; }

   private: // methods.
      virtual void execute()
      {
         if (mClientSubscriptionHandle.isValid())
         {
            mClientSubscriptionHandle->end();
         }
      }
   private: // members.
      ClientSubscriptionHandle  mClientSubscriptionHandle;
   };
   // ---------------------------------------------------------------------------
} // resip

#endif // RESIP_InternalClientSubscriptionMessage_hxx

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
