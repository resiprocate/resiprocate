#include <cassert>
#include <cstdlib>

#include <stdio.h>
#include "sip2/util/RandomHex.hxx"
#include <cstdlib>
#include "sip2/util/Timer.hxx"
#include "sip2/util/Mutex.hxx"
#include "sip2/util/Lock.hxx"


#ifndef USE_OPENSSL
#  if defined(WIN32) || defined(__QNX__) || ( __linux__ ) // archs where it defaults to off 
#    define USE_OPENSSL 0
#  endif
#  if defined( __sun__ ) // arch where it defaults to on 
#    define USE_OPENSSL 1
#  endif
#  ifndef USE_OPENSSL // default value for unkown archs
#    define USE_OPENSSL 0
#  endif
#endif

#if ( USE_OPENSSL == 1 )
#  include <openssl/rand.h>

using namespace Vocal2;

void
RandomHex::initialize()
{}

Data
RandomHex::get(unsigned int len)
{
   unsigned char buffer[len];
   int ret = RAND_bytes(buffer, len);
   assert (ret == 1);
   
   Data result;
   result = convertToHex(buffer, len);
   
   return result;
}

#else


#ifdef WIN32
inline int randomCall() { return rand(); }
#else
inline int randomCall() { return random(); }
#endif

using namespace Vocal2;

static bool sRandomCalled = false;
static Mutex mutex;

void
RandomHex::initialize()
{
   Lock lock(mutex);
   //throwing away first 32 bits
   unsigned int seed = static_cast<unsigned int>(Timer::getTimeMs());
#ifdef WIN32
   srand(seed);
#else
   srandom(seed);
#endif
}


Data
RandomHex::get(unsigned int len)
{
   Lock lock(mutex);
   if (!sRandomCalled)
   {
      unsigned int seed = static_cast<unsigned int>(Timer::getTimeMs());
#ifdef WIN32
      srand(seed);
#else
      srandom(seed);
#endif
      sRandomCalled = true;
   }
   unsigned char buf[1024]; 
   assert( len < 1024 );
   unsigned int count=0;  //has to be signed

   for (count=0; count < len/4; count++)
   {
      //generate a random number;
      int rand = randomCall();
      memcpy(buf + (count*4), &rand, 4);  //copy 4 bytes from rand to buf.
   }
   
   // now copy the remainder
   int remainder = len % 4;
   if (remainder)
   {
      int rand = randomCall();
      memcpy(buf + (count*4), &rand, remainder);  //copy 4 bytes from rand to buf.
   }
   return convertToHex(buf, len);   
}

Data 
RandomHex::convertToHex(const unsigned char* src, int len)
{
   // !cj! this is really a poor way to build - don't use sprintf, don't reallooc the data buffer size 
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

        sprintf(buf, "%x%x", hi, low); // !cj! - this is likely a slow way to do this
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

#endif
