#ifndef RESIP_DataStream_hxx
#define RESIP_DataStream_hxx

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <iostream>
#include "resipfaststreams.hxx"

namespace resip
{

class Data;

/** @brief Implementation of std::streambuf used to back the DataStream, 
   iDataStream, and oDataStream.
 */
class DataBuffer : 
#ifdef RESIP_USE_STL_STREAMS
   public std::streambuf
#else
   public ResipStreamBuf
#endif
{
   public:
      DataBuffer(Data& str);
      virtual ~DataBuffer();

   protected:
#ifdef RESIP_USE_STL_STREAMS
      virtual int sync();
      virtual int overflow(int c = -1);
#else
      virtual size_t writebuf(const char *s, size_t count);
      virtual size_t readbuf(char *buf, size_t count);
      virtual size_t putbuf(char ch);
      virtual void flushbuf(void)
      {}
      virtual UInt64 tellpbuf(void);
#endif
      Data& mStr;
   private:
      DataBuffer(const DataBuffer&);
      DataBuffer& operator=(const DataBuffer&);
};

/**
   @brief An iostream that operates on an existing Data.

   The data written to the stream is appended to the reference passed to the
   constructor.  The data is valid after DataStream's destructor is called. 
   Data read from the stream is read from the reference passed to the
   constructor.
 */
class DataStream : private DataBuffer
#ifdef  RESIP_USE_STL_STREAMS
, public std::iostream
#else
, public ResipFastOStream
#endif

{
   public:
      /** Constructs a DataStream with a Data to operate on.
          @param str A Data to operate on.
       */
      DataStream(Data& str);
      /** Calls flush on itself to force the update to the Data reference
        passed into the constructor.
       */
      ~DataStream();

   private:
      DataStream(const DataStream&);
      DataStream& operator=(const DataStream&);
};

/**
   @brief An istream that operates on an existing Data.
 */
class iDataStream : private DataBuffer, public DecodeStream
{
   public:
      /** Constructs a iDataStream with a Data to operate on.
          @param str A Data to operate on.
       */
      iDataStream(Data& str);
      ~iDataStream();
      
   private:
      iDataStream(const iDataStream&);
      iDataStream& operator=(const iDataStream&);

};

/**
   @brief An ostream that operates on an existing Data.

   The data is appended to the reference passed to the constructor.  The data is 
   valid after DataStream's destructor is called.
 */
class oDataStream : private DataBuffer, public EncodeStream 
{
   public:
      /** Constructs a oDataStream with a Data to operate on.
          @param str A Data to operate on.
       */
      oDataStream(Data& str);
      /** Calls flush on itself to force the update to the Data reference
        passed into the constructor.
       */
      ~oDataStream();

      /** Clears the underlying Data reference. */
      void reset();

   private:
      oDataStream(const oDataStream&);
      oDataStream& operator=(const oDataStream&);
};
}
#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005
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
