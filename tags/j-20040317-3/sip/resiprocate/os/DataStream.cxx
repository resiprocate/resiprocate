#include "resiprocate/os/DataStream.hxx"
#include "resiprocate/os/Data.hxx"

using namespace resip;

DataBuffer::DataBuffer(Data& str)
   : mStr(str)
{
   char* gbuf = const_cast<char*>(mStr.mBuf);
   setg(gbuf, gbuf, gbuf+mStr.size());
   // expose the excess capacity as the put buffer
   setp(gbuf+mStr.mSize, gbuf+mStr.mCapacity);
}

DataBuffer::~DataBuffer()
{
}

int
DataBuffer::sync()
{
   size_t len = pptr() - pbase();
   if (len > 0) 
   {
      size_t pos = gptr() - eback();  // remember the get position
      mStr.mSize += len;
      char* gbuf = const_cast<char*>(mStr.data());
      // reset the get buffer
      setg(gbuf, gbuf+pos, gbuf+mStr.size()); 
      // reset the put buffer
      setp(gbuf + mStr.mSize, gbuf + mStr.mCapacity);
   }
   return 0;
}

int
DataBuffer::overflow(int c)
{
   // sync, but reallocate
   size_t len = pptr() - pbase();
   if (len > 0) 
   {
      size_t pos = gptr() - eback();  // remember the get position

      // update the length
      mStr.mSize += len;

      // resize the underlying Data and reset the input buffer
      mStr.resize(((mStr.mCapacity+16)*3)/2, true);

      char* gbuf = const_cast<char*>(mStr.mBuf);
      // reset the get buffer
      setg(gbuf, gbuf+pos, gbuf+mStr.mSize); 
      // reset the put buffer
      setp(gbuf + mStr.mSize, gbuf + mStr.mCapacity);
   }
   if (c != -1) 
   {
      mStr.mBuf[mStr.mSize] = c;
      pbump(1);
      return c;
   }
   return 0;
}

iDataStream::iDataStream(Data& str)
   : std::istream(0),
     mStreambuf(str)
{
   init(&mStreambuf);
}

iDataStream::~iDataStream()
{
}

oDataStream::oDataStream(Data& str)
   : std::ostream(0),
     mStreambuf(str)
{
   init(&mStreambuf);
}

oDataStream::~oDataStream()
{
   flush();
}

DataStream::DataStream(Data& str)
   : std::iostream(0),
     mStreambuf(str)
{
   init(&mStreambuf);
}

DataStream::~DataStream()
{
   flush();
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
