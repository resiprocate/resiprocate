#include <cassert>
#include <cstdlib>

#include <stdio.h>
#include <util/RandomHex.hxx>

#if !defined(WIN32)
#include <openssl/rand.h>
#endif

using namespace Vocal2;

void
RandomHex::initialize()
{
#if defined(WIN32)
   // TODO FIX 
   unsigned int seed = 1;
   srand(seed);
#else
    assert(RAND_status() == 1);
#endif
}

Data
RandomHex::get(unsigned int len)
{
#if defined(WIN32)
   assert( len <= 16 );
   unsigned char buffer[16];
   int ret = rand();
   assert (ret == 1);
   
   return convertToHex(buffer, len);
#else
   unsigned char buffer[len];
   int ret = RAND_bytes(buffer, len);
   assert (ret == 1);
   
   Data result;
   result = convertToHex(buffer, len);
   
   return result;
#endif
}

Data 
RandomHex::convertToHex(const unsigned char* src, int len)
{
    Data data;

    unsigned char temp;

    //convert to hex.
    int i;
    //int j = 0;

    for (i = 0; i < len; i++)
    {
        temp = src[i];

        int hi = (temp & 0xf0) / 16;
        int low = (temp & 0xf);

        char buf[4];
        buf[0] = '\0';

        sprintf(buf, "%x%x", hi, low);
        data += buf;
    }

    return data;
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
