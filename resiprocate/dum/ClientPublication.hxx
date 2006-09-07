#if !defined(RESIP_CLIENTPUBLICATION_HXX)
#define RESIP_CLIENTPUBLICATION_HXX

#include "resiprocate/dum/NonDialogUsage.hxx"
#include "resiprocate/dum/Win32ExportDum.hxx"

namespace resip
{

class DUM_API ClientPublication : public NonDialogUsage
{
   public:
      friend class InternalClientPublicationMessage_Refresh;
      friend class InternalClientPublicationMessage_Update;
      friend class InternalClientPublicationMessage_End;
      friend class InternalClientPublicationMessage_SipMsg;
      friend class InternalClientPublicationMessage_TimeoutMsg;

      ClientPublication(DialogUsageManager& dum, DialogSet& dialogSet, SipMessage& pub);

      typedef Handle<ClientPublication> ClientPublicationHandle;
      ClientPublicationHandle getHandle();
      const Data& getEventType() { return mEventType; }

      // !polo! aysnc.
      void refreshAsync(unsigned int expiration=0);
      void updateAsync(std::auto_ptr<Contents> body);
      virtual void endAsync();
      virtual void dispatchAsync(const SipMessage& msg);
      virtual void dispatchAsync(const DumTimeout& timer);
      // --------

      const Contents* getContents() const { return mDocument; }      
      virtual std::ostream& dump(std::ostream& strm) const;

   protected:
      //0 means the last value of Expires will be used.
      void refresh(unsigned int expiration=0);
      void update(const Contents* body);

      virtual void end();
      virtual void dispatch(const SipMessage& msg);
      virtual void dispatch(const DumTimeout& timer);

   protected:
      virtual ~ClientPublication();
      virtual void send(SipMessage& request);
      
   private:
      friend class DialogSet;

      bool mWaitingForResponse;
      bool mPendingPublish;
      
      SipMessage& mPublish;
      Data mEventType;
      int mTimerSeq; // expected timer seq (all < are stale)
      const Contents* mDocument;
      
      // disabled
      ClientPublication(const ClientPublication&);
      ClientPublication& operator=(const ClientPublication&);
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
