#include <cassert>

#ifdef WIN32
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
#include "resiprocate/os/Logger.hxx"

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

using namespace Vocal2;

#define VOCAL_SUBSYSTEM Subsystem::SIP

bool Random::mIsInitialized = false;
Random::Init Random::initer;

void
Random::initialize()
{  
   static Mutex mutex;
   Lock lock(mutex);

   if (!mIsInitialized)
   {
      Timer::setupTimeOffsets();
      
      //throwing away first 32 bits
      unsigned int seed = static_cast<unsigned int>(Timer::getTimeMs());

#ifdef WIN32
      Socket fd = -1;
      // !cj! need to find a better way - use pentium random commands?
#else
      int fd = open("/dev/random",O_RDONLY);
      if ( fd != -1 )
      {
         int s = read( fd,&seed,sizeof(seed) );

         if ( s != sizeof(seed) )
         {
//            ErrLog( << "System is short of randomness" );
         }
      }
      else
      {
//         ErrLog( << "Could not open /dev/random" );
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
         closesocket(fd);
      }

#ifdef WIN32
      srand(seed);
#else
      srandom(seed);
#endif
      mIsInitialized = true;
   }
}

int
Random::getRandom()
{
   if (!mIsInitialized)
   {
     initialize();
   }
   // !dlb! Lock
   assert( mIsInitialized == true );
#ifdef WIN32
   assert( RAND_MAX == 0x7fff );
   int r1 = rand();
   int r2 = rand();

   int ret = (r1<<16) + r2;

   return ret;
#else
   return random(); 
#endif
}

int
Random::getCryptoRandom()
{
   if (!mIsInitialized)
   {
     initialize();
   }
   assert( mIsInitialized == true );
#if USE_OPENSSL
   int ret;
   int e = RAND_bytes( (unsigned char*)&ret , sizeof(ret) );
   if ( e != 1 )
   {
      // error of some type - likely not enough rendomness to dod this 
      long err = ERR_get_error();
      
      char buf[1024];
      ERR_error_string_n(err,buf,sizeof(buf));
      
      ErrLog( << buf );
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
   if (!mIsInitialized)
   {
     initialize();
   }
   assert( mIsInitialized == true );

   // !dlb! unnecessary copy
   char buf[512];
   assert( len <= sizeof(buf) );

   char* p=buf;
   unsigned int count=0;
   
   while ( count < len )
   {
      int data = Random::getRandom();

      assert(sizeof(int) == 4 );
      char* d = reinterpret_cast<char*>( &data );
		
      memcpy(p,d,sizeof(int));
      p += sizeof(int);
      count += sizeof(int);
   }

   Data ret(buf,len);

   assert( ret.size() == len );
   return ret;
}

Data 
Random::getCryptoRandom(unsigned int len)
{
   if (!mIsInitialized)
   {
     initialize();
   }
   assert( mIsInitialized == true );

   char buf[512];
   assert( len <= sizeof(buf) );

   char* p=buf;
   unsigned int count=0;
   
   while ( count < len )
   {
     
      int data = Random::getCryptoRandom();
      assert(sizeof(int) == 4 );
      char* d = reinterpret_cast<char*>( &data );
		
      memcpy(p,d,sizeof(int));
      p += sizeof(int);
      count += sizeof(int);
   }

   Data ret(buf,len);

   assert( ret.size() == len );
   return ret;
}

Data 
Random::getRandomHex(unsigned int numBytes)
{
   if (!mIsInitialized)
   {
     initialize();
   }
   assert( mIsInitialized == true );
   Data rand = Random::getRandom(numBytes);
   return rand.hex();
}

Data 
Random::getCryptoRandomHex(unsigned int numBytes)
{
   if (!mIsInitialized)
   {
     initialize();
   }
   assert( mIsInitialized == true );
   Data rand = Random::getCryptoRandom(numBytes);
   return rand.hex();
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

