#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <iostream>
#include <stdexcept>

#include "resip/stack/ssl/Security.hxx"
//#include "rutil/ssl/OpenSSLInit.hxx"
#include "rutil/ThreadIf.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"

#ifdef USE_SSL
#include <openssl/evp.h>
#endif

using namespace std;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

// the destructor in BaseSecurity started crashing on the Mac and Windows
// at Revision 5785. The crash can be reproduced by creating 2 security
// objects, one after another.
void testMultiple()
{
   {
      Security security;
   }
#ifdef WIN32
      Sleep(10000);
#else
      sleep(10);      
#endif
   {
      Security security;
   }
}

class HashThread : public ThreadIf
{
   public:
      void thread()
      {
         Security security;
         for(int i = 0; i < 500000; i++)
         {
            if (i % 1000 == 0)
            {
               DebugLog(<< "1000 digest calculations complete. ");               
            }
            makeMD5Digest("I don't give a damn about digest");
         }
      }
   private:
      void makeMD5Digest(const char *pBuf)
      {

#ifdef USE_SSL
         unsigned char MD5_digest[EVP_MAX_MD_SIZE+1];
         unsigned int iDigest = 0;
         memset(MD5_digest, 0, sizeof(MD5_digest));

         if(0 == pBuf)
            return;

         const EVP_MD *pDigest = EVP_md5();
         if( 0 == pDigest)
            return;

         EVP_MD_CTX cCtx;
         EVP_DigestInit(&cCtx, pDigest);
         EVP_DigestUpdate(&cCtx, pBuf, strlen(pBuf));
         EVP_DigestFinal(&cCtx, MD5_digest, &iDigest);
         EVP_MD_CTX_cleanup(&cCtx);

//         cout << "Your digest is: " << MD5_digest << endl;
#else
//         cout << "OpenSSL not enabled; cannot calculate digest !!!";
#endif
      }
};

class DumbInitThread : public ThreadIf
{
   public:
      void thread()
      {
         for(int i = 0; i < 500; i++)
         {
            DebugLog(<< "Creating Security object on stack");           
            Security security;
         }
      }
};

void testSecurityMultiThread()
{
   HashThread t1;
   DumbInitThread t2;

   t1.run();
   t2.run();

   t1.join();
   t2.join();
}


int
main(int argc, const char** argv)
{
   Log::initialize(Log::Cout, Log::Debug, argv[0]);
   assert(Security::parseSSLType("TLSv1") == SecurityTypes::TLSv1);
   assert(Security::parseSSLType("SSLv23") == SecurityTypes::SSLv23);
   try
   {
      SecurityTypes::SSLType val = BaseSecurity::parseSSLType("BigBrotherIsWatching");
      assert(0); // should have thrown an exception
   }
   catch (const invalid_argument& ia) { } // ignore, expected exception

   assert(Security::parseOpenSSLCTXOption("SSL_OP_NO_SSLv2") == SSL_OP_NO_SSLv2);
   assert(Security::parseOpenSSLCTXOption("SSL_OP_NO_SSLv3") == SSL_OP_NO_SSLv3);
   try
   {
      SecurityTypes::SSLType val = BaseSecurity::parseSSLType("BigBrotherIsWatching");
      assert(0); // should have thrown an exception
   }
   catch (const invalid_argument& ia) { } // ignore, expected exception

#if 0
   testMultiple();
#else
   testSecurityMultiThread();
#endif
   return 0;
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
