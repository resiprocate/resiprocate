#include "resip/dum/DialogEventStateManager.hxx"
#include "rutil/Random.hxx"

using namespace resip;

//!dcm! -- we can optimize by time and only update what is necessary in the
//!DialogeventInfo, or by space and only store creation and id in the map.

//?dcm? -- threading/lifetime design: we can have DialogEventInfo point to the
//right information and not hold state and require the user to be in the dum
//thread.  This is consistent w/ the dum api; a copy cons could copy the state
//but would have to be run in the DUM thread; probably unecesssary.

//sounds like the storing approach is easier in the face of working w/ key
//mismatch.

//the comparator/key of the map must have an ordering so that a key can be
//contructed which points to the beinning of a dialogSet.  This could be done by
//no remote tag being always, which might be the existing behaviour, but
//shouldn't be relied on.



// we've received an INVITE
void
DialogEventStateManager::onTryingUas(Dialog& dialog, const SipMessage& invite)
{
   DialogEventInfo* eventInfo = new DialogEventInfo();
   eventInfo->mDialogEventId = Random::getVersion4UuidUrn(); // !jjg! is this right?
   eventInfo->mDialogId = dialog.getId();
   eventInfo->mDirection = DialogEventInfo::Recipient;
   eventInfo->mCreationTime = Timer::getTimeSecs();
   eventInfo->mInviteSession = InviteSessionHandle::NotValid();
   eventInfo->mRemoteSdp = invite.getContents()->clone();
   eventInfo->mLocalIdentity = dialog.getLocalNameAddr();
   eventInfo->mLocalTarget = dialog.getLocalContact().uri();
   eventInfo->mRemoteIdentity = dialog.getRemoteNameAddr();
   eventInfo->mRemoteTarget = new Uri(dialog.getRemoteTarget().uri());
   eventInfo->mRouteSet = dialog.getRouteSet();
   eventInfo->mState = DialogEventInfo::Trying;

   mDialogIdToEventInfo[dialog.getId()] = eventInfo;

   mDialogEventHandler->onTrying(*eventInfo, invite);
}

// we've sent an INVITE
void
DialogEventStateManager::onTryingUac(DialogSet& dialogSet, const SipMessage& invite)
{
   DialogEventInfo* eventInfo = new DialogEventInfo();
   eventInfo->mDialogEventId = Random::getVersion4UuidUrn();
   eventInfo->mDialogId = DialogId(dialogSet.getId(), Data::Empty);
   eventInfo->mDirection = DialogEventInfo::Initiator;
   eventInfo->mCreationTime = Timer::getTimeSecs();
   eventInfo->mInviteSession = InviteSessionHandle::NotValid();
   eventInfo->mLocalIdentity = invite.header(h_From);
   eventInfo->mLocalTarget = invite.header(h_Contacts).front().uri();
   eventInfo->mRemoteIdentity = invite.header(h_To);
   eventInfo->mLocalSdp = invite.getContents()->clone();
   eventInfo->mState = DialogEventInfo::Trying;

   mDialogIdToEventInfo[eventInfo->mDialogId] = eventInfo;

   mDialogEventHandler->onTrying(*eventInfo, invite);
}

// we've received a 1xx response without a remote tag
void
DialogEventStateManager::onProceedingUac(const DialogSet& dialogSet, const SipMessage& response)
{
   // .jjg. if we have INVITE/180 (tag #1)/180 (no tag)
   // then we'll assume that we can just drop the event
   DialogId fakeId(dialogSet.getId(), Data::Empty);
   std::map<DialogId, DialogEventInfo*, DialogIdComparator>::iterator it = mDialogIdToEventInfo.find(fakeId);
   if (it != mDialogIdToEventInfo.end())
   {
      DialogEventInfo* eventInfo = it->second;
      eventInfo->mState = DialogEventInfo::Proceeding;
      mDialogEventHandler->onProceeding(*eventInfo);
   }
}

// we've received a 1xx response WITH a remote tag
void
DialogEventStateManager::onEarlyUac(const Dialog& dialog, InviteSessionHandle is)
{
   DialogEventInfo* eventInfo = findOrCreateDialogInfo(dialog);

   eventInfo->mState = DialogEventInfo::Early;
   eventInfo->mRouteSet = dialog.getRouteSet();
   eventInfo->mInviteSession = is;

   mDialogEventHandler->onEarly(*eventInfo);
}

// we've sent a 1xx response WITH a local tag
void
DialogEventStateManager::onEarlyUas(const Dialog& dialog, InviteSessionHandle is)
{
   DialogEventInfo* eventInfo = findOrCreateDialogInfo(dialog);
   eventInfo->mInviteSession = is;
   eventInfo->mRouteSet = dialog.getRouteSet();
   eventInfo->mState = DialogEventInfo::Early;

   mDialogEventHandler->onEarly(*eventInfo);
}

