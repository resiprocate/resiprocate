#if !defined(RESIP_SERVERPUBLICATION_HXX)
#define RESIP_SERVERPUBLICATION_HXX

#include "resiprocate/dum/BaseUsage.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/dum/Win32ExportDum.hxx"

namespace resip
{

class DUM_API ServerPublication : public BaseUsage 
{
   public:
      friend class InternalServerPublicationMessage_End;
      friend class InternalServerPublicationMessage_DispatchSipMsg;
      friend class InternalServerPublicationMessage_DispatchTimeoutMsg;
      friend class InternalServerPublicationMessage_Send;

      typedef Handle<ServerPublication> ServerPublicationHandle;
      ServerPublicationHandle getHandle();

      const Data& getEtag() const;
      const Data& getDocumentKey() const;
      
      SipMessage& accept(int statusCode = 200);
      SipMessage& reject(int responseCode);
      
      const Data& getPublisher() const; // aor of From

      // Async
      virtual void endAsync();
      virtual void dispatchAsync(const SipMessage& msg);
      virtual void dispatchAsync(const DumTimeout& timer);
      void sendAsync(const SipMessage& response);
      void sendAsync(bool accept, int statusCode);
      // -------
      
protected:  // Sync
      virtual void end();

      virtual void dispatch(const SipMessage& msg);
      virtual void dispatch(const DumTimeout& timer);

      void send(SipMessage& response);

   protected:
      virtual ~ServerPublication();
      void updateMatchingSubscriptions();

   private:
      friend class DialogUsageManager;
      ServerPublication(DialogUsageManager& dum, const Data& etag, const SipMessage& request);

      SipMessage mLastRequest;
      SipMessage mLastResponse;
      const Data mEtag;
      const Data mEventType;
      const Data mDocumentKey;
      Helper::ContentsSecAttrs mLastBody;
      int mTimerSeq;
      int mExpires;
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
