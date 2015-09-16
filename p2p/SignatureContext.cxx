#include "SignatureContext.hxx"

#include <openssl/evp.h>

#if 0
namespace p2p {
SignatureContext::SignatureContext(Profile &profile)
   : mProfile(profile)
{
   ;
}

Data SignatureContext::computeSignature(const vector<Data> toBeSigned)
{
   EVP_MD_CTX md;
   
   const EVP_MD *digest_ptr=EVP_get_digestbyname(mProfile.signatureDigest().c_str());
   
   resip_assert(digest_ptr!=0);
   
   if(!EVP_SignInit(md,digest_ptr))
      resip_assert(0); // This should not fail
   
   for(unsigned int i=0;i<toBeSigned.size();i++)
   {
      EVP_DigestUpdate(&md,toBeSigned[i].data(),toBeSigned[i].size());
   }


   unsigned char *sig_buf sig_buf=new unsigned char[EVP_PKEY_size(size)];
   unsigned int sig_len;
   
   if(!EVP_SignFinal(&md,sig_buf,&sig_len,mProfile.getPrivateKey()))
   {
      delete sig_buf[];
   }

   return Data(sig_buf);
}






}
#endif

/* ======================================================================
 *  Copyright (c) 2008, Various contributors to the Resiprocate project
 *  All rights reserved.
 *  
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *      - Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 *      - The names of the project's contributors may not be used to
 *        endorse or promote products derived from this software without
 *        specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 *  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 *  THE POSSIBILITY OF SUCH DAMAGE.
 *====================================================================== */

