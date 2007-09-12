#include "resip/dum/DialogEventStateManager.hxx"
#include "rutil/Random.hxx"

using namespace resip;

// we've received an INVITE
void DialogEventStateManager::onTryingUas(Dialog& dialog, const SipMessage& invite)
{
   DialogEventInfo eventInfo;
   eventInfo.mDialogEventId = Random::getVersion4UuidUrn(); // !jjg! is this right?
   eventInfo.mDialogId = dialog.getId();
   eventInfo.mDirection = DialogEventInfo::Recipient;
   eventInfo.mInviteSession = InviteSessionHandle::NotValid();
   //eventInfo.mLocalIdentity = dialog.mLocalNameAddr;
   //eventInfo.mLocalTarget = dialog.mLocalContact;
   eventInfo.mReferredBy = NameAddr();
   //eventInfo.mRemoteIdentity = dialog.mRemoteNameAddr;
   //eventInfo.mRemoteTarget = dialog.mRemoteTarget;
   //eventInfo.mRouteSet = dialog.mRouteSet;
   eventInfo.mState = DialogEventInfo::Trying;

   mDialogIdToGeneratedId[dialog.getId()] = eventInfo.mDialogEventId;

   mDialogEventHandler->onTrying(eventInfo, invite);
}

// we've sent an INVITE
void DialogEventStateManager::onTryingUac(DialogSet& dialogSet, const SipMessage& invite)
{
   DialogEventInfo eventInfo;
   eventInfo.mDialogEventId = Random::getVersion4UuidUrn();
   eventInfo.mDialogId = DialogId(dialogSet.getId(), Data::Empty);
   eventInfo.mDirection = DialogEventInfo::Initiator;
   eventInfo.mInviteSession = InviteSessionHandle::NotValid();
   eventInfo.mLocalIdentity = invite.header(h_From);
   eventInfo.mLocalTarget = invite.header(h_Contacts).front().uri();
   eventInfo.mReferredBy = NameAddr();
   eventInfo.mRemoteIdentity = invite.header(h_To);
   //eventInfo.mRemoteTarget = (will not exist until we get a response with a Contact...)
   //eventInfo.mRouteSet = dialog.mRouteSet;
   eventInfo.mState = DialogEventInfo::Trying;

   mDialogIdToGeneratedId[eventInfo.mDialogId] = eventInfo.mDialogEventId;

   mDialogEventHandler->onTrying(eventInfo, invite);
}

// we've received a 1xx response without a remote tag
void DialogEventStateManager::onProceedingUac(const DialogSet& dialogSet, const SipMessage& response)
{
   DialogId fakeId(dialogSet.getId(), Data::Empty);
   std::map<DialogId, Data>::iterator it = mDialogIdToGeneratedId.find(fakeId);
   if (it != mDialogIdToGeneratedId.end())
   {
      DialogEventInfo eventInfo;
      eventInfo.mDialogEventId = it->second;
      eventInfo.mDialogId = DialogId(dialogSet.getId(), Data::Empty);
      eventInfo.mDirection = DialogEventInfo::Initiator;
      eventInfo.mInviteSession = InviteSessionHandle::NotValid();
      eventInfo.mLocalIdentity = response.header(h_From);
      //eventInfo.mLocalTarget = !jjg! where to get this?
      eventInfo.mReferredBy = NameAddr();
      eventInfo.mRemoteIdentity = response.header(h_To);
      eventInfo.mRemoteTarget = response.header(h_Contacts).front().uri();
      //eventInfo.mRouteSet = dialog.mRouteSet;
      eventInfo.mState = DialogEventInfo::Proceeding;

      mDialogEventHandler->onProceeding(eventInfo);
   }
}

// we've received a 1xx response WITH a remote tag
void DialogEventStateManager::onEarlyUac(const Dialog& dialog, InviteSessionHandle is)
{
   DialogEventInfo eventInfo;
   eventInfo.mDialogId = dialog.getId();
   eventInfo.mDirection = DialogEventInfo::Initiator;
   eventInfo.mInviteSession = is;
   //eventInfo.mLocalIdentity = dialog.mLocalNameAddr;
   //eventInfo.mLocalTarget = dialog.mLocalContact;
   eventInfo.mReferredBy = NameAddr();
   //eventInfo.mRemoteIdentity = dialog.mRemoteNameAddr;
   //eventInfo.mRemoteTarget = dialog.mRemoteTarget;
   //eventInfo.mRouteSet = dialog.mRouteSet;
   eventInfo.mState = DialogEventInfo::Early;

   DialogId fakeId(dialog.getId().getDialogSetId(), Data::Empty);
   std::map<DialogId, Data>::iterator it = mDialogIdToGeneratedId.find(fakeId);

   if (it != mDialogIdToGeneratedId.end())
   {
      eventInfo.mDialogEventId = it->second;

      if (it->first.getRemoteTag() == Data::Empty)
      {
         // we clear out the original entry if and only if it wasn't a full dialog yet
         mDialogIdToGeneratedId.erase(it);
      }
   }
   else
   {
      eventInfo.mDialogEventId = Random::getVersion4UuidUrn();
   }

   mDialogIdToGeneratedId[eventInfo.mDialogId] = eventInfo.mDialogEventId;
   mDialogEventHandler->onEarly(eventInfo);
}

