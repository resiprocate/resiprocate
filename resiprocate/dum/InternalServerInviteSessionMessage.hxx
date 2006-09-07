// ==========================================================================================================
// InternalServerInviteSessionMessage.hxx                                                2006 @ TelTel
// ==========================================================================================================
// Hanldes public methods asynchronizely in ServerInviteSession.
// ==========================================================================================================
#ifndef RESIP_InternalServerInviteSessionMessage_hxx
#define RESIP_InternalServerInviteSessionMessage_hxx

#include <memory>

#include "resiprocate/SdpContents.hxx"
#include "resiprocate/WarningCategory.hxx"
#include "resiprocate/dum/InternalDumAsyncMessageBase.hxx"
#include "resiprocate/dum/ServerInviteSession.hxx"

namespace resip
{
   class InternalServerInviteSessionMessage_Redirect : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerInviteSessionMessage_Redirect);
      InternalServerInviteSessionMessage_Redirect(ServerInviteSessionHandle& h, const NameAddrs& contacts, int code=302)
         : mInviteSessionHandle(h), mContacts(contacts), mCode(code) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerInviteSessionMessage_Redirect"; }

   private: // methods.
      virtual void execute()
      {
         if (mInviteSessionHandle.isValid())
         {
            mInviteSessionHandle->redirect(mContacts, mCode);
         }
      }
   private: // members.
      ServerInviteSessionHandle  mInviteSessionHandle;
      NameAddrs                  mContacts;
      int                        mCode;
   };
   // ---------------------------------------------------------------------------
   class InternalServerInviteSessionMessage_Provisional : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerInviteSessionMessage_Provisional);
      InternalServerInviteSessionMessage_Provisional(ServerInviteSessionHandle& h, int code=180)
         : mInviteSessionHandle(h), mCode(code) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerInviteSessionMessage_Provisional"; }

   private: // methods.
      virtual void execute()
      {
         if (mInviteSessionHandle.isValid())
         {
            mInviteSessionHandle->provisional(mCode);
         }
      }
   private: // members.
      ServerInviteSessionHandle  mInviteSessionHandle;
      int                        mCode;
   };
   // ---------------------------------------------------------------------------
   class InternalServerInviteSessionMessage_ProvideOffer : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerInviteSessionMessage_ProvideOffer);
      InternalServerInviteSessionMessage_ProvideOffer(ServerInviteSessionHandle& h, std::auto_ptr<SdpContents> offerSdp) 
         : mInviteSessionHandle(h), mOfferSdp(offerSdp) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerInviteSessionMessage_ProvideOffer"; }

   private: // methods.
      virtual void execute()
      {
         if (mInviteSessionHandle.isValid())
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
      ServerInviteSessionHandle  mInviteSessionHandle;   // should be connected.
      std::auto_ptr<SdpContents> mOfferSdp;
   };
   // ---------------------------------------------------------------------------
   class InternalServerInviteSessionMessage_ProvideAnswer : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerInviteSessionMessage_ProvideAnswer);
      InternalServerInviteSessionMessage_ProvideAnswer(ServerInviteSessionHandle& h, std::auto_ptr<SdpContents> answerSdp) 
         : mInviteSessionHandle(h), mAnswerSdp(answerSdp) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerInviteSessionMessage_ProvideAnswer"; }

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
      ServerInviteSessionHandle  mInviteSessionHandle;
      std::auto_ptr<SdpContents> mAnswerSdp;
   };
   // ---------------------------------------------------------------------------
   class InternalServerInviteSessionMessage_End : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerInviteSessionMessage_End);
      InternalServerInviteSessionMessage_End(ServerInviteSessionHandle& h) : mInviteSessionHandle(h) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerInviteSessionMessage_End"; }

   private: // methods.
      virtual void execute()
      {
         if (mInviteSessionHandle.isValid())
         {
            mInviteSessionHandle->end();
         }
      }
   private: // members.
      ServerInviteSessionHandle  mInviteSessionHandle;
   };
   // ---------------------------------------------------------------------------
   class InternalServerInviteSessionMessage_Reject : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerInviteSessionMessage_Reject);
      InternalServerInviteSessionMessage_Reject(ServerInviteSessionHandle& h, int statusCode, WarningCategory* warning=0)
         : mInviteSessionHandle(h), mStatusCode(statusCode)
      {
         if (warning)
         {
            mWarning.reset(new WarningCategory(*warning));
         }
      }

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerInviteSessionMessage_Reject"; }

   private: // methods.
      virtual void execute()
      {
         if (mInviteSessionHandle.isValid())
         {
            mInviteSessionHandle->reject(mStatusCode, mWarning.get());
         }
      }
   private: // members.
      ServerInviteSessionHandle        mInviteSessionHandle;
      int                              mStatusCode;
      std::auto_ptr<WarningCategory>   mWarning;
   };
   // ---------------------------------------------------------------------------
   class InternalServerInviteSessionMessage_Accept : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalServerInviteSessionMessage_Accept);
      InternalServerInviteSessionMessage_Accept(ServerInviteSessionHandle& h, int statusCode=200)
         : mInviteSessionHandle(h), mStatusCode(statusCode) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalServerInviteSessionMessage_Accept"; }

   private: // methods.
      virtual void execute()
      {
         if (mInviteSessionHandle.isValid())
         {
            mInviteSessionHandle->accept(mStatusCode);
         }
      }
   private: // members.
      ServerInviteSessionHandle  mInviteSessionHandle;
      int                        mStatusCode;
   };
   // ---------------------------------------------------------------------------
} // resip

#endif // RESIP_InternalServerInviteSessionMessage_hxx

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
