#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "rutil/ResipAssert.h"
#include <stdlib.h>

#ifdef WIN32
#include "rutil/Socket.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Data.hxx"
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include "rutil/Random.hxx"
#include "rutil/Timer.hxx"
#include "rutil/Mutex.hxx"
#include "rutil/Lock.hxx"
#include "rutil/Logger.hxx"


#ifdef USE_SSL
#ifdef WIN32
//hack for name collision of OCSP_RESPONSE and wincrypt.h in latest openssl release 0.9.8h
//http://www.google.com/search?q=OCSP%5fRESPONSE+wincrypt%2eh
//continue to watch this issue for a real fix.
#undef OCSP_RESPONSE
#endif
#include "rutil/ssl/OpenSSLInit.hxx"
#  define USE_OPENSSL 1
#else
#  define USE_OPENSSL 0
#endif

#if ( USE_OPENSSL == 1 )
#  include <openssl/e_os2.h>
#  include <openssl/rand.h>
#  include <openssl/err.h>
#endif

using namespace resip;
#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

Mutex Random::mMutex;
bool Random::mIsInitialized = false;

#ifdef WIN32
Random::Initializer Random::mInitializer;
#ifdef RESIP_RANDOM_WIN32_RTL
BOOLEAN (APIENTRY *Random::RtlGenRandom)(void*, ULONG) = 0;
#endif
#endif //WIN32

#ifdef RESIP_RANDOM_THREAD_MUTEX
struct random_data* Random::sRandomState = 0;
#endif

#ifdef RESIP_RANDOM_THREAD_LOCAL
ThreadIf::TlsKey Random::sRandomStateKey = 0;
#endif

#define RANDOM_STATE_SIZE 128

const char*
Random::getImplName()
{
#ifdef WIN32
#if defined(RESIP_RANDOM_WIN32_RTL)
   return "win32_rtl";
#else
   return "win32_rand";
#endif
#else // WIN32
#if defined(RESIP_RANDOM_THREAD_LOCAL)
   return "posix_thread_local";
#elif defined(RESIP_RANDOM_THREAD_MUTEX)
   return "posix_thread_mutex";
#else
   return "posix_random";
#endif
#endif // not WIN32
}

/**
   Key goal is to make sure that each thread has distinct seed.
**/
unsigned
Random::getSimpleSeed()
{
   // !cj! need to find a better way - use pentium random commands?
   Data buffer;
   {
      DataStream strm(buffer);
#ifdef WIN32
      strm << GetTickCount() << ":";
      strm << GetCurrentProcessId() << ":";
      strm << GetCurrentThreadId();
#else
      // .kw. previously just used the lower 32bits of getTimeMs()
      strm << ResipClock::getTimeMicroSec() << ":";
      strm << getpid();
#if defined(RESIP_RANDOM_THREAD_LOCAL)
      strm << ":" << ThreadIf::selfId();
#endif
#endif
   }
   return (unsigned int)buffer.hash();
}

