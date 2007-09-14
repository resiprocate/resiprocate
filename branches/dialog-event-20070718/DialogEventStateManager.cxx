#include "resip/dum/DialogEventStateManager.hxx"
#include "rutil/Random.hxx"

using namespace resip;

// we've received an INVITE
void
DialogEventStateManager::onTryingUas(Dialog& dialog, const SipMessage& invite)
{
   DialogEventInfo eventInfo;
   eventInfo.mDialogEventId = Random::getVersion4UuidUrn(); // !jjg! is this right?
   eventInfo.mDialogId = dialog.getId();
   eventInfo.mDirection = DialogEventInfo::Recipient;
   eventInfo.mCreationTime = Timer::getTimeSecs();
   eventInfo.mInviteSession = InviteSessionHandle::NotValid();
   eventInfo.mRemoteSdp = invite.getContents()->clone();
   eventInfo.mLocalIdentity = dialog.getLocalNameAddr();
   eventInfo.mLocalTarget = dialog.getLocalContact().uri();
   eventInfo.mRemoteIdentity = dialog.getRemoteNameAddr();
   eventInfo.mRemoteTarget = new Uri(dialog.getRemoteTarget().uri());
   eventInfo.mRouteSet = dialog.getRouteSet();
   eventInfo.mState = DialogEventInfo::Trying;

   mDialogIdToEventInfo[dialog.getId()] = eventInfo;

   mDialogEventHandler->onTrying(eventInfo, invite);
}

// we've sent an INVITE
void
DialogEventStateManager::onTryingUac(DialogSet& dialogSet, const SipMessage& invite)
{
   DialogEventInfo eventInfo;
   eventInfo.mDialogEventId = Random::getVersion4UuidUrn();
   eventInfo.mDialogId = DialogId(dialogSet.getId(), Data::Empty);
   eventInfo.mDirection = DialogEventInfo::Initiator;
   eventInfo.mCreationTime = Timer::getTimeSecs();
   eventInfo.mInviteSession = InviteSessionHandle::NotValid();
   eventInfo.mLocalIdentity = invite.header(h_From);
   eventInfo.mLocalTarget = invite.header(h_Contacts).front().uri();
   eventInfo.mRemoteIdentity = invite.header(h_To);
   eventInfo.mLocalSdp = invite.getContents()->clone();
   eventInfo.mState = DialogEventInfo::Trying;

   mDialogIdToEventInfo[eventInfo.mDialogId] = eventInfo;

   mDialogEventHandler->onTrying(eventInfo, invite);
}

// we've received a 1xx response without a remote tag
void
DialogEventStateManager::onProceedingUac(const DialogSet& dialogSet, const SipMessage& response)
{
   DialogId fakeId(dialogSet.getId(), Data::Empty);
   std::map<DialogId, DialogEventInfo>::iterator it = mDialogIdToEventInfo.find(fakeId);
   if (it != mDialogIdToEventInfo.end())
   {
      DialogEventInfo& eventInfo = it->second;
      eventInfo.mState = DialogEventInfo::Proceeding;
      mDialogEventHandler->onProceeding(eventInfo);
   }
}

// we've received a 1xx response WITH a remote tag
void
DialogEventStateManager::onEarlyUac(const Dialog& dialog, InviteSessionHandle is)
{
   DialogEventInfo eventInfo;
   eventInfo.mDialogId = dialog.getId();
   eventInfo.mDirection = DialogEventInfo::Initiator;
   eventInfo.mInviteSession = is;
   eventInfo.mLocalIdentity = dialog.getLocalNameAddr();
   eventInfo.mLocalTarget = dialog.getLocalContact().uri();
   eventInfo.mRemoteIdentity = dialog.getRemoteNameAddr();
   eventInfo.mRemoteTarget = new Uri(dialog.getRemoteTarget().uri());
   eventInfo.mRouteSet = dialog.getRouteSet();
   eventInfo.mState = DialogEventInfo::Early;

   DialogId fakeId(dialog.getId().getDialogSetId(), Data::Empty);
   std::map<DialogId, DialogEventInfo>::iterator it = mDialogIdToEventInfo.find(fakeId);

   if (it != mDialogIdToEventInfo.end())
   {
      eventInfo.mDialogEventId = it->second.mDialogEventId;
      eventInfo.mCreationTime = it->second.mCreationTime;

      if (it->first.getRemoteTag() == Data::Empty)
      {
         // we clear out the original entry if and only if it wasn't a full dialog yet
         mDialogIdToEventInfo.erase(it);
      }
   }
   else
   {
      eventInfo.mDialogEventId = Random::getVersion4UuidUrn();
      eventInfo.mCreationTime = Timer::getTimeSecs();
   }

   mDialogIdToEventInfo[eventInfo.mDialogId] = eventInfo;
   mDialogEventHandler->onEarly(eventInfo);
}

