#include "resip/dum/DialogEventInfo.hxx"
#include "resip/dum/InviteSession.hxx"

namespace resip
{

DialogEventInfo::DialogEventInfo()
: mState(DialogEventInfo::Trying),
  mDialogId(Data::Empty, Data::Empty, Data::Empty),
  mDirection(DialogEventInfo::Initiator),
  mInviteSession(InviteSessionHandle::NotValid()),
  mCreationTimeSeconds(0),
  mReplaced(false)
{
}

DialogEventInfo::DialogEventInfo(const DialogEventInfo& rhs)
: mState(rhs.mState),
  mDialogEventId(rhs.mDialogEventId),
  mDialogId(rhs.mDialogId),
  mDirection(rhs.mDirection),
  mInviteSession(rhs.mInviteSession),
  mReferredBy(rhs.mReferredBy ? new NameAddr(*rhs.mReferredBy) : nullptr),
  mRouteSet(rhs.mRouteSet),
  mLocalIdentity(rhs.mLocalIdentity),
  mRemoteIdentity(rhs.mRemoteIdentity),
  mLocalTarget(rhs.mLocalTarget),
  mRemoteTarget(rhs.mRemoteTarget ? new Uri(*rhs.mRemoteTarget) : nullptr),
  mCreationTimeSeconds(rhs.mCreationTimeSeconds),
  mReplaced(rhs.mReplaced)
{
   if (rhs.mReplacesId)
   {
      mReplacesId = std::unique_ptr<DialogId>(new DialogId(rhs.mReplacesId->getCallId(),
                                                         rhs.mReplacesId->getLocalTag(),
                                                         rhs.mReplacesId->getRemoteTag()));
   }
   if (rhs.mLocalOfferAnswer)
   {
      mLocalOfferAnswer = std::unique_ptr<Contents>(rhs.mLocalOfferAnswer->clone());
   }
   if (rhs.mRemoteOfferAnswer)
   {
      mRemoteOfferAnswer = std::unique_ptr<Contents>(rhs.mRemoteOfferAnswer->clone());
   }
}

DialogEventInfo&
DialogEventInfo::operator=(const DialogEventInfo& dialogEventInfo)
{
   if (this != &dialogEventInfo)
   {
      mDialogId = dialogEventInfo.mDialogId;
      mState = dialogEventInfo.mState;
      mCreationTimeSeconds = dialogEventInfo.mCreationTimeSeconds;
      mDialogEventId = dialogEventInfo.mDialogEventId;
      mDirection = dialogEventInfo.mDirection;
      mInviteSession = dialogEventInfo.mInviteSession;
      mLocalIdentity = dialogEventInfo.mLocalIdentity;

      mLocalOfferAnswer.reset();
      mReferredBy.reset();
      mRemoteOfferAnswer.reset();
      mRemoteTarget.reset();
      mReplacesId.reset();

      if(dialogEventInfo.mLocalOfferAnswer)
      {
         mLocalOfferAnswer.reset(dialogEventInfo.mLocalOfferAnswer->clone());
      }

      if(dialogEventInfo.mReferredBy)
      {
         mReferredBy.reset((NameAddr*)(dialogEventInfo.mReferredBy->clone()));
      }

      if(dialogEventInfo.mRemoteOfferAnswer)
      {
         mRemoteOfferAnswer.reset(dialogEventInfo.mRemoteOfferAnswer->clone());
      }

      if(dialogEventInfo.mRemoteTarget)
      {
         mRemoteTarget.reset((Uri*)dialogEventInfo.mRemoteTarget->clone());
      }

      if(dialogEventInfo.mReplacesId)
      {
         mReplacesId.reset(
                  new DialogId(dialogEventInfo.mReplacesId->getDialogSetId(), 
                  dialogEventInfo.mReplacesId->getRemoteTag()));
      }

      mLocalTarget = dialogEventInfo.mLocalTarget;
      mRemoteIdentity = dialogEventInfo.mRemoteIdentity;
      mRouteSet = dialogEventInfo.mRouteSet;
      mReplaced = dialogEventInfo.mReplaced;
   }
   return *this;
}

bool 
DialogEventInfo::operator==(const DialogEventInfo& rhs) const
{
   return (mDialogEventId == rhs.mDialogEventId);
}

bool 
DialogEventInfo::operator!=(const DialogEventInfo& rhs) const
{
   return (mDialogEventId != rhs.mDialogEventId);
}

bool 
DialogEventInfo::operator<(const DialogEventInfo& rhs) const
{
   return (mDialogEventId < rhs.mDialogEventId);
}

const DialogEventInfo::State& 
DialogEventInfo::getState() const
{
   return mState;
}

const Data& 
DialogEventInfo::getDialogEventId() const
{
   return mDialogEventId;
}

const Data& 
DialogEventInfo::getCallId() const
{
   return mDialogId.getCallId();
}

const Data& 
DialogEventInfo::getLocalTag() const
{
   return mDialogId.getLocalTag();
}

bool 
DialogEventInfo::hasRemoteTag() const
{
   return (mDialogId.getRemoteTag() != Data::Empty);
}

const Data& 
DialogEventInfo::getRemoteTag() const
{
   return mDialogId.getRemoteTag();
}

bool 
DialogEventInfo::hasRefferedBy() const
{
   return mReferredBy != nullptr;
}

const NameAddr& 
DialogEventInfo::getRefferredBy() const
{
   return *mReferredBy;
}

const NameAddr& 
DialogEventInfo::getLocalIdentity() const
{
   return mLocalIdentity;
}

const NameAddr& 
DialogEventInfo::getRemoteIdentity() const
{
   return mRemoteIdentity;
}

const Uri& 
DialogEventInfo::getLocalTarget() const
{
   return mLocalTarget;   
}

bool
DialogEventInfo::hasRouteSet() const
{
   return false;
}

const NameAddrs& 
DialogEventInfo::getRouteSet() const
{
   return mRouteSet;
}

bool
DialogEventInfo::hasRemoteTarget() const
{
   return mRemoteTarget != nullptr;
}

const Uri&
DialogEventInfo::getRemoteTarget() const
{
   return *mRemoteTarget;
}

const Contents&
DialogEventInfo::getLocalOfferAnswer() const
{
   if (mInviteSession.isValid())
   {
      if (mInviteSession->hasLocalOfferAnswer())
      {
         return mInviteSession->getLocalOfferAnswer();
      }
   }
   resip_assert(mLocalOfferAnswer != nullptr);
   return *mLocalOfferAnswer;
}

const Contents&
DialogEventInfo::getRemoteOfferAnswer() const
{
   if (mInviteSession.isValid())
   {
      if (mInviteSession->hasRemoteOfferAnswer())
      {
         return mInviteSession->getRemoteOfferAnswer();
      }
   }
   resip_assert(mRemoteOfferAnswer != nullptr);
   return *mRemoteOfferAnswer;
}

bool
DialogEventInfo::hasLocalOfferAnswer() const
{
   return (mInviteSession.isValid() ? mInviteSession->hasLocalOfferAnswer() : mLocalOfferAnswer != nullptr);
}

bool
DialogEventInfo::hasRemoteOfferAnswer() const
{
   return (mInviteSession.isValid() ? mInviteSession->hasRemoteOfferAnswer() : mRemoteOfferAnswer != nullptr);
}

uint64_t
DialogEventInfo::getDurationSeconds() const
{
   uint64_t delta = Timer::getTimeSecs() - mCreationTimeSeconds;
   return delta;
}

bool 
DialogEventInfo::hasReplacesId() const
{
   return mReplacesId != nullptr;
}

const DialogId& 
DialogEventInfo::getReplacesId() const
{
   return *mReplacesId;
}

DialogEventInfo::Direction
DialogEventInfo::getDirection() const
{
   return mDirection;
}

} // namespace resip

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