void
Random::initialize()
{  
#ifdef WIN32
//#if defined(USE_SSL)
#if 0 //!dcm! - this shouldn't be per thread for win32, and this is slow. Going
      //to re-work openssl initialization
   if ( !Random::mIsInitialized)
   {
      Lock lock(mMutex);
      if (!Random::mIsInitialized)
      {
         mIsInitialized = true;
         RAND_screen ();
      }
   }
#else      
   if (!Random::mInitializer.isInitialized())
   {
      Lock lock(mMutex);      
      if (!Random::mInitializer.isInitialized())
      {
         Random::mInitializer.setInitialized();

         unsigned seed = getSimpleSeed();
         srand(seed);

#ifdef RESIP_RANDOM_WIN32_RTL
         // .jjg. from http://blogs.msdn.com/michael_howard/archive/2005/01/14/353379.aspx
         // srand(..) and rand() have proven to be insufficient sources of randomness,
         // leading to transaction id collisions in resip.
         // SystemFunction036 maps to RtlGenRandom, which is used by rand_s() (which is available
         // only with the VC 8.0 runtime or later) and is the Microsoft-recommended way of getting
         // a random number. This code allows that functionality to be accessed even from VC 7.1.
         // However, SystemFunction036 only exists in Windows XP and later, so we may need to fallback
         // to the old method using rand().
         HMODULE hLib = LoadLibrary("ADVAPI32.DLL");
         if (hLib)
         {
            Random::RtlGenRandom =
               (BOOLEAN (APIENTRY *)(void*,ULONG))GetProcAddress(hLib,"SystemFunction036");

            if (!Random::RtlGenRandom)
            {
               WarningLog(<< "Using srand(..) and rand() for random numbers");
            }
         }
#endif   // RESIP_RANDOM_WIN32_RTL

         mIsInitialized = true;

      }
   }
#endif  // not dead code

#else   // WIN32
   // ?dcm? -- OpenSSL will transparently initialize PRNG if /dev/urandom is
   // present. In any case, will move into OpenSSLInit
   if ( !Random::mIsInitialized)
   {
      Lock lock(mMutex);
      if (!Random::mIsInitialized)
      {
         mIsInitialized = true;
         Timer::setupTimeOffsets();

         unsigned seed = getSimpleSeed();

#if defined(RESIP_RANDOM_THREAD_LOCAL)
         ThreadIf::tlsKeyCreate(sRandomStateKey, ::free);
#elif defined(RESIP_RANDOM_THREAD_MUTEX)
         struct random_data *buf;
         size_t sz = sizeof(*buf)+RANDOM_STATE_SIZE;
         buf = (struct random_data*) ::malloc(sz);
         memset( buf, 0, sz);      // .kw. strange segfaults without this
         initstate_r(seed, ((char*)buf)+sizeof(*buf), RANDOM_STATE_SIZE, buf);
         sRandomState = buf;
#else
         srandom(seed);
#endif


         int fd = open("/dev/urandom", O_RDONLY);
         // !ah! blocks on embedded devices -- not enough entropy.
         if ( fd != -1 )
         {
            int s = read( fd,&seed,sizeof(seed) ); //!ah! blocks if /dev/random on embedded sys

            if ( s != sizeof(seed) )
            {
               ErrLog( << "System is short of randomness" ); // !ah! never prints
            }
         }
         else
         {
            ErrLog( << "Could not open /dev/urandom" );
         }

#if defined(USE_SSL)
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
               ErrLog( << "System is short of randomness" );
            }
         
            RAND_add(buf,sizeof(buf),double(s*8));
         }
#endif   // SSL
         if (fd != -1 )
         {
            ::close(fd);
         }
      }
   }
#endif  // not WIN32
}

int
Random::getRandom()
{
   initialize();

#ifdef WIN32

   int ret = 0;

#ifdef RESIP_RANDOM_WIN32_RTL
   // see comment in initialize()
   if (Random::RtlGenRandom)
   {
      unsigned long buff[1];
      ULONG ulCbBuff = sizeof(buff);
      if (Random::RtlGenRandom(buff,ulCbBuff))
      {
         // .kw. all other impls here return positive number, so do the same...
         ret = buff[0] & (~(1<<31));
         return ret;
      }
   }
   // fallback to using rand() if this is a Windows version previous to XP
#endif	// RESIP_RANDOM_WIN32_RTL
   {
      // rand() returns [0,RAND_MAX], which on Windows is 15 bits and positive
      // code below gets 30bits of randomness; with bit31 and bit15
      // always zero; result is always positive
      resip_assert( RAND_MAX == 0x7fff );
      // WATCHOUT: on Linux, rand() returns 31bits, and assert above will fail
      int r1 = rand();
      int r2 = rand();
      ret = (r1<<16) + r2;
   }

   return ret;
#else	// WIN32

#if defined(RESIP_RANDOM_THREAD_LOCAL)
   struct random_data *buf = (struct random_data*) ThreadIf::tlsGetValue(sRandomStateKey);
   if ( buf==NULL ) {
      size_t sz = sizeof(*buf)+RANDOM_STATE_SIZE;
      buf = (struct random_data*) ::malloc(sz);
      memset( buf, 0, sz);      // .kw. strange segfaults without this
      unsigned seed = getSimpleSeed();
      initstate_r(seed, ((char*)buf)+sizeof(*buf), RANDOM_STATE_SIZE, buf);
      ThreadIf::tlsSetValue(sRandomStateKey, buf);
   }
   int32_t ret;
   random_r(buf, &ret);
   return ret;
#elif defined(RESIP_RANDOM_THREAD_MUTEX)
   int32_t ret;
   {
      Lock statelock(mMutex);
      random_r(sRandomState, &ret);
   }
   return ret;
#else
   // random returns [0,RAN_MAX]. On Linux, this is 31 bits and positive.
   // On some platforms it might be on 15 bits, and will need to do something.
   // assert( RAND_MAX == ((1<<31)-1) );  // ?slg? commented out assert since, RAND_MAX is not used in random(), it applies to rand() only
   return random(); 
#endif  // THREAD_LOCAL
#endif // WIN32
}

int
Random::getCryptoRandom()
{
   initialize();

#if USE_OPENSSL
   int ret;
   int e = RAND_bytes( (unsigned char*)&ret , sizeof(ret) );
   if ( e < 0 )
   {
      // error of some type - likely not enough rendomness to dod this 
      long err = ERR_get_error();
      
      char buf[1024];
      ERR_error_string_n(err,buf,sizeof(buf));
      
      ErrLog( << buf );
      resip_assert(0);
   }
   return ret;
#else
   return getRandom();
#endif
}