// we've sent a 1xx response WITH a local tag
void
DialogEventStateManager::onEarlyUas(const Dialog& dialog, InviteSessionHandle is)
{
   DialogEventInfo eventInfo;
   eventInfo.mDialogId = dialog.getId();
   eventInfo.mDirection = DialogEventInfo::Recipient;
   eventInfo.mInviteSession = is;
   eventInfo.mLocalIdentity = dialog.getLocalNameAddr();
   eventInfo.mLocalTarget = dialog.getLocalContact().uri();
   eventInfo.mRemoteIdentity = dialog.getRemoteNameAddr();
   eventInfo.mRemoteTarget = new Uri(dialog.getRemoteTarget().uri());
   eventInfo.mRouteSet = dialog.getRouteSet();
   eventInfo.mState = DialogEventInfo::Early;

   std::map<DialogId, DialogEventInfo>::iterator it = mDialogIdToEventInfo.find(dialog.getId());
   if (it != mDialogIdToEventInfo.end())
   {
      eventInfo.mDialogEventId = it->second.mDialogEventId;
      eventInfo.mCreationTime = it->second.mCreationTime;
      it->second = eventInfo;
      mDialogEventHandler->onEarly(eventInfo);
   }
}

void
DialogEventStateManager::onConfirmed(const Dialog& dialog, InviteSessionHandle is)
{
   DialogEventInfo eventInfo;
   eventInfo.mDialogId = dialog.getId();
   eventInfo.mInviteSession = is;
   eventInfo.mLocalIdentity = dialog.getLocalNameAddr();
   eventInfo.mLocalTarget = dialog.getLocalContact().uri();
   eventInfo.mRemoteIdentity = dialog.getRemoteNameAddr();
   eventInfo.mRemoteTarget = new Uri(dialog.getRemoteTarget().uri());
   eventInfo.mRouteSet = dialog.getRouteSet();
   eventInfo.mState = DialogEventInfo::Confirmed;

   std::map<DialogId, DialogEventInfo>::iterator it = mDialogIdToEventInfo.find(dialog.getId());
   if (it != mDialogIdToEventInfo.end())
   {
      eventInfo.mDirection = it->second.mDirection;
      eventInfo.mDialogEventId = it->second.mDialogEventId;
      eventInfo.mCreationTime = it->second.mCreationTime;
      it->second = eventInfo;
   }
   else
   {
      // we got a 200 with a DIFFERENT remote tag...
      eventInfo.mDirection = DialogEventInfo::Initiator;
      eventInfo.mDialogEventId = Random::getVersion4UuidUrn();
      eventInfo.mCreationTime = Timer::getTimeSecs();
      mDialogIdToEventInfo[eventInfo.mDialogId] = eventInfo;

      it = mDialogIdToEventInfo.find(DialogId(dialog.getId().getDialogSetId(), Data::Empty));
      if (it != mDialogIdToEventInfo.end())
      {
         if (it->first.getRemoteTag() == Data::Empty)
         {
            // we clear out the original entry if and only if it wasn't a full dialog yet
            mDialogIdToEventInfo.erase(it);
         }
      }
   }
   mDialogEventHandler->onConfirmed(eventInfo);
}

void
DialogEventStateManager::onTerminated(const Dialog& dialog, const SipMessage& msg, InviteSessionHandler::TerminatedReason reason)
{
   DialogEventInfo eventInfo;
   eventInfo.mDialogId = dialog.getId();
   //eventInfo.mInviteSession = dialog.getInviteSession(); // !jjg! likely not needed anyways
                                                           // since no relevant SDP at this point
   eventInfo.mLocalIdentity = dialog.getLocalNameAddr();
   eventInfo.mLocalTarget = dialog.getLocalContact().uri();
   eventInfo.mRemoteIdentity = dialog.getRemoteNameAddr();
   eventInfo.mRemoteTarget = new Uri(dialog.getRemoteTarget().uri());
   eventInfo.mRouteSet = dialog.getRouteSet();
   eventInfo.mState = DialogEventInfo::Terminated;

   if (reason == InviteSessionHandler::Referred)
   {
      eventInfo.mReferredBy = new NameAddr(msg.header(h_ReferredBy));
   }

   if (reason == InviteSessionHandler::Replaced)
   {
      // !jjg! need to check that this is right...
      eventInfo.mReplacesId = new DialogId(msg.header(h_Replaces).value(), 
         msg.header(h_Replaces).param(p_toTag),
         msg.header(h_Replaces).param(p_fromTag));
   }

   std::map<DialogId, DialogEventInfo>::iterator it = mDialogIdToEventInfo.find(dialog.getId());
   if (it != mDialogIdToEventInfo.end())
   {
      eventInfo.mDirection = it->second.mDirection;
      eventInfo.mDialogEventId = it->second.mDialogEventId;
      eventInfo.mCreationTime = it->second.mCreationTime;
      mDialogIdToEventInfo.erase(it);
   }
   else
   {
      // we got a 3xx or 4xx with a DIFFERENT remote tag...
      eventInfo.mDirection = DialogEventInfo::Initiator;
      eventInfo.mDialogEventId = Random::getVersion4UuidUrn();
      eventInfo.mCreationTime = Timer::getTimeSecs();

      it = mDialogIdToEventInfo.find(DialogId(dialog.getId().getDialogSetId(), Data::Empty));
      if (it != mDialogIdToEventInfo.end())
      {
         if (it->first.getRemoteTag() == Data::Empty)
         {
            // we clear out the original entry if and only if it wasn't a full dialog yet
            mDialogIdToEventInfo.erase(it);
         }
      }
   }

   mDialogEventHandler->onTerminated(eventInfo, reason);
}

const std::map<DialogId, DialogEventInfo>&
DialogEventStateManager::getDialogEventInfos() const
{
   return mDialogIdToEventInfo;
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
