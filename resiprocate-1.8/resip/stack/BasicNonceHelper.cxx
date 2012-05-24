

#include "resip/stack/BasicNonceHelper.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/Random.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP


/**
 * BasicNonceHelper implements the makeNonce function in the same way
 * as the original implementation of makeNonce within Helper.cxx
 *
 * To operate a farm/cluster of UASs/proxies, you must:
 * a) make sure the clocks are sychronized (using ntpd for instance)
 * b) use the same privateKey value on every instance of the application
 *
 * To operate with SER, use the SERNonceHelper instead, as that generates
 * the nonce string the same way as SER.  You must also observe the same
 * conditions above regarding clock and key synchronization.
 */
BasicNonceHelper::BasicNonceHelper() 
{
  //privateKey = Data("asdfklsadflkj");
  privateKey = Random::getRandomHex(24);
}

BasicNonceHelper::~BasicNonceHelper() 
{
}

void
BasicNonceHelper::setPrivateKey(const Data& pprivateKey)
{
  this->privateKey = pprivateKey;
}

Data 
BasicNonceHelper::makeNonce(const SipMessage& request, const Data& timestamp) 
{
   Data nonce(100, Data::Preallocate);
   nonce += timestamp;
   nonce += Symbols::COLON;
   Data noncePrivate(100, Data::Preallocate);
   noncePrivate += timestamp;
   noncePrivate += Symbols::COLON;
   // !jf! don't include the Call-Id since it might not be the same.
   // noncePrivate += request.header(h_CallId).value();
   noncePrivate += request.header(h_From).uri().user();
   noncePrivate += privateKey;
   nonce += noncePrivate.md5();
   return nonce;
}

NonceHelper::Nonce
BasicNonceHelper::parseNonce(const Data& nonce) 
{
   ParseBuffer pb(nonce.data(), nonce.size());
   if (!pb.eof() && !isdigit(*pb.position()))
   {
      DebugLog(<< "Invalid nonce; expected timestamp.");
      return BasicNonceHelper::Nonce(0);
   }
   const char* anchor = pb.position();
   pb.skipToChar(Symbols::COLON[0]);
   if (pb.eof())
   {
      DebugLog(<< "Invalid nonce; expected timestamp terminator.");
      return BasicNonceHelper::Nonce(0);
   }
   Data creationTime;
   pb.data(creationTime, anchor);
   return BasicNonceHelper::Nonce(creationTime.convertUInt64());
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

