#if !defined(RESIP_PARSEBUFFER_HXX)
#define RESIP_PARSEBUFFER_HXX 

#include "rutil/Data.hxx"
#include "rutil/ParseException.hxx"

namespace resip
{

/**
   @brief Provides string-parsing functionality with protection from buffer
      overflow.

   Throws ParseException when parse failures occur.
   @ingroup text_proc
*/
class ParseBuffer
{
   public:
      // does NOT OWN the buffer memory
      ParseBuffer(const char* buff, size_t len, 
                  const Data& errorContext = Data::Empty);

      explicit ParseBuffer(const char* buff, 
                           const Data& errorContext = Data::Empty);

      explicit ParseBuffer(const Data& data,
                           const Data& errorContext = Data::Empty);

      ParseBuffer(const ParseBuffer& other);

///@cond
      // .bwc. Backwards-compatibility hack; ParseException used to be
      // a full inner class of ParseBuffer, and we also had a separate 
      // ParseException class that other things used. For consistency, we have
      // moved everything over to use ParseException, but there is app-code
      // out there that still expects ParseException to exist.
      typedef ParseException Exception;
///@endcond

   private:
      /**
         @internal
         @brief Provides some access to the current position of a ParseBuffer, 
         which guards against invalid accesses. Just a wrapper around a 
         ParseBuffer&, so is very cheap to initialize/copy.
      */
      class CurrentPosition
      {
         public:
            inline explicit CurrentPosition(const ParseBuffer& pb) :
               mPb(pb)
            {}

            operator const char*() const
            {
               return mPb.mPosition;
            }

            const char& operator*() const
            {
               mPb.assertNotEof();
               return *mPb.mPosition;
            }

            const ParseBuffer& mPb;
      };

      /**
         @internal
         @brief Similar to CurrentPosition, but does not move depending on the 
         state of the ParseBuffer. Initializing one of these is more expensive.
      */
      class Pointer
      {
         public:
            Pointer(const ParseBuffer& pb,
                    const char* position,
                    bool atEof);
            Pointer(const CurrentPosition& pos);

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
      const Data& getContext() const {return mErrorContext;}
      
      // allow the buffer to be rolled back
      ParseBuffer& operator=(const ParseBuffer& other);
      void reset(const char* pos)
      {
         resip_assert( mBuff <= mEnd);
         resip_assert( (pos >= mBuff) && (pos <= mEnd) );
         mPosition = pos;
      }

      // abcdef
      // ^     ^
      // begin end
      bool eof() const { return mPosition >= mEnd;}
      bool bof() const { return mPosition <= mBuff;}
      bool valid() const {return (!eof()) && (!bof());}
      Pointer start() const { return Pointer(*this, mBuff, eof()); }
      CurrentPosition position() const { return CurrentPosition(*this); }
      Pointer end() const { return Pointer(*this, mEnd, true); }
      size_t lengthRemaining() { return mEnd - mPosition; }

      CurrentPosition skipChar()
      {
         if (eof())
         {
            fail(__FILE__, __LINE__,"skipped over eof");
         }
         ++mPosition;
         return CurrentPosition(*this);
      }

      CurrentPosition skipChar(char c);
      CurrentPosition skipChars(const char* cs);
      CurrentPosition skipChars(const Data& cs);
      CurrentPosition skipNonWhitespace();
      CurrentPosition skipWhitespace();
      CurrentPosition skipLWS();
      CurrentPosition skipToTermCRLF();
      CurrentPosition skipToChar(char c)
      {
         mPosition = (const char*)memchr(mPosition, c, mEnd-mPosition);
         if(!mPosition)
         {
            mPosition=mEnd;
         }
         return CurrentPosition(*this);
      }
      CurrentPosition skipToChars(const char* cs);
      CurrentPosition skipToChars(const Data& cs); // ?dlb? case sensitivity arg?
      CurrentPosition skipToOneOf(const char* cs);
      CurrentPosition skipToOneOf(const char* cs1, const char* cs2);
      CurrentPosition skipToOneOf(const Data& cs);
      CurrentPosition skipToOneOf(const Data& cs1, const Data& cs2);

      // std::bitset based parse function
      CurrentPosition skipChars(const std::bitset<256>& cs)
      {
         while (mPosition < mEnd)
         {
            if (cs.test((unsigned char)(*mPosition)))
            {
               mPosition++;
            }
            else
            {
               return CurrentPosition(*this);
            }
         }
         return CurrentPosition(*this);
      }

      CurrentPosition skipToOneOf(const std::bitset<256>& cs)
      {
         while (mPosition < mEnd)
         {
            if (cs.test((unsigned char)(*mPosition)))
            {
               return CurrentPosition(*this);
            }
            else
            {
               mPosition++;
            }
         }
         return CurrentPosition(*this);
      }

      const char* skipToEndQuote(char quote = '"');
      CurrentPosition skipN(int count)
      {
         mPosition += count;
         if (mPosition > mEnd)
         {
            fail(__FILE__, __LINE__, "skipped eof");
         }
         return CurrentPosition(*this);
      }

      CurrentPosition skipToEnd()
      {
         mPosition = mEnd;
         return CurrentPosition(*this);
      }

      // inverse of skipChar() -- end up at char not before it
      const char* skipBackChar();
      const char* skipBackWhitespace();
      const char* skipBackN(int count)
      {
         mPosition -= count;
         if (bof())
         { 
           fail(__FILE__, __LINE__,"backed over beginning of buffer");
         }
         return mPosition;
      }

      const char* skipBackChar(char c);
      const char* skipBackToChar(char c);
      const char* skipBackToOneOf(const char* cs);

      void assertEof() const
      {
         if (!eof())
         {
            fail(__FILE__, __LINE__,"expected eof");
         }
      }

      void assertNotEof() const
      {
         if (eof())
         {
            fail(__FILE__, __LINE__,"unexpected eof");
         }
      }

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

      RESIP_DEPRECATED(UInt64 unsignedLongLong()){return uInt64();} 
      RESIP_DEPRECATED(unsigned long unsignedInteger()){return uInt32();}

#ifndef RESIP_FIXED_POINT		
      float floatVal();
#endif
      int qVal();

      static bool oneOf(char c, const char* cs);
      static bool oneOf(char c, const Data& cs);
      static const char* Whitespace;
      static const char* ParamTerm;
   private:
      friend class ParseBuffer::CurrentPosition;
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