void
DialogEventStateManager::onConfirmed(const Dialog& dialog, InviteSessionHandle is)
{
   DialogEventInfo* eventInfo = findOrCreateDialogInfo(dialog);

   eventInfo->mInviteSession = is;
   //eventInfo->mRouteSet = dialog.getRouteSet();
   eventInfo->mState = DialogEventInfo::Confirmed;

   // local or remote target might change due to an UPDATE or re-INVITE
   eventInfo->mLocalTarget = dialog.getLocalContact().uri();
   eventInfo->mRemoteTarget = new Uri(dialog.getRemoteTarget().uri());

   mDialogEventHandler->onConfirmed(*eventInfo);
}

void
DialogEventStateManager::onTerminated(const Dialog& dialog, const SipMessage& msg, InviteSessionHandler::TerminatedReason reason)
{
   DialogEventInfo* eventInfo = NULL;

   /**
    * cases:
    *    1) UAC: INVITE/180 (tag #1)/180 (tag #2)/486 (tag #2)
    *    2) UAS: INVITE/100/486 (tag #1)
    *    3) UAS: INVITE/100/180 (tag #1)/486 (tag #1)
    */

   //find dialogSet.  All non-confirmed dialogs are destroyed by this event.
   //Confirmed dialogs are only destroyed by an exact match.

   DialogId fakeId(dialog.getId().getDialogSetId(), Data::Empty);
   std::map<DialogId, DialogEventInfo*, DialogIdComparator>::iterator it = mDialogIdToEventInfo.lower_bound(fakeId);

   while (it != mDialogIdToEventInfo.end() && 
          it->first.getDialogSetId() == dialog.getId().getDialogSetId())
   {
      eventInfo = it->second;
      eventInfo->mState = DialogEventInfo::Terminated;

      if (reason == InviteSessionHandler::Referred)
      {
         eventInfo->mReferredBy = new NameAddr(msg.header(h_ReferredBy));
      }

      if (reason == InviteSessionHandler::Replaced)
      {
         // !jjg! need to check that this is right...
         eventInfo->mReplacesId = new DialogId(msg.header(h_Replaces).value(), 
            msg.header(h_Replaces).param(p_toTag),
            msg.header(h_Replaces).param(p_fromTag));
      }

      mDialogEventHandler->onTerminated(*eventInfo, reason);
      delete it->second;
      mDialogIdToEventInfo.erase(it++);
   }
}

const std::map<DialogId, DialogEventInfo*, DialogEventStateManager::DialogIdComparator>&
DialogEventStateManager::getDialogEventInfos() const
{
   return mDialogIdToEventInfo;
}

DialogEventInfo* 
DialogEventStateManager::findOrCreateDialogInfo(const Dialog& dialog)
{
   DialogEventInfo* eventInfo = NULL;

   /**
    * cases:
    *    1) INVITE/180 (no tag)/183 (tag)
    *    2) INVITE/180 (tag)
    *    3) INVITE/180 (tag #1)/180 (tag #2)
    *
    */

   std::map<DialogId, DialogEventInfo*, DialogIdComparator>::iterator it = mDialogIdToEventInfo.find(dialog.getId());

   if (it != mDialogIdToEventInfo.end())
   {
      return it->second;
   }
   else
   {
      // either we have a dialog set id with an empty remote tag, or we have other dialog(s) with different
      // remote tag(s)
      DialogId fakeId(dialog.getId().getDialogSetId(), Data::Empty);
      std::map<DialogId, DialogEventInfo*, DialogIdComparator>::iterator it = mDialogIdToEventInfo.lower_bound(fakeId);

      if (it->first.getRemoteTag() == Data::Empty)
      {
         // convert this bad boy into a full on Dialog
         eventInfo = it->second;
         mDialogIdToEventInfo.erase(it);
         eventInfo->mDialogId = dialog.getId();
      }
      else
      {
         assert(it->first.getDialogSetId() == dialog.getId().getDialogSetId());

         // clone this fellow member dialog, initializing it with a new id and creation time 
         DialogEventInfo* newForkInfo = new DialogEventInfo(*(it->second));
         newForkInfo->mDialogEventId = Random::getVersion4UuidUrn();
         newForkInfo->mCreationTime = Timer::getTimeSecs();
         newForkInfo->mDialogId = dialog.getId();
         //newForkInfo->mInviteSession = is;
         newForkInfo->mRemoteIdentity = dialog.getRemoteNameAddr();
         newForkInfo->mRemoteTarget = new Uri(dialog.getRemoteTarget().uri());
         newForkInfo->mRouteSet = dialog.getRouteSet();
         eventInfo = newForkInfo;
      }
   }

   mDialogIdToEventInfo[dialog.getId()] = eventInfo;

   return eventInfo;
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