Data 
Random::getRandom(unsigned int len)
{
   initialize();
   resip_assert(len < Random::maxLength+1);
   
   union 
   {
         char cbuf[Random::maxLength+1];
         unsigned int  ibuf[(Random::maxLength+1)/sizeof(int)];
   };
   
   for (unsigned int count=0; count<(len+sizeof(int)-1)/sizeof(int); ++count)
   {
      ibuf[count] = Random::getRandom();
   }
   return Data(cbuf, len);
}

Data 
Random::getCryptoRandom(unsigned int len)
{
   unsigned char* buf = new unsigned char[len];
   getCryptoRandom(buf, len); // USE_SSL check is in here
   return Data(Data::Take, (char*)buf, len);
}

Data 
Random::getRandomHex(unsigned int numBytes)
{
   return Random::getRandom(numBytes).hex();
}

Data 
Random::getRandomBase64(unsigned int numBytes)
{
   return Random::getRandom(numBytes).base64encode();
}

Data 
Random::getCryptoRandomHex(unsigned int numBytes)
{
   return Random::getCryptoRandom(numBytes).hex();
}

Data 
Random::getCryptoRandomBase64(unsigned int numBytes)
{
   return Random::getCryptoRandom(numBytes).base64encode();
}

/*
   [From RFC 4122]

   The version 4 UUID is meant for generating UUIDs from truly-random or
   pseudo-random numbers.

   The algorithm is as follows:

   o  Set the two most significant bits (bits 6 and 7) of the
      clock_seq_hi_and_reserved to zero and one, respectively.

   o  Set the four most significant bits (bits 12 through 15) of the
      time_hi_and_version field to the 4-bit version number from
      Section 4.1.3. (0 1 0 0)

   o  Set all the other bits to randomly (or pseudo-randomly) chosen
      values.

      UUID                   = time-low "-" time-mid "-"
                               time-high-and-version "-"
                               clock-seq-and-reserved
                               clock-seq-low "-" node
      time-low               = 4hexOctet
      time-mid               = 2hexOctet
      time-high-and-version  = 2hexOctet
      clock-seq-and-reserved = hexOctet
      clock-seq-low          = hexOctet
      node                   = 6hexOctet
      hexOctet               = hexDigit hexDigit
*/
Data 
Random::getVersion4UuidUrn()
{
  Data urn ("urn:uuid:");
  urn += getCryptoRandomHex(4); // time-low
  urn += "-";
  urn += getCryptoRandomHex(2); // time-mid
  urn += "-";

  Data time_hi_and_version = Random::getCryptoRandom(2);
  time_hi_and_version[0] &= 0x0f;
  time_hi_and_version[0] |= 0x40;
  urn += time_hi_and_version.hex();

  urn += "-";

  Data clock_seq_hi_and_reserved = Random::getCryptoRandom(1);
  clock_seq_hi_and_reserved[0] &= 0x3f;
  clock_seq_hi_and_reserved[0] |= 0x40;
  urn += clock_seq_hi_and_reserved.hex();

  urn += getCryptoRandomHex(1); // clock-seq-low
  urn += "-";
  urn += getCryptoRandomHex(6); // node
  return urn;
}

void 
Random::getCryptoRandom(unsigned char* buf, unsigned int numBytes)
{
   resip_assert(numBytes < Random::maxLength+1);

#if USE_OPENSSL
   initialize();
   int e = RAND_bytes( (unsigned char*)buf , numBytes );
   if ( e < 0 )
   {
      // error of some type - likely not enough rendomness to dod this 
      long err = ERR_get_error();
      
      char buf[1024];
      ERR_error_string_n(err,buf,sizeof(buf));
      
      ErrLog( << buf );
      resip_assert(0);
   }
#else
   // !bwc! Should optimize this.
   Data temp=Random::getRandom(numBytes);
   memcpy(buf, temp.data(), numBytes);
#endif
}

#ifdef WIN32
Random::Initializer::Initializer()  : mThreadStorage(::TlsAlloc())
{ 
   resip_assert(mThreadStorage != TLS_OUT_OF_INDEXES);
}

Random::Initializer::~Initializer() 
{ 
   ::TlsFree(mThreadStorage); 
}

void 
Random::Initializer::setInitialized() 
{ 
   ::TlsSetValue(mThreadStorage, (LPVOID) TRUE);
}

bool 
Random::Initializer::isInitialized() 
{ 
   // Note:  if value is not set yet then 0 (false) is returned
   return (BOOL) ::TlsGetValue(mThreadStorage) == TRUE; 
}
#endif


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2005.   All rights reserved.
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
 * vi: set shiftwidth=3 expandtab:
 */
