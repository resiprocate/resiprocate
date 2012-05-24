#include "rutil/Data.hxx"
#include "rutil/DigestStream.hxx"
#include "rutil/Log.hxx"
#include "assert.h"

#include <openssl/evp.h>


using namespace resip;
using namespace std;


int
main()
{
   {
      DigestStream str(EVP_sha1());
      assert(str.getHex() == "da39a3ee5e6b4b0d3255bfef95601890afd80709");
   }

   {
      Data input("sip:alice@atlanta.example.com"
                 ":a84b4c76e66710"
                 ":314159 INVITE"
                 ":Thu, 21 Feb 2002 13:02:03 GMT"
                 ":sip:alice@pc33.atlanta.example.com"
                 ":v=0\r\n"
                 "o=UserA 2890844526 2890844526 IN IP4 pc33.atlanta.example.com\r\n"
                 "s=Session SDP\r\n"
                 "c=IN IP4 pc33.atlanta.example.com\r\n"
                 "t=0 0\r\n"
                 "m=audio 49172 RTP/AVP 0\r\n"
                 "a=rtpmap:0 PCMU/8000\r\n");
      {
         
         DigestStream str(EVP_sha1());
         str << input;

         assert(str.getHex() == "f2eec616bd43c75fd1cd300691e57301b52b3ad3");
      }
//       {
//          DigestStream str(EVP_sha1());
//          str << input;
//          assert(str.getBin(32).size() == 4);
//       }
      {
         DigestStream str(EVP_sha1());
         str << input;
         assert(str.getBin().size() == 20);
      }
//       {
//          DigestStream str(EVP_sha1());
//          str << input;
//          assert(str.getBin(128).size() == 16);
//       }

      Data a;
      Data b;
//       {
//          DigestStream str(EVP_sha1());
//          str << input;
//          a = str.getBin(32);
//          cerr << "A: " << a.hex() << endl;
//       }
//       {
//          DigestStream str(EVP_sha1());
//          str << input;
//          b = str.getBin(128);
//          cerr << "B: " << b.hex() << endl;
//       }
      
//      assert(memcmp(a.c_str(), b.c_str()+12, 4) == 0);
   }


   cerr << "All OK" << endl;

   return 0;
}
/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2005 Vovida Networks, Inc.  All rights reserved.
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
