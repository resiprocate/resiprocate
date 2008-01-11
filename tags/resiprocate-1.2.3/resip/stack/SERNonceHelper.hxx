#if !defined(RESIP_BASICNONCEHELPER_HXX)
#define RESIP_BASICNONCEHELPER_HXX

#include "resip/stack/NonceHelper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "rutil/Data.hxx"

/**
 * SERNonceHelper implements the makeNonce function in the same way
 * as SIP Express Router (SER) - http://www.iptel.org/ser
 *
 * To operate a farm/cluster of UASs/proxies, you must:
 * a) make sure the clocks are sychronized (using ntpd for instance)
 * b) use the same privateKey value on every instance of the application
 * c) call Helper::setNonceHelper(mySERNonceHelper) to over-ride
 *    the default implementation of NonceHelper in the reSIProcate stack
 *
 */

namespace resip
{

class SERNonceHelper : public NonceHelper {

   private:
      Data privateKey; 
      // SER puts the expiry time in the nonce, while the reSIProcate stack
      // expects to work with the creation time of the nonce.  Therefore,
      // serOffset should be initialised to the duration for which a SER
      // generated nonce is valid, in seconds.
      int serOffset;

   public:
      SERNonceHelper(int serOffset);
      virtual ~SERNonceHelper();
      void setPrivateKey(const Data& privateKey);
      Data makeNonce(const SipMessage& request, const Data& timestamp);  
      Nonce parseNonce(const Data& nonce);

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