// we've sent a 1xx response WITH a local tag
void DialogEventStateManager::onEarlyUas(const Dialog& dialog, InviteSessionHandle is)
{
   DialogEventInfo eventInfo;
   eventInfo.mDialogId = dialog.getId();
   eventInfo.mDirection = DialogEventInfo::Initiator;
   eventInfo.mInviteSession = is;
   //eventInfo.mLocalIdentity = dialog.mLocalNameAddr;
   //eventInfo.mLocalTarget = dialog.mLocalContact;
   eventInfo.mReferredBy = NameAddr();
   //eventInfo.mRemoteIdentity = dialog.mRemoteNameAddr;
   //eventInfo.mRemoteTarget = dialog.mRemoteTarget;
   //eventInfo.mRouteSet = dialog.mRouteSet;
   eventInfo.mState = DialogEventInfo::Early;

   std::map<DialogId, Data>::iterator it = mDialogIdToGeneratedId.find(dialog.getId());
   if (it != mDialogIdToGeneratedId.end())
   {
      eventInfo.mDialogEventId = it->second;
      mDialogEventHandler->onEarly(eventInfo);
   }
}

void DialogEventStateManager::onConfirmed(const Dialog& dialog, InviteSessionHandle is)
{
   DialogEventInfo eventInfo;
   eventInfo.mDialogId = dialog.getId();
   eventInfo.mDirection = DialogEventInfo::Initiator;
   eventInfo.mInviteSession = is;
   //eventInfo.mLocalIdentity = dialog.mLocalNameAddr;
   //eventInfo.mLocalTarget = dialog.mLocalContact;
   eventInfo.mReferredBy = NameAddr();
   //eventInfo.mRemoteIdentity = dialog.mRemoteNameAddr;
   //eventInfo.mRemoteTarget = dialog.mRemoteTarget;
   //eventInfo.mRouteSet = dialog.mRouteSet;
   eventInfo.mState = DialogEventInfo::Confirmed;

   std::map<DialogId, Data>::iterator it = mDialogIdToGeneratedId.find(dialog.getId());
   if (it != mDialogIdToGeneratedId.end())
   {
      eventInfo.mDialogEventId = it->second;
   }
   else
   {
      // we got a 200 with a DIFFERENT remote tag...
      eventInfo.mDialogEventId = Random::getVersion4UuidUrn();
      mDialogIdToGeneratedId[eventInfo.mDialogId] = eventInfo.mDialogEventId;

      it = mDialogIdToGeneratedId.find(DialogId(dialog.getId().getDialogSetId(), Data::Empty));
      if (it != mDialogIdToGeneratedId.end())
      {
         if (it->first.getRemoteTag() == Data::Empty)
         {
            // we clear out the original entry if and only if it wasn't a full dialog yet
            mDialogIdToGeneratedId.erase(it);
         }
      }
   }
   mDialogEventHandler->onConfirmed(eventInfo);
}

void DialogEventStateManager::onTerminated(const Dialog& dialog, InviteSessionHandler::TerminatedReason reason)
{
   DialogEventInfo eventInfo;
   eventInfo.mDialogId = dialog.getId();
   eventInfo.mDirection = DialogEventInfo::Initiator;
   //eventInfo.mInviteSession = dialog.getInviteSession(); // !jjg! likely not needed anyways
                                                           // since no relevant SDP at this point
   //eventInfo.mLocalIdentity = dialog.mLocalNameAddr;
   //eventInfo.mLocalTarget = dialog.mLocalContact;
   eventInfo.mReferredBy = NameAddr();
   //eventInfo.mRemoteIdentity = dialog.mRemoteNameAddr;
   //eventInfo.mRemoteTarget = dialog.mRemoteTarget;
   //eventInfo.mRouteSet = dialog.mRouteSet;
   eventInfo.mState = DialogEventInfo::Terminated;

   std::map<DialogId, Data>::iterator it = mDialogIdToGeneratedId.find(dialog.getId());
   if (it != mDialogIdToGeneratedId.end())
   {
      eventInfo.mDialogEventId = it->second;
      mDialogIdToGeneratedId.erase(it);
   }
   else
   {
      // we got a 3xx or 4xx with a DIFFERENT remote tag...
      eventInfo.mDialogEventId = Random::getVersion4UuidUrn();

      it = mDialogIdToGeneratedId.find(DialogId(dialog.getId().getDialogSetId(), Data::Empty));
      if (it != mDialogIdToGeneratedId.end())
      {
         if (it->first.getRemoteTag() == Data::Empty)
         {
            // we clear out the original entry if and only if it wasn't a full dialog yet
            mDialogIdToGeneratedId.erase(it);
         }
      }
   }

   mDialogEventHandler->onTerminated(eventInfo, reason);
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
