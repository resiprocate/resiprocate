#if !defined(RESIP_REGISTRATION_HXX)
#define RESIP_REGISTRATION_HXX


#include <memory>
#include "resiprocate/os/Timer.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/ParserCategories.hxx"

namespace resip
{

class SipMessage;


class Registration 
{
   public: 
      // Register a binding from aor to default contact
      Registration(const Uri& aor);

      // Register a binding from aor to specified contact
      Registration(const Uri& aor, const Uri& contact);

      // Register a binding from aor to specified contact as third party from
      Registration(const Uri& from, const Uri& aor, const Uri& contact);

      void setExpiration(int secs);

      // returns time (in secs) when REGISTER should be refreshed
      UInt64 getTimeToRefresh() const;
      const CallID& getCallID() const;
      bool isRegistered() const;
      
      // Returns the current register request
      SipMessage& getRegistration();

      // Will increment CSeq and return an updated register request
      SipMessage& refreshRegistration();

      // Will modify the register request to unregister
      SipMessage& unregister();
      
      void handleResponse(const SipMessage& response);
      
   private:  
      NameAddr mAor;
      NameAddr mContact;
      NameAddr mFrom;
      
      UInt64 mTimeTillExpiration;
      
      typedef enum 
      {
         Invalid=0,
         Initialized,
         Active,
         Terminated
      } State;
      
      State mState;
      std::auto_ptr<SipMessage> mRegister;
      NameAddrs mContacts;
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
