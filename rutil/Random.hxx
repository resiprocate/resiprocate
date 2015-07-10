#if !defined(RESIP_RANDOM_HXX)
#define RESIP_RANDOM_HXX 

#include "rutil/Mutex.hxx"
#include "rutil/Data.hxx"
#include "rutil/ThreadIf.hxx"     // for ThreadLocalStorage
#include "rutil/ResipAssert.h"
struct random_data;

/**
 * Define below to enable "RtlGenRandom" (aka SystemFunction036) on
 * Windows platform. See Random.cxx for details.
 */
// #define RESIP_RANDOM_WIN32_RTL 1

/**
 * Define below to use common random number generator sate for all resip
 * threads, but private from other parts of app.
 * This makes use of random_r() and friends with mutex protection.
 */
// #define RESIP_RANDOM_THREAD_MUTEX 1

/**
 * Define below to use independent random number generator state
 * for each thread. This makes use of random_r() and friends
 * with TheadIf::tls (thread-local-storage).
 */
// #define RESIP_RANDOM_THREAD_LOCAL 1

/**
 * By default, on POSIX, the standard srandom() and random()
 * functions will be used. This shares the generator state with
 * others libraries running in the same application. Under Linux
 * random() obtains a mutex so is threadsafe.
 * NOTE: See http://evanjones.ca/random-thread-safe.html for some good info.
 * WATCHOUT: Some other library can call srandom() in a stupid way,
 * causing duplicate callids and such.
 */


namespace resip
{

/**
   @brief A static class that wraps the random-number generation code of your
      platform.
   @ingroup crypto
*/
class Random
{
   public:
      static unsigned getSimpleSeed();
      static void initialize();

      enum {maxLength = 512};
      
      static Data getRandom(unsigned int numBytes);
      static Data getRandomHex(unsigned int numBytes); // actual length is 2*numBytes
      static Data getRandomBase64(unsigned int numBytes); // actual length is 1.5*numBytes

      static Data getCryptoRandom(unsigned int numBytes);
      static Data getCryptoRandomHex(unsigned int numBytes); // actual length is 2*numBytes
      static Data getCryptoRandomBase64(unsigned int numBytes); // actual length is 1.5*numBytes

      static void getCryptoRandom(unsigned char* buf, unsigned int numBytes);

      /**
        Returns a version 4 (random) UUID as defined in RFC 4122

        @todo This is something of a suboptimal hack. Ideally, we would
              encapsulate UUID as its own class, with options to create
              any of versions 1 through 5 (and Nil UUIDs). This class
              would have various access methods to pull the result out
              as a bitstring, as raw data, as a URN, etc. For what
              I need to do right now, however, version 4 UUIDs get me
              where I need to go. <abr>
      */
      static Data getVersion4UuidUrn();

      /**
          Returns a postive integer (31 bits) of randomness. Implementation
          is platform dependent.
      **/
      static int  getRandom();
      static int  getCryptoRandom();

      static const char* getImplName();

   private:
      static Mutex mMutex;
      static bool  mIsInitialized;
      
#ifdef WIN32
      // ensure each thread is initialized since windows requires you to call srand for each thread
      ///@internal
      class Initializer
      {
         public:
            Initializer();
            ~Initializer();
            void setInitialized();
            bool isInitialized();

         private:
            DWORD mThreadStorage;
      };
      static Initializer mInitializer;

#ifdef RESIP_RANDOM_WIN32_RTL
      static BOOLEAN (APIENTRY *RtlGenRandom)(void*, ULONG);
#endif
#endif  // WIN32
#ifdef RESIP_RANDOM_THREAD_LOCAL
      static ThreadIf::TlsKey sRandomStateKey;
#endif
#ifdef RESIP_RANDOM_THREAD_MUTEX
      static struct random_data* sRandomState;
      // we re-use the initialization mutex
#endif
};
 
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
