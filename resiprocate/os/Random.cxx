#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#ifdef _WIN32
#define _CRT_RAND_S
#include <stdlib.h>
#endif 

#include <cassert>
#include <limits>

#ifdef _WIN32
#include "resiprocate/os/Socket.hxx"
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include "resiprocate/os/Random.hxx"
#include "resiprocate/os/Timer.hxx"
#include "resiprocate/os/Mutex.hxx"
#include "resiprocate/os/Lock.hxx"
#if !defined(DISABLE_RESIP_LOG)
#include "resiprocate/os/Logger.hxx"
#endif
#ifdef USE_SSL
#  define USE_OPENSSL 1
#else
#  define USE_OPENSSL 0
#endif

#if ( USE_OPENSSL == 1 )
#  include <openssl/e_os2.h>
#  include <openssl/rand.h>
#  include <openssl/err.h>
#endif

#ifdef _WIN32
#include <WinBase.h>
#include <ObjBase.h>
#endif

using namespace resip;

#if !defined(DISABLE_RESIP_LOG)
#define RESIPROCATE_SUBSYSTEM Subsystem::SIP
#endif

bool Random::sIsInitialized = false;
Mutex Random::sMutex;
#if 0
Random::Init Random::initer;
#endif

void
Random::initialize()
{  
   Lock lock(sMutex);

   if (!sIsInitialized)
   {
      Timer::setupTimeOffsets();
      
      //throwing away first 32 bits
#ifdef _WIN32
      // !polo! - if GUID or rand_s is used, srand() is unnecessary, consider to remove it?
      LARGE_INTEGER largSeed;
      unsigned int seed;
      if(::QueryPerformanceCounter(&largSeed))  // better than GetTickCount().
      {
         seed = largSeed.LowPart;
      }
      else
      {
         seed = ::GetTickCount();
      }
#else
      unsigned int seed = static_cast<unsigned int>(Timer::getTimeMs());
#endif

#ifdef _WIN32
      Socket fd = -1;
      // !cj! need to find a better way - use pentium random commands?
#else
      int fd = open("/dev/urandom", O_RDONLY);
      // !ah! blocks on embedded devices -- not enough entropy.
      if ( fd != -1 )
      {
        int s = read( fd,&seed,sizeof(seed) ); //!ah! blocks if /dev/random on embedded sys

         if ( s != sizeof(seed) )
         {
//            ErrLog( << "System is short of randomness" ); // !ah! never prints
         }
      }
      else
      {
//         ErrLog( << "Could not open /dev/urandom" );
      }
#endif

#if USE_OPENSSL
      if (fd == -1 )
      {
         // really bad sign - /dev/random does not exist so need to intialize
         // OpenSSL some other way

// !cj! need to fix         assert(0);
      }
      else
      {
         char buf[1024/8]; // size is number byes used for OpenSSL init 

         int s = read( fd,&buf,sizeof(buf) );

         if ( s != sizeof(buf) )
         {
//            ErrLog( << "System is short of randomness" );
         }
         
         RAND_add(buf,sizeof(buf),double(s*8));
      }
#endif

      if (fd != -1 )
      {
#ifdef _WIN32
		 closeSocket(fd);
#else
         close(fd);
#endif
      }

#ifdef WIN32
      srand(seed);
      srand(rand());
#else
      srandom(seed);
      srandom(random());
#endif
      sIsInitialized = true;
   }
}

int
Random::getRandom()
{
   if (!sIsInitialized)
   {
     initialize();
   }
   // !dlb! Lock
   assert( sIsInitialized == true );
#ifdef WIN32
   assert( RAND_MAX == 0x7fff );
# if 1
   int r1;
   GUID guid;
   if(::CoCreateGuid(&guid) == S_OK)
   {
      r1 = (long)guid.Data1;
   }
   else
   {
      rand_s((unsigned int*)&r1);
   }
   return r1;
# else
   Lock lock(sMutex);
   double r1 = ((double)rand()/((double)(RAND_MAX) + 1.0));
   double r2 = ((double)rand()/((double)(RAND_MAX) + 1.0));
   DebugLog( << "Random1: " << r1 << "|Random2: " << r2);

   return (((int)(((double)(std::numeric_limits<unsigned short>::max)()) * r1) + 1) << 16) | 
      ((int)(((double)(std::numeric_limits<unsigned short>::max)()) * r2) + 1);
# endif
#else
   return random(); 
#endif
}

