#if !defined(RESIP_DialogEventStateManager_HXX)
#define RESIP_DialogEventStateManager_HXX

#include "resip/dum/DialogEventInfo.hxx"
#include "resip/dum/DialogEventHandler.hxx"
#include "resip/dum/InviteSessionHandler.hxx"
#include "resip/dum/Dialog.hxx"
#include "resip/dum/DialogSet.hxx"

namespace resip
{

/**
 * Implements the FSM for dialog state as spec'd in RFC 4235.
 * Called from DialogSet, ClientInviteSession, and ServerInviteSession.
 */
class DialogEventStateManager
{
public:
   typedef std::vector<DialogEventInfo> DialogEventInfos;

   DialogEventStateManager(const DialogEventStateManager&) = delete;
   DialogEventStateManager(DialogEventStateManager&&) = delete;
   virtual ~DialogEventStateManager();

   DialogEventStateManager& operator=(const DialogEventStateManager&) = delete;
   DialogEventStateManager& operator=(DialogEventStateManager&&) = delete;

   DialogEventInfos getDialogEventInfo() const;
   DialogEventInfos getDialogEventInfo(const Uri& entityUri, bool bMatchRemoteIdentityOnly = false) const;

private:
      DialogEventStateManager();
      
      void onTryingUas(Dialog& dialog, const SipMessage& invite);
      void onTryingUac(DialogSet& dialogSet, const SipMessage& invite);
      void onProceedingUac(const DialogSet& dialogSet, const SipMessage& response);

   //?dcm? how is direction determined when the onEarly is the first use of
   //this dialog?
      void onEarly(const Dialog& dialog, InviteSessionHandle is);
      
      void onConfirmed(const Dialog& dialog, InviteSessionHandle is);
      void onTerminated(const Dialog& dialog, const SipMessage& msg, InviteSessionHandler::TerminatedReason reason);
      
      void onTerminated(const DialogSet& dialogSet, const SipMessage& msg, InviteSessionHandler::TerminatedReason reason);
      
      
   // order by DialogSet, such that the following ordering occurs
   // DialogSetId          remoteTag
   //     a                  null
   //     a                   1
   //     a                   2
   //     b                   1
   //     b                   2
   class DialogIdComparator
   {
   public:
      bool operator()(const DialogId& x, const DialogId& y) const
      {
         if (x.getDialogSetId() == y.getDialogSetId())
         {
            return (x.getRemoteTag() < y.getRemoteTag());
         }
         return (x.getDialogSetId() < y.getDialogSetId());
      }
   };

   DialogEventInfo* findOrCreateDialogInfo(const Dialog& dialog);

   void onDialogSetTerminatedImpl(const DialogSetId& dialogSetId, const SipMessage& msg, InviteSessionHandler::TerminatedReason reason);
   TerminatedDialogEvent* onDialogTerminatedImpl(DialogEventInfo* eventInfo, InviteSessionHandler::TerminatedReason reason, 
                                                 int responseCode = 0, Uri* remoteTarget = NULL);

   static int getResponseCode(const SipMessage& msg);
   static Uri* getFrontContact(const SipMessage& msg);

private:

   friend class DialogUsageManager;
   friend class ServerInviteSession;
   friend class ClientInviteSession;
   friend class InviteSession;
   friend class DialogSet;

   // .jjg. we'll only have the DialogSetId if we aren't yet in the 'early' state;
   // once we get to early, we'll remove the DialogSetId in favour of the DialogId.
   // The comparator/key of the map must have an ordering so that a key can be
   // constructed which points to the beginning of a dialogSet.  This could be done by
   // no remote tag being always, which might be the existing behaviour, but
   // shouldn't be relied on.
   std::map<DialogId, DialogEventInfo*, DialogIdComparator> mDialogIdToEventInfo;

   DialogEventHandler* mDialogEventHandler;
};

}
#endif


/* ====================================================================
 * The Vovida Software License, Version 1.0
 *
 * Copyright (c) 2026 SIP Spectrum, Inc. https://www.sipspectrum.com
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
 * IN EXCESS OF $1,000 NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * ====================================================================
 */