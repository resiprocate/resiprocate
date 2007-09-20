#include "resip/dum/DialogEventInfo.hxx"
#include "resip/dum/InviteSession.hxx"

using namespace resip;

DialogEventInfo::DialogEventInfo()
: mDialogId(Data::Empty, Data::Empty, Data::Empty),
  mState(DialogEventInfo::Trying),
  mDirection(DialogEventInfo::Initiator),
  mCreationTime(0),
  mInviteSession(InviteSessionHandle::NotValid())
{
}

DialogEventInfo::DialogEventInfo(const DialogEventInfo& rhs)
: mDialogId(rhs.mDialogId),
  mState(rhs.mState),
  mDialogEventId(rhs.mDialogEventId),
  mDirection(rhs.mDirection),
  mReplacesId(new DialogId(rhs.mReplacesId->getCallId(), rhs.mReplacesId->getLocalTag(), rhs.mReplacesId->getRemoteTag())),
  mInviteSession(rhs.mInviteSession),
  mReferredBy(new NameAddr(*rhs.mReferredBy)),
  mRouteSet(rhs.mRouteSet),
  mLocalIdentity(rhs.mLocalIdentity),
  mRemoteIdentity(rhs.mRemoteIdentity),
  mLocalTarget(rhs.mLocalTarget),
  mRemoteTarget(new Uri(*rhs.mRemoteTarget)),
  mCreationTime(rhs.mCreationTime)
{
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
DialogEventInfo::getState()
{
   return mState;
}

const Data& 
DialogEventInfo::getDialogEventId()
{
   return mDialogEventId;
}

const Data& 
DialogEventInfo::getCallId()
{
   return mDialogId.getCallId();
}

const Data& 
DialogEventInfo::getLocalTag()
{
   return mDialogId.getLocalTag();
}

bool 
DialogEventInfo::hasRemoteTag()
{
   return (mDialogId.getRemoteTag() != Data::Empty);
}

const Data& 
DialogEventInfo::getRemoteTag()
{
   return mDialogId.getRemoteTag();
}

bool 
DialogEventInfo::hasRefferedBy() const
{
   return (mReferredBy.get() != NULL);
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
   return (mRemoteTarget.get() != NULL);
}

const Uri&
DialogEventInfo::getRemoteTarget() const
{
   return *mRemoteTarget;
}

const SdpContents&
DialogEventInfo::getLocalSdp() const
{
   if (mInviteSession.isValid())
   {
      if (mInviteSession->hasLocalSdp())
      {
         return mInviteSession->getLocalSdp();
      }
   }
   assert(mLocalSdp.get() != NULL);
   return *mLocalSdp;
}

const SdpContents&
DialogEventInfo::getRemoteSdp() const
{
   if (mInviteSession.isValid())
   {
      if (mInviteSession->hasRemoteSdp())
      {
         return mInviteSession->getRemoteSdp();
      }
   }
   assert(mRemoteSdp.get() != NULL);
   return *mRemoteSdp;
}

UInt64
DialogEventInfo::getDuration() const
{
   UInt64 delta = Timer::getTimeSecs() - mCreationTime;
   return delta;
}


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
