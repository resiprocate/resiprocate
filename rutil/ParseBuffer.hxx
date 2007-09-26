#if !defined(RESIP_PARSEBUFFER_HXX)
#define RESIP_PARSEBUFFER_HXX 

#include "rutil/Data.hxx"
#include "rutil/ParseException.hxx"

namespace resip
{

class ParseBuffer
{
   public:
      // does NOT OWN the buffer memory
      ParseBuffer(const char* buff, unsigned int len, 
                  const Data& errorContext = Data::Empty);

      explicit ParseBuffer(const Data& data,
                           const Data& errorContext = Data::Empty);

      ParseBuffer(const ParseBuffer& other);

      // .bwc. Backwards-compatibility hack; ParseBuffer::Exception used to be
      // a full inner class of ParseBuffer, and we also had a separate 
      // ParseException class that other things used. For consistency, we have
      // moved everything over to use ParseException, but there is app-code
      // out there that still expects ParseBuffer::Exception to exist.
      typedef ParseException Exception;

   private:
      class Pointer
      {
         public:
            Pointer(const ParseBuffer& pb,
                    const char* position,
                    bool atEof);

            operator const char*() const
            {
               return mPosition;
            }

            const char& operator*() const;
         private:
            const ParseBuffer& mPb;
            const char* mPosition;
            const bool mIsValid;
            static const Data msg;
      };

   public:
      const Data& getContext() const;
      
      // allow the buffer to be rolled back
      ParseBuffer& operator=(const ParseBuffer& other);
      void reset(const char* pos);

      // abcdef
      // ^     ^
      // begin end
      bool eof() const { return mPosition >= mEnd;}
      bool bof() const { return mPosition <= mBuff;}
      Pointer start() const { return Pointer(*this, mBuff, eof()); }
      Pointer position() const { return Pointer(*this, mPosition, eof()); }
      Pointer end() const { return Pointer(*this, mEnd, true); }

      Pointer skipChar();
      Pointer skipChar(char c);
      Pointer skipChars(const char* cs);
      Pointer skipChars(const Data& cs);
      Pointer skipNonWhitespace();
      Pointer skipWhitespace();
      Pointer skipLWS();
      Pointer skipToTermCRLF();
      Pointer skipToChar(char c);
      Pointer skipToChars(const char* cs);
      Pointer skipToChars(const Data& cs); // ?dlb? case sensitivity arg?
      Pointer skipToOneOf(const char* cs);
      Pointer skipToOneOf(const char* cs1, const char* cs2);
      Pointer skipToOneOf(const Data& cs);
      Pointer skipToOneOf(const Data& cs1, const Data& cs2);
      const char* skipToEndQuote(char quote = '"');
      Pointer skipN(int count);
      Pointer skipToEnd();

      // inverse of skipChar() -- end up at char not before it
      const char* skipBackChar();
      const char* skipBackWhitespace();
      const char* skipBackN(int count);
      const char* skipBackChar(char c);
      const char* skipBackToChar(char c);

      void assertEof() const;
      void assertNotEof() const;
      void fail(const char* file, unsigned int line,
                const Data& errmsg = Data::Empty) const;

      /// make the passed in data share memory with the buffer (uses Data::Share)
      void data(Data& data, const char* start) const;

      Data data(const char* start) const;

      void dataUnescaped(Data& data, const char* start) const;      
      
      int integer();

      
      UInt8 uInt8();
      UInt32 uInt32();
      UInt64 uInt64();

#ifndef WIN32
      __attribute__ ((deprecated)) UInt64 unsignedLongLong(){return uInt64();} 
      __attribute__ ((deprecated)) unsigned long unsignedInteger(){return uInt32();}
#endif

#ifndef RESIP_FIXED_POINT		
      float floatVal();
#endif
      int qVal();
      static const char* Whitespace;
      static const char* ParamTerm;
   private:
      const char* mBuff;
      const char* mPosition;
      const char* mEnd;
      const Data& mErrorContext;
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
