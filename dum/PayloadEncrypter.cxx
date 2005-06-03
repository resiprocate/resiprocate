#include <cassert>
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/dum/DumEncrypted.hxx"
#include "resiprocate/dum/BaseUsage.hxx"
#include "resiprocate/dum/Handles.hxx"
#include "resiprocate/TransactionUser.hxx"
#include "resiprocate/Security.hxx"
#include "resiprocate/dum/PayloadEncrypter.hxx"
#include "resiprocate/Contents.hxx"
#include "resiprocate/MultipartSignedContents.hxx"
#include "resiprocate/Pkcs7Contents.hxx"

using namespace resip;
using namespace std;

PayloadEncrypter::PayloadEncrypter(TransactionUser& tu, Security* security)
   : mTu(tu),
     mSecurity(security)
{
   assert(mSecurity);
}

PayloadEncrypter::~PayloadEncrypter()
{
}

void PayloadEncrypter::encrypt(std::auto_ptr<Contents> src, 
                               const Data& senderAor, 
                               BaseUsageHandle handle)
{
   auto_ptr<Contents> contents;
   try 
   {
      MultipartSignedContents* multipart = mSecurity->sign(senderAor, src.get());
      contents = auto_ptr<Contents>(static_cast<Contents*>(multipart));
      DumEncrypted* encrypted = new DumEncrypted(true, "", handle, contents);
      mTu.post(encrypted);
   }
   catch (Security::Exception& e) 
   {
      DumEncrypted* encrypted = new DumEncrypted(false, e.getMessage(), handle, contents);
      mTu.post(encrypted);
   }
}

void PayloadEncrypter::encrypt(std::auto_ptr<Contents> src,
                               const Data& senderAor,
                               const Data& recipAor,
                               BaseUsageHandle handle)
{
   auto_ptr<Contents> contents;
   try
   {
      //Pkcs7Contents* signAndEncrypted = mSecurity->signAndEncrypt(senderAor, src.get(), recipAor);
      Pkcs7Contents* signAndEncrypted = mSecurity->encrypt(src.get(), recipAor); // signAndEncrpyt no-op, just for testing.
      contents = auto_ptr<Contents>(static_cast<Contents*>(signAndEncrypted));
      DumEncrypted* encrypted = new DumEncrypted(true, "", handle, contents);
      mTu.post(encrypted);
   }
   catch (Security::Exception& e)
   {
      DumEncrypted* encrypted = new DumEncrypted(false, e.getMessage(), handle, contents);
      mTu.post(encrypted);
   }
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
