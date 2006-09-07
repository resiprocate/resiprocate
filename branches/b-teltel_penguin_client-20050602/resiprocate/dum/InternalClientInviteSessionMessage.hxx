// ==========================================================================================================
// InternalClientInviteSessionMessage.hxx                                                2006 @ TelTel
// ==========================================================================================================
// Hanldes public methods asynchronizely in ClientInviteSession.
// ==========================================================================================================
#ifndef RESIP_InternalClientInviteSessionMessage_hxx
#define RESIP_InternalClientInviteSessionMessage_hxx

#include <memory>

#include "resiprocate/SdpContents.hxx"
#include "resiprocate/WarningCategory.hxx"
#include "resiprocate/dum/InternalDumAsyncMessageBase.hxx"
#include "resiprocate/dum/ClientInviteSession.hxx"

namespace resip
{
   class InternalClientInviteSessionMessage_ProvideOffer : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientInviteSessionMessage_ProvideOffer);
      InternalClientInviteSessionMessage_ProvideOffer(ClientInviteSessionHandle& h, std::auto_ptr<SdpContents> offerSdp) 
         : mInviteSessionHandle(h), mOfferSdp(offerSdp) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientInviteSessionMessage_ProvideOffer"; }

   private: // methods.
      virtual void execute()
      {
         if (mInviteSessionHandle.isValid() /*&& mInviteSessionHandle->isConnected()*/)
         {
            assert(mInviteSessionHandle->isConnected());
            assert(mOfferSdp.get());
            if(mOfferSdp.get())
            {
               mInviteSessionHandle->provideOffer(*mOfferSdp);
            }
         }
      }
   private: // members.
      ClientInviteSessionHandle  mInviteSessionHandle;   // should be connected.
      std::auto_ptr<SdpContents> mOfferSdp;
   };
   // ---------------------------------------------------------------------------
   class InternalClientInviteSessionMessage_ProvideAnswer : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientInviteSessionMessage_ProvideAnswer);
      InternalClientInviteSessionMessage_ProvideAnswer(ClientInviteSessionHandle& h, std::auto_ptr<SdpContents> answerSdp) 
         : mInviteSessionHandle(h), mAnswerSdp(answerSdp) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientInviteSessionMessage_ProvideAnswer"; }

   private: // methods.
      virtual void execute()
      {
         if (mInviteSessionHandle.isValid())
         {
            assert(mAnswerSdp.get());
            if(mAnswerSdp.get())
            {
               mInviteSessionHandle->provideAnswer(*mAnswerSdp);
            }
         }
      }
   private: // members.
      ClientInviteSessionHandle  mInviteSessionHandle;
      std::auto_ptr<SdpContents> mAnswerSdp;
   };
   // ---------------------------------------------------------------------------
   class InternalClientInviteSessionMessage_End : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientInviteSessionMessage_End);
      InternalClientInviteSessionMessage_End(ClientInviteSessionHandle& h) : mInviteSessionHandle(h) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientInviteSessionMessage_End"; }

   private: // methods.
      virtual void execute()
      {
         if (mInviteSessionHandle.isValid())
         {
            mInviteSessionHandle->end();
         }
      }
   private: // members.
      ClientInviteSessionHandle  mInviteSessionHandle;
   };
   // ---------------------------------------------------------------------------
   class InternalClientInviteSessionMessage_Reject : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalClientInviteSessionMessage_Reject);
      InternalClientInviteSessionMessage_Reject(ClientInviteSessionHandle& h, int statusCode, WarningCategory* warning=0)
         : mInviteSessionHandle(h), mStatusCode(statusCode)
      {
         if (warning)
         {
            mWarning.reset(new WarningCategory(*warning));
         }
      }

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalClientInviteSessionMessage_Reject"; }

   private: // methods.
      virtual void execute()
      {
         if (mInviteSessionHandle.isValid())
         {
            mInviteSessionHandle->reject(mStatusCode, mWarning.get());
         }
      }
   private: // members.
      ClientInviteSessionHandle        mInviteSessionHandle;
      int                              mStatusCode;
      std::auto_ptr<WarningCategory>   mWarning;
   };
} // resip

#endif // RESIP_InternalClientInviteSessionMessage_hxx

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
