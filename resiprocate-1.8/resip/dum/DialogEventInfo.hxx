#if !defined(RESIP_DialogEventInfo_hxx)
#define RESIP_DialogEventInfo_hxx

#include <memory>
#include "resip/stack/NameAddr.hxx"
#include "resip/dum/DialogId.hxx"
#include "resip/dum/Handles.hxx"
#include "resip/stack/Contents.hxx"

namespace resip
{

//As uac, one is created for DialogSet; given to first dialog, new instances are
//generated for each fork.  As uas, created at the same time as a Dialog.
class DialogEventInfo
{
   public:
      DialogEventInfo(void);
      DialogEventInfo(const DialogEventInfo& rhs);
      DialogEventInfo& operator=(const DialogEventInfo& dialogEventInfo);
      
      //based on DialogEventId
      bool operator==(const DialogEventInfo& rhs) const;
      bool operator!=(const DialogEventInfo& rhs) const;
      bool operator<(const DialogEventInfo& rhs) const;
         
      enum Direction 
      {
         Initiator,
         Recipient
      };

      enum State
      {
         Trying = 0,
         Proceeding,
         Early,
         Confirmed,
         Terminated
      };

      const State& getState() const;

      const Data& getDialogEventId() const;
      
      const Data& getCallId() const;
      const Data& getLocalTag() const;
      bool hasRemoteTag() const;
      const Data& getRemoteTag() const;
      
      bool hasRefferedBy() const;
      const NameAddr& getRefferredBy() const;
      
      const NameAddr& getLocalIdentity() const;
      const NameAddr& getRemoteIdentity() const;
      const Uri& getLocalTarget() const;
      //has... below only applicable when direction is outgoing
      bool hasRouteSet() const;
      const NameAddrs& getRouteSet() const;
      bool hasRemoteTarget() const;
      const Uri& getRemoteTarget() const;

      // cache the first one, then forevermore lookup from InviteSession
      const Contents& getLocalOfferAnswer() const;
      const Contents& getRemoteOfferAnswer() const;
      bool hasLocalOfferAnswer() const;
      bool hasRemoteOfferAnswer() const;

      UInt64 getDurationSeconds() const; // in seconds

      bool hasReplacesId() const;
      const DialogId& getReplacesId() const;

      Direction getDirection() const;

   protected:
      friend class DialogEventStateManager;

      State mState;
      Data mDialogEventId; //unique for all Dialogs at this ua...may hash local +
                          //callid, all 3 tags for forks.  Or could cycles an
                          //integer...hash memory location+salt at cons time(might be easiest).
      DialogId mDialogId;
      Direction mDirection;
      //ID of the dialog this dialog replaced.
      std::auto_ptr<DialogId> mReplacesId;
      InviteSessionHandle mInviteSession;

      std::auto_ptr<NameAddr> mReferredBy;

//could back-point to dialog for this information to save space
      NameAddrs mRouteSet; 
      NameAddr mLocalIdentity;
      NameAddr mRemoteIdentity;
      Uri mLocalTarget;
      std::auto_ptr<Uri> mRemoteTarget;

      UInt64 mCreationTimeSeconds;

      std::auto_ptr<Contents> mLocalOfferAnswer;
      std::auto_ptr<Contents> mRemoteOfferAnswer;

   private:
      bool mReplaced;
};

}

#endif


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
