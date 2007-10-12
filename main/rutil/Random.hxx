#if !defined(RESIP_RANDOM_HXX)
#define RESIP_RANDOM_HXX 

#include "rutil/Mutex.hxx"
#include "rutil/Data.hxx"
#include <cassert>

namespace resip
{

/**
   @brief A static class that wraps the random-number generation code of your
      platform.
*/
class Random
{
   public:
      static void initialize();

      enum {maxLength = 512};
      
      static Data getRandom(unsigned int numBytes);
      static Data getRandomHex(unsigned int numBytes); // actual length is 2*numBytes
      static Data getRandomBase64(unsigned int numBytes); // actual length is 1.5*numBytes

      static Data getCryptoRandom(unsigned int numBytes);
      static Data getCryptoRandomHex(unsigned int numBytes); // actual length is 2*numBytes
      static Data getCryptoRandomBase64(unsigned int numBytes); // actual length is 1.5*numBytes

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

      static int  getRandom();
      static int  getCryptoRandom();

   private:
      static Mutex mMutex;
      static bool  mIsInitialized;
      
#ifdef WIN32
      // ensure each thread is initialized since windows requires you to call srand for each thread
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
 */