Int64
Random::getRandom64()
{
   if (!sIsInitialized)
   {
      initialize();
   }
   union
   {
      Int64 ret;
      int   ret2[2];
   };
   // !dlb! Lock
   assert( sIsInitialized == true );
#ifdef WIN32
   assert( RAND_MAX == 0x7fff );
   GUID guid;
   if(::CoCreateGuid(&guid) == S_OK)
   {
      ret = *((Int64*)(guid.Data4));
   }
   else
   {
      rand_s((unsigned int*)&ret2[0]);
      rand_s((unsigned int*)&ret2[1]);
   }
#else
   ret2[0] = random();
   ret2[1] = random();
#endif
   return ret;
}

int
Random::getCryptoRandom()
{
   if (!sIsInitialized)
   {
     initialize();
   }
   assert( sIsInitialized == true );
#if USE_OPENSSL
   int ret;
   int e = RAND_bytes( (unsigned char*)&ret , sizeof(ret) );
   if ( e < 0 )
   {
      // error of some type - likely not enough rendomness to dod this 
      long err = ERR_get_error();
      
      char buf[1024];
      ERR_error_string_n(err,buf,sizeof(buf));
#if !defined(DISABLE_RESIP_LOG)    
      ErrLog( << buf );
#endif
      assert(0);
   }
   return ret;
#else
   return getRandom();
#endif
}

Data 
Random::getRandom(unsigned int len)
{
   if (!sIsInitialized)
   {
     initialize();
   }
   assert(sIsInitialized == true);
   assert(len < Random::maxLength+1);
   
   union 
   {
      char cbuf[Random::maxLength + 1];
      unsigned int ibuf[(Random::maxLength)/sizeof(int) + 1];
   };
   ::memset(ibuf, 0, sizeof(ibuf));
   unsigned int count = (len+sizeof(int)-1)/sizeof(int);
   for (unsigned int i = 0; i < count; ++i)
   {
      ibuf[i] = Random::getRandom();
   }
   return Data(cbuf, len);
}

Data 
Random::getRandom64(unsigned int len)
{
   if(len < 8)
   {
      return getRandom(len);
   }

   if (!sIsInitialized)
   {
      initialize();
   }
   assert(sIsInitialized == true);
   assert(len < Random::maxLength+1);

   union 
   {
      char cbuf[Random::maxLength + 1];
      Int64 ibuf[(Random::maxLength)/sizeof(long long) + 1];
   };
   ::memset(ibuf, 0, sizeof(ibuf));
   unsigned int count = (len+sizeof(long long)-1)/sizeof(long long);
   for (unsigned int i = 0; i < count; ++i)
   {
      ibuf[i] = Random::getRandom64();
   }
   return Data(cbuf, len);
}

Data 
Random::getCryptoRandom(unsigned int len)
{
   if (!sIsInitialized)
   {
     initialize();
   }
   assert( sIsInitialized == true );
   assert(len < Random::maxLength+1);
   
   union 
   {
      char cbuf[Random::maxLength+1];
      unsigned int ibuf[(Random::maxLength)/sizeof(int) + 1];
   };
   ::memset(ibuf, 0, sizeof(ibuf));
   unsigned int count = (len+sizeof(int)-1)/sizeof(int);
   for (unsigned int i = 0; i < count; ++i)
   {
      ibuf[count] = Random::getCryptoRandom();
   }
   return Data(cbuf, len);
}

Data 
Random::getRandomHex(unsigned int numBytes)
{
   if (!sIsInitialized)
   {
     initialize();
   }
   assert( sIsInitialized == true );
   return Random::getRandom64(numBytes).hex();
}

Data 
Random::getCryptoRandomHex(unsigned int numBytes)
{
   if (!sIsInitialized)
   {
     initialize();
   }
   assert( sIsInitialized == true );
   return Random::getCryptoRandom(numBytes).hex();
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


