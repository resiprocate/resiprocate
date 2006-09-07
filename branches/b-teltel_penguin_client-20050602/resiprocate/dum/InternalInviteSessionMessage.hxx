// ==========================================================================================================
// InternalInviteSessionMessage.hxx                                                      2006 @ TelTel
// ==========================================================================================================
// Hanldes public methods asynchronizely in InviteSession.
// ==========================================================================================================
#ifndef RESIP_InternalInviteSessionMessage_hxx
#define RESIP_InternalInviteSessionMessage_hxx

#include <memory>

#include "resiprocate/SdpContents.hxx"
#include "resiprocate/WarningCategory.hxx"
#include "resiprocate/dum/InternalDumAsyncMessageBase.hxx"
#include "resiprocate/dum/InviteSession.hxx"

namespace resip
{
   class InternalInviteSessionMessage_ProvideOffer : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalInviteSessionMessage_ProvideOffer);
      InternalInviteSessionMessage_ProvideOffer(InviteSessionHandle& h, std::auto_ptr<SdpContents> offerSdp) 
         : mInviteSessionHandle(h), mOfferSdp(offerSdp) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalInviteSessionMessage_ProvideOffer"; }

   private: // methods.
      virtual void execute()
      {
         if (mInviteSessionHandle.isValid())
         {
            assert(mOfferSdp.get());
            if(mOfferSdp.get())
            {
               mInviteSessionHandle->provideOffer(*mOfferSdp);
            }
         }
      }
   private: // members.
      InviteSessionHandle        mInviteSessionHandle;   // should be connected.
      std::auto_ptr<SdpContents> mOfferSdp;
   };
   // ---------------------------------------------------------------------------
   class InternalInviteSessionMessage_ProvideAnswer : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalInviteSessionMessage_ProvideAnswer);
      InternalInviteSessionMessage_ProvideAnswer(InviteSessionHandle& h, std::auto_ptr<SdpContents> answerSdp) 
         : mInviteSessionHandle(h), mAnswerSdp(answerSdp) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalInviteSessionMessage_ProvideAnswer"; }

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
      InviteSessionHandle        mInviteSessionHandle;
      std::auto_ptr<SdpContents> mAnswerSdp;
   };
   // ---------------------------------------------------------------------------
   class InternalInviteSessionMessage_End : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalInviteSessionMessage_End);
      InternalInviteSessionMessage_End(InviteSessionHandle& h) : mInviteSessionHandle(h) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalInviteSessionMessage_End"; }

   private: // methods.
      virtual void execute()
      {
         if (mInviteSessionHandle.isValid())
         {
            mInviteSessionHandle->end();
         }
      }
   private: // members.
      InviteSessionHandle  mInviteSessionHandle;
   };
   // ---------------------------------------------------------------------------
   class InternalInviteSessionMessage_Reject : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalInviteSessionMessage_Reject);
      InternalInviteSessionMessage_Reject(InviteSessionHandle& h, int statusCode, WarningCategory* warning=0)
         : mInviteSessionHandle(h), mStatusCode(statusCode)
      {
         if (warning)
         {
            mWarning.reset(new WarningCategory(*warning));
         }
      }

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalInviteSessionMessage_Reject"; }

   private: // methods.
      virtual void execute()
      {
         if (mInviteSessionHandle.isValid())
         {
            mInviteSessionHandle->reject(mStatusCode, mWarning.get());
         }
      }
   private: // members.
      InviteSessionHandle              mInviteSessionHandle;
      int                              mStatusCode;
      std::auto_ptr<WarningCategory>   mWarning;
   };
   // ---------------------------------------------------------------------------
   class InternalInviteSessionMessage_TargetRefresh : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalInviteSessionMessage_TargetRefresh);
      InternalInviteSessionMessage_TargetRefresh(InviteSessionHandle& h, const NameAddr& localUri)
         : mInviteSessionHandle(h), mLocalUri(localUri) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalInviteSessionMessage_TargetRefresh"; }

   private: // methods.
      virtual void execute()
      {
         if (mInviteSessionHandle.isValid())
         {
            mInviteSessionHandle->targetRefresh(mLocalUri);
         }
      }
   private: // members.
      InviteSessionHandle  mInviteSessionHandle;
      NameAddr             mLocalUri;
   };
   // ---------------------------------------------------------------------------
   class InternalInviteSessionMessage_Refer : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalInviteSessionMessage_Provisional);
      InternalInviteSessionMessage_Refer(InviteSessionHandle& h, const NameAddr& referTo, InviteSessionHandle sessionToReplace)
         : mInviteSessionHandle(h), mReferTo(referTo), mSessionToReplace(sessionToReplace) {/*Empty*/}
      InternalInviteSessionMessage_Refer(InviteSessionHandle& h, const NameAddr& referTo)
         : mInviteSessionHandle(h), mReferTo(referTo) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalInviteSessionMessage_Refer"; }

   private: // methods.
      virtual void execute()
      {
         if (mInviteSessionHandle.isValid())
         {
            if(mSessionToReplace.isValid())
            {
               mInviteSessionHandle->refer(mReferTo, mSessionToReplace);
            }
            else
            {
               mInviteSessionHandle->refer(mReferTo);
            }
         }
      }
   private: // members.
      InviteSessionHandle  mInviteSessionHandle;
      NameAddr             mReferTo;
      InviteSessionHandle  mSessionToReplace;
   };
   // ---------------------------------------------------------------------------
   class InternalInviteSessionMessage_Info : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalInviteSessionMessage_Info);
      InternalInviteSessionMessage_Info(InviteSessionHandle& h, std::auto_ptr<Contents> contents)
         : mInviteSessionHandle(h), mContents(contents) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalInviteSessionMessage_Info"; }

   private: // methods.
      virtual void execute()
      {
         if (mInviteSessionHandle.isValid())
         {
            assert(mContents.get());
            if(mContents.get())
            {
               mInviteSessionHandle->info(*mContents);
            }
         }
      }
   private: // members.
      InviteSessionHandle     mInviteSessionHandle;
      std::auto_ptr<Contents> mContents;
   };
   // ---------------------------------------------------------------------------
   class InternalInviteSessionMessage_AcceptInfo : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalInviteSessionMessage_AcceptInfo);
      InternalInviteSessionMessage_AcceptInfo(InviteSessionHandle& h, int statusCode=200)
         : mInviteSessionHandle(h), mStatusCode(statusCode) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalInviteSessionMessage_AcceptInfo"; }

   private: // methods.
      virtual void execute()
      {
         if (mInviteSessionHandle.isValid())
         {
            mInviteSessionHandle->acceptInfo(mStatusCode);
         }
      }
   private: // members.
      InviteSessionHandle  mInviteSessionHandle;
      int                  mStatusCode;
   };
   // ---------------------------------------------------------------------------
   class InternalInviteSessionMessage_RejectInfo : public InternalDumAsyncMessageBase
   {
   public:
      RESIP_HeapCount(InternalInviteSessionMessage_RejectInfo);
      InternalInviteSessionMessage_RejectInfo(InviteSessionHandle& h, int statusCode=488)
         : mInviteSessionHandle(h), mStatusCode(statusCode) {/*Empty*/}

      virtual std::ostream& encodeBrief(std::ostream& strm) const { return strm << "InternalInviteSessionMessage_RejectInfo"; }

   private: // methods.
      virtual void execute()
      {
         if (mInviteSessionHandle.isValid())
         {
            mInviteSessionHandle->rejectInfo(mStatusCode);
         }
      }
   private: // members.
      InviteSessionHandle  mInviteSessionHandle;
      int                  mStatusCode;
   };
   // ---------------------------------------------------------------------------
} // resip

#endif // RESIP_InternalInviteSessionMessage_hxx

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
