#include <cassert>

#include "sip2/util/Logger.hxx"
#include "sip2/util/ParseBuffer.hxx"

using namespace Vocal2;

#define VOCAL_SUBSYSTEM Subsystem::SIP

const char* ParseBuffer::ParamTerm = ";?"; // maybe include "@>,"?
const char* ParseBuffer::Whitespace = " \t\r\n";

ParseBuffer::ParseBuffer(const ParseBuffer& rhs)
   : mBuff(rhs.mBuff),
     mTraversalPtr(rhs.mTraversalPtr),
     mEnd(rhs.mEnd)
{}

ParseBuffer& 
ParseBuffer::operator=(const ParseBuffer& rhs)
{
   mBuff = rhs.mBuff;
   mTraversalPtr = rhs.mTraversalPtr;
   mEnd = rhs.mEnd;

   return *this;
}

// !dlb! replace with copy and assign
void
ParseBuffer::reset(const char* pos) //should be renamed to set
{
   assert(pos >= mBuff && pos <= mEnd);
   mTraversalPtr = pos;
}

const char*
ParseBuffer::skipChar(char c)
{
   if (*position() != c)
   {
      DebugLog (<< "Expected '" << c << "'");
      throw Exception("parse error", __FILE__, __LINE__);
   }
   return ++mTraversalPtr;
}

const char* 
ParseBuffer::skipChars(const char* cs)
{
   const char* match = cs;
   while (*match != 0)
   {
      if (eof() || (*match != *mTraversalPtr))
      {
         DebugLog (<< "Expected \"" << cs << "\"");
         throw Exception("parse error", __FILE__, __LINE__);
      }
      match++;
      mTraversalPtr++;
   }
   return mTraversalPtr;
}

const char* 
ParseBuffer::skipChars(const Data& cs)
{
   const char* match = cs.data();
   for(Data::size_type i = 0; i < cs.size(); i++)
   {
      if (eof() || (*match != *mTraversalPtr))
      {
         DebugLog (<< "Expected \"" << cs << "\"");
         throw Exception("parse error", __FILE__, __LINE__);
      }
      match++;
      mTraversalPtr++;
   }
   return mTraversalPtr;
}

const char* 
ParseBuffer::skipNonWhitespace()
{
   while (mTraversalPtr < mEnd)
   {
      switch (*position())
      {
         case ' ' :
         case '\t' : 
         case '\r' : 
         case '\n' : 
            return position();
         default : 
            mTraversalPtr++;
      }
   }
   return position();
}

const char* 
ParseBuffer::skipWhitespace()
{
   while (mTraversalPtr < mEnd)
   {
      switch (*position())
      {
         case ' ' :
         case '\t' : 
         case '\r' : 
         case '\n' : 
         {
            mTraversalPtr++;
            break;
         }
         default : 
            return position();
      }
   }
   return position();
}

// "SIP header field values can be folded onto multiple lines if the
//  continuation line begins with a space or horizontal tab"
const char* 
ParseBuffer::skipLWS()
{
   enum State {WS, CR, LF};
   State state = WS;
   while (mTraversalPtr < mEnd)
   {
      switch (*mTraversalPtr++)
      {
         case ' ' :
         case '\t' : 
         {
            state = WS;
            break;
         }
         case '\r' : 
         {
            state = CR;
            break;
         }
         case '\n' : 
         {
            if (state == CR)
            {
               state = LF;
            }
            else
            {
               state = WS;
            }
            break;
         }
         default : 
         {
            // terminating CRLF not skipped
            if (state == LF)
            {
               mTraversalPtr -= 3;
            }
            else
            {
               mTraversalPtr--;
            }
            return mTraversalPtr;
         }
      }
   }
   return mTraversalPtr;
}

const char*
ParseBuffer::skipToTermCRLF()
{
   while (true)
   {
      static Data CRLF("\r\n");
      skipToChars(CRLF);
      mTraversalPtr += 2;
      if (*mTraversalPtr != ' ' &&
          *mTraversalPtr != '\t')
      {
         mTraversalPtr -= 2;
         break;
      }
   }
   return mTraversalPtr;
}

const char* 
ParseBuffer::skipToChar(char c)
{
   while (mTraversalPtr < mEnd)
   {
      if (*position() == c)
      {
         return position();
      }
      else
      {
         mTraversalPtr++;
      }
   }
   return position();
}

const char*
ParseBuffer::skipToChars(const char* cs)
{
   assert(cs);
   if (*cs == 0)
   {
      return mTraversalPtr;
   }

   const char* pos;
   const char* rpos;

   while (mTraversalPtr < mEnd)
   {
      rpos = mTraversalPtr;
      pos = cs;
      while (*pos++ == *mTraversalPtr++)
      {
         if (*pos == 0)
         {
            reset(rpos);
            return rpos;
         }
      }
   }
   return mTraversalPtr;
}

