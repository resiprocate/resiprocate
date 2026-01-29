#if !defined(RESIP_MD5STREAM_HXX)
#define RESIP_MD5STREAM_HXX 

#include <iostream>
#include "rutil/compat.hxx"
#include "rutil/Data.hxx"
#include "rutil/vmd5.hxx"

#pragma warning(1: 4996) // Move deprecated warnings back to level 1

/**
 * @file  MD5Stream.hxx
 * @brief DEPRECATED - Use DigestStream.hxx instead.
 * * @deprecated This class is being phased out in favor of the unified DigestStream
 * architecture. MD5Stream is now a legacy wrapper and may be
 * removed in future releases.
 * * ### Transition Guide
 * The new DigestStream class replaces specialized stream types with a
 * parameter-based approach, providing support for
 * modern hash algorithms (SHA-256, SHA-512, SHA-512/256).
 * * #### Old Usage:
 * @code
 * resip::MD5Stream md5;
 * md5 << "data";
 * resip::Data hash = md5.getHex();
 * @endcode
 * * #### New Usage:
 * @code
 * #include "rutil/DigestStream.hxx"
 * // Defaults to MD5, or specify DigestBuffer::MD5 explicitly
 * resip::DigestStream ds(resip::DigestBuffer::MD5);
 * ds << "data";
 * resip::Data hash = ds.getHex();
 * @endcode
 */

namespace resip
{

/** 
   @brief Implementation of std::streambuf used to back the MD5Stream.
 */
class MD5Buffer : public std::streambuf
{
   public:
      MD5Buffer();
      virtual ~MD5Buffer();
      /** @returns the MD5 hexadecimal representation of the data from the buffer
       */
      Data getHex();
      Data getBin();
      size_t bytesTaken();
   protected:
      virtual int sync();
      virtual int overflow(int c = -1);
   private:
      char mBuf[64];
      MD5Context mContext;
      size_t mLen;
};

/** 
   @brief Used to accumlate data written to the stream in a MD5Buffer and
    convert the data to MD5.
 */
class MD5Stream : private MD5Buffer, public std::ostream
{
   public:
      RESIP_DEPRECATED(MD5Stream());
      ~MD5Stream();
      /** Calls flush() on itself and returns the MD5 data in hex format.
          @returns the MD5 hexadecimal representation of the data written to the
          stream and convert the data to MD5.
       */
      Data getHex();
      Data getBin();
      size_t bytesTaken();
   private:
      //MD5Buffer mStreambuf;
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
