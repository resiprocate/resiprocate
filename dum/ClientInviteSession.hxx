#if !defined(RESIP_CLIENTINVITESESSION_HXX)
#define RESIP_CLIENTINVITESESSION_HXX

#include "resiprocate/sam/InviteSession.hxx"

namespace resip
{

    class SdpContents;

class ClientInviteSession : public InviteSession
{
   public:
      class Handle : public DialogUsageManager::Handle
      {
         public:
            // throws if no session 
            ClientInviteSession* operator->();
         private:
            DialogUsageManager& mDum;
      };

      /// Called to set the offer that will be used in the next messages that
      /// sends and offer. Does not send an offer 
      virtual void setOffer(SdpContents* offer);
      
      /// Sends an offer in whatever messages is approperate to send one at
      /// this point in the dialog. Must call setOffer before this.
      virtual void sendOfferInAnyMessage();
      
      /// Called to set the answer that will be used in the next messages that
      /// sends and offer. Does not send an answer
      virtual void setAnswer(SdpContents* answer);

      /// Sends an offer in whatever messages is approperate to send one at
      /// this point in the dialog. Must call setAnswer before this. 
      virtual void sendAnswerInAnyMessage();

      /// Makes the dialog end. Depending ont eh current state, this might
      /// results in BYE or CANCEL being sent.
      virtual void end();

      /// Rejects an offer at the SIP level. So this can send a 487 to a
      /// reINVITE or and UPDATE
      virtual void rejectOffer(int statusCode);
      
   private:
      friend class DialogUsageManager;
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