const char*
ParseBuffer::skipToChars(const Data& cs)
{
   if (cs.empty())
   {
      return mTraversalPtr;
   }

   const char* rpos;
   while (mTraversalPtr < mEnd)
   {
      rpos = mTraversalPtr;
      for (size_t i = 0; i < cs.size(); i++)
      {
         if (cs[i] != *mTraversalPtr++)
         {
            goto skip;
         }
      }
      reset(rpos);
      return rpos;
     skip: ;
   }
   return mTraversalPtr;
}

bool oneOf(char c, const char* cs)
{
   while (*cs)
   {
      if (c == *(cs++))
      {
         return true;
      }
   }
   return false;
}

bool oneOf(char c, const Data& cs)
{
   for (Data::size_type i = 0; i < cs.size(); i++)
   {
      if (c == cs[i])
      {
         return true;
      }
   }
   return false;
}

const char* 
ParseBuffer::skipToOneOf(const char* cs)
{
   while (mTraversalPtr < mEnd)
   {
      if (oneOf(*position(), cs))
      {
         return position();
      }
      else
      {
         mTraversalPtr++;
      }
   }
   return position();
}

const char* 
ParseBuffer::skipToOneOf(const char* cs1,
                         const char* cs2)
{
   while (mTraversalPtr < mEnd)
   {
      if (oneOf(*position(), cs1) ||
          oneOf(*position(), cs2))
      {
         return position();
      }
      else
      {
         mTraversalPtr++;
      }
   }
   return position();
}

const char* 
ParseBuffer::skipToOneOf(const Data& cs1,
                         const Data& cs2)
{
   while (mTraversalPtr < mEnd)
   {
      if (oneOf(*position(), cs1) ||
          oneOf(*position(), cs2))
      {
         return position();
      }
      else
      {
         mTraversalPtr++;
      }
   }
   return position();
}

const char* 
ParseBuffer::skipToEndQuote(char quote)
{
   while (mTraversalPtr < mEnd)
   {
      // !dlb! mark character encoding
      if (*position() == '\\')
      {
         mTraversalPtr += 2;
      }
      else if (*position() == quote)
      {
         return position();
      }
      else
      {
         mTraversalPtr++;
      }
   }

   DebugLog (<< "Missing '" << quote);
   throw Exception("Missing quote", __FILE__,__LINE__);
}

const char* 
ParseBuffer::skipN(int count)
{
   mTraversalPtr += count;
   if (mTraversalPtr > mEnd)
   {
      mTraversalPtr = mEnd;
   }
   return mTraversalPtr;
}

const char* 
ParseBuffer::skipToEnd()
{
   mTraversalPtr = mEnd;
   return mTraversalPtr;
}

void
ParseBuffer::data(Data& data, const char* start) const
{
   assert( mBuff <= start );
   assert( start <= mTraversalPtr);

   if (data.mMine)
   {
      delete[] data.mBuf;
   }
   data.mSize = (unsigned int)(mTraversalPtr - start);
   data.mBuf = const_cast<char*>(start);
   data.mCapacity = data.mSize;
   data.mMine = false;
}

Data
ParseBuffer::data(const char* start) const
{
   assert(mBuff <= start && start <= mTraversalPtr);

   Data data(start, mTraversalPtr - start);
   return data;
}

int
ParseBuffer::integer()
{
   if ( this->eof() )
   {
      DebugLog(<< "Expected a digit, got eof ");
      throw Exception("Expected a digit, got eof", __FILE__, __LINE__);
   }

   const char* p = position();
   assert( p );
   char c = *p;

   if (!isdigit(c))
   {
      DebugLog(<< "Expected a digit, got: " << Data(position(), (mEnd - position())));
      throw Exception("Expected a digit", __FILE__, __LINE__);
   }
   
   int num = 0;
   while (!eof() && isdigit(*position()))
   {
      num = num*10 + (*position()-'0');
      skipChar();
   }
   
   return num;
}

float
ParseBuffer::floatVal()
{
   const char* s = position();
   try
   {
      int num = integer();
      // !dlb! a little too fussy -- can't parse "1 "
      skipChar('.');
      const char* pos = position();
      float mant = float(integer());
      int s = position() - pos;
      while (s--)
      {
         mant /= 10.0;
      }
      return num + mant;
   }
   catch (Exception&)
   {
      DebugLog(<< "Expected a floating point value, got: " << Data(s, position() - s));
      throw Exception("Expected a floating point value", __FILE__, __LINE__);
   }
}

void
ParseBuffer::assertEof()
{
   if (!eof())
   {
      throw Exception("Expected eof", __FILE__, __LINE__);
   }      
}

void
ParseBuffer::assertNotEof()
{
   if (eof())
   {
      throw Exception("Expected not eof", __FILE__, __LINE__);
   }      
}

void
ParseBuffer::fail(const Data& message)
{
   throw Exception(message, __FILE__, __LINE__);
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
