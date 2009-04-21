
#include "resip/stack/SERNonceHelper.hxx"
#include "rutil/Random.hxx"
#include "resip/stack/Helper.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP


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


SERNonceHelper::SERNonceHelper(int serOffset) : serOffset(serOffset)
{
   //privateKey = Data("asdfklsadflkj");
   privateKey = Random::getRandomHex(24);
}

SERNonceHelper::~SERNonceHelper() 
{
}

void
SERNonceHelper::setPrivateKey(const Data& pprivateKey)
{
   this->privateKey = pprivateKey;
}

Data 
SERNonceHelper::makeNonce(const SipMessage& request, const Data& timestamp) 
{
   char buf[8];
   Data md5buf(8, Data::Preallocate);
   Data nonce(40, Data::Preallocate);
   int ts = timestamp.convertInt() + serOffset;
   Helper::integer2hex(buf, ts);
   md5buf.append(buf, 8);
   nonce.append(buf, 8);
   md5buf += privateKey;
   nonce += md5buf.md5(); 
   return nonce;
}

NonceHelper::Nonce
SERNonceHelper::parseNonce(const Data& nonce) 
{
   if(nonce.size() != 40)
   {
      return SERNonceHelper::Nonce(0);
   }
   const char *s = nonce.data();
   unsigned int ts = Helper::hex2integer(s) - serOffset;
   return SERNonceHelper::Nonce(ts); 
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

