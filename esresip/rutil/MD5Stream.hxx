/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if !defined(RESIP_MD5STREAM_HXX)
#define RESIP_MD5STREAM_HXX 

#include <iostream>
#include "rutil/Data.hxx"

namespace resip
{
class MD5Context;
class SSLMD5Context;

/** 
   @brief Implementation of std::streambuf used to back the MD5Stream.
   @ingroup crypto
 */
class MD5Buffer : public std::streambuf
{
   public:
      MD5Buffer();
      virtual ~MD5Buffer();
      /** @return the MD5 hexadecimal representation of the data from the buffer
       */
      Data getHex();
      Data getBin();
   protected:
      virtual int sync();
      virtual int overflow(int c = -1);
   private:
      char mBuf[64];
      union
      {
         // We do different things here, depending on whether SSL was enabled.
         // Use a union, since we don't want to put any #ifdef USE_SSL crud in
         // here. Which way we use this union is determined in the cxx file by
         // using the USE_SSL preprocessor definition (ie; at compile time).
         // If you write code that tries to use this union, you are on your own.
         MD5Context* mContext;
         SSLMD5Context* mSSLContext;
      };
};

/** 
   @brief Used to accumulate data written to the stream in a MD5Buffer and
    convert the data to MD5.
   @ingroup crypto
 */
class MD5Stream : private MD5Buffer, public std::ostream
{
   public:
      MD5Stream();
      ~MD5Stream();
      /** Calls flush() on itself and returns the MD5 data in hex format.
          @return the MD5 hexadecimal representation of the data written to the
          stream and convert the data to MD5.
          @example
          @code
           MD5Stream str; 
           Data data ;
           int i = 1000;
           cerr << i << endl;
           str << i;
           data += Data(i);
           assert(str.getHex() == data.md5());
          @endcode
       */
      Data getHex();
      Data getBin();
   private:
      //MD5Buffer mStreambuf;
};

}

#endif
/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
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
