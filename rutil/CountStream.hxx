#if !defined(RESIP_COUNTSTREAM_HXX)
#define RESIP_COUNTSTREAM_HXX 

#include <iostream>
#include "rutil/Data.hxx"

namespace resip
{

/** CountBuffer is used to back the CountStream - it counts the bytes written
    to the stream and does not accumulate the data.
 */
class CountBuffer : public std::streambuf
{
   public:
      /** Constructs a CountBuffer with the reference to the size_t where the
      count should be written after flush is called on the stream.
       */
      CountBuffer(size_t& count);
      virtual ~CountBuffer();

   protected:
      virtual int sync();
      virtual int overflow(int c = -1);

   private:
      size_t& mCount;
};

/** CountStream is used to count the amount of data written to the stream.  The
    actual data written to the stream is not accumulated anywhere.  It follows
    the general pattern of DataStream in sip/resiprocate/os where the
    data is accumulated into the reference passed to the constructor.  The data
    is valid after CountStream's destructor is called.
*/
class CountStream : private CountBuffer, public std::ostream
{
   public:
      /** Constructs a CountStream with a reference to where the count of bytes
      	written to the stream should be stored.
        @param count A reference to the size_t where the number of bytes written
        to the stream after the stream goes out of scope.
       */
      CountStream(size_t& count);
      /** Calls flush on itself to force the update to the count reference
        passed into the constructor.
       */
      ~CountStream();

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
