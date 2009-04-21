#if !defined(STATELESS_PROXY_HXX)
#define STATELESS_PROXY_HXX

#include "resip/stack/SipStack.hxx"
#include "rutil/Data.hxx"
#include "rutil/ThreadIf.hxx"

namespace resip
{

class StatelessProxy : public resip::SipStack, public resip::ThreadIf
{
   public:
      // If targetHost=0, use dns (SRV, etc) to route the request
      StatelessProxy(const char* proxyHost, int proxyPort, 
                     const char* targetHost=0, int targetPort=0, const char* proto=0);
      virtual ~StatelessProxy(){};
      void thread();
      const resip::Uri& uri() const { return mProxyUrl; }

   protected:
      // will be called just before the request is forwarded. Return true if the
      // request is to be forwarded. Return false otherwise
      // by default return true. 
      virtual bool onForwardRequest(resip::SipMessage* msg);

      // will be called just before the response is forwarded. Return true to
      // forward the response. 
      virtual bool onForwardResponse(resip::SipMessage* msg);
      
   private:
      // returns false if request did NOT validate
      bool requestValidation(resip::SipMessage* msg);// 16.3
      void preprocess(resip::SipMessage* msg); // 16.4 
      void forwardRequest(resip::SipMessage* msg); // 16.6
      void handleResponse(resip::SipMessage* msg);
      
      void sendResponse(resip::SipMessage* request, int code, const resip::Data& reason=resip::Data::Empty);
      
   private:
      bool mLoose; // is a loose router
      bool mUseTarget;
      resip::Uri mTarget;
      resip::Uri mProxyUrl;
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
 * This software consists of voluntary contributions made by PurpleComm,
 * Inc. and many individuals on behalf of PurpleComm, Inc. Inc.
 *
 */
