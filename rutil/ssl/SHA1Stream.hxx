#if !defined(RESIP_SHA1STREAM_HXX)
#define RESIP_SHA1STREAM_HXX 

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include <iostream>
#include <memory>
#include <vector>
#include "rutil/Data.hxx"

// This will not be compiled or installed if USE_SSL isn't set. If you are 
// including this file from a source tree, and you are getting link errors, you 
// are probably trying to link against libs that were built without SSL support. 
// Either stop trying to use this file, or re-build the libs with ssl support 
// enabled.
//#if defined (USE_SSL)
//# include "openssl/sha.h"
//#else
//// !kh!
//// so it would compile without openssl.
//// also see my comment below.
//typedef int SHA_CTX;
//#endif // USE_SSL

# include "openssl/sha.h"

namespace resip
{

/** 
   @brief An implementation of std::streambuf used to back the SHA1Stream.
 */
class SHA1Buffer : public std::streambuf
{
   public:
      SHA1Buffer();
      virtual ~SHA1Buffer();
      /** @returns the SHA1 hexadecimal representation of the data from the buffer
       */
      Data getHex();
      /** 
          @param bits the lowest order bits in network byte order. bits must be
          multiple of 8
          @returns the SHA1 binary representation of the data from the buffer
       */
      Data getBin(unsigned int bits);

   protected:
      virtual int sync();
      virtual int overflow(int c = -1);
   private:
      // !kh!
      // used pointers to keep the same object layout.
      // this adds overhead, two additional new/delete.
      // could get rid of the overhead if, sizeof(SHA_CTX) and SHA_DIGEST_LENGTH are known and FIXED.
      // could use pimpl to get rid of one new/delete pair.
      std::auto_ptr<SHA_CTX> mContext;
      std::vector<char> mBuf;
      bool mBlown;
};

/** 
   @brief Used to accumlate data written to the stream in a SHA1Buffer and
    convert the data to SHA1.
 */
class SHA1Stream : private SHA1Buffer, public std::ostream
{
   public:
      SHA1Stream();
      ~SHA1Stream();
      /** Calls flush() on itself and returns the SHA1 data in hex format.
          @returns the SHA1 hexadecimal representation of the data written to the stream
       */
      Data getHex();
      /** Calls flush() on itself and returns the SHA1 data in binary format.
          @param bits the lowest order bits in network byte order. bits must be
          multiple of 8
          @returns the SHA1 binary representation of the data written to the stream
       */
      Data getBin(unsigned int bits=160);

      /** Calls getBin(32) and converts to a UInt32 */
      UInt32 getUInt32();
      
};

}

#endif
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
