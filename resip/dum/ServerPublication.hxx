#if !defined(RESIP_SERVERPUBLICATION_HXX)
#define RESIP_SERVERPUBLICATION_HXX

#include "rutil/SharedPtr.hxx"
#include "resip/dum/BaseUsage.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Helper.hxx"

namespace resip
{

class ServerPublication : public BaseUsage 
{
   public:
      typedef Handle<ServerPublication> ServerPublicationHandle;
      ServerPublicationHandle getHandle();

      const Data& getEtag() const;
      const Data& getDocumentKey() const;
      
      SharedPtr<SipMessage> accept(int statusCode = 200);
      SharedPtr<SipMessage> reject(int responseCode);

      virtual void end();

      virtual void dispatch(const SipMessage& msg);
      virtual void dispatch(const DumTimeout& timer);

      void send(SharedPtr<SipMessage> response);      

      Data getPublisher() const; // aor of From
      virtual EncodeStream& dump(EncodeStream& strm) const;
      
   protected:
      virtual ~ServerPublication();
      void updateMatchingSubscriptions();

   private:
      friend class DialogUsageManager;
      ServerPublication(DialogUsageManager& dum, const Data& etag, const SipMessage& request);
      SipMessage mLastRequest;
      SharedPtr<SipMessage> mLastResponse;
      const Data mEtag;
      const Data mEventType;
      const Data mDocumentKey;
      Helper::ContentsSecAttrs mLastBody;
      unsigned int mTimerSeq;
      UInt32 mExpires;
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
