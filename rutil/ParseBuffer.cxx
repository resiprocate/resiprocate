#include "rutil/ResipAssert.h"

#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/ParseException.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

const char* ParseBuffer::ParamTerm = ";?"; // maybe include "@>,"?
const char* ParseBuffer::Whitespace = " \t\r\n";
const Data ParseBuffer::Pointer::msg("dereferenced ParseBuffer eof");

ParseBuffer::ParseBuffer(const char* buff, size_t len, 
                         const Data& errorContext)
   : mBuff(buff),
     mPosition(buff),
     mEnd(buff+len),
     mErrorContext(errorContext)
{}

ParseBuffer::ParseBuffer(const char* buff, 
                         const Data& errorContext)
   : mBuff(buff),
     mPosition(buff),
     mEnd(buff+strlen(buff)),
     mErrorContext(errorContext)
{}

ParseBuffer::ParseBuffer(const Data& data,
            const Data& errorContext)
   : mBuff(data.data()),
     mPosition(mBuff),
     mEnd(mBuff + data.size()),
     mErrorContext(errorContext)
{}

ParseBuffer::ParseBuffer(const ParseBuffer& rhs)
   : mBuff(rhs.mBuff),
     mPosition(rhs.mPosition),
     mEnd(rhs.mEnd),
     mErrorContext(rhs.mErrorContext)
{}

ParseBuffer& 
ParseBuffer::operator=(const ParseBuffer& rhs)
{
   mBuff = rhs.mBuff;
   mPosition = rhs.mPosition;
   mEnd = rhs.mEnd;

   return *this;
}

ParseBuffer::CurrentPosition
ParseBuffer::skipChar(char c)
{
   if (eof())
   {
      fail(__FILE__, __LINE__,"skipped over eof");
   }
   if (*mPosition != c)
   {
      Data msg("expected '");
      msg += c;
      msg += "'";
      fail(__FILE__, __LINE__,msg);
   }
   ++mPosition;
   return CurrentPosition(*this);
}

ParseBuffer::CurrentPosition
ParseBuffer::skipChars(const char* cs)
{
   const char* match = cs;
   while (*match != 0)
   {
      if (eof() || (*match != *mPosition))
      {
          Data msg("Expected \"");
          msg += cs;
          msg +=  "\"";
         fail(__FILE__, __LINE__,msg);
      }
      match++;
      mPosition++;
   }
   return CurrentPosition(*this);
}

ParseBuffer::CurrentPosition
ParseBuffer::skipChars(const Data& cs)
{
   const char* match = cs.data();
   for(Data::size_type i = 0; i < cs.size(); i++)
   {
      if (eof() || (*match != *mPosition))
      {
          Data msg( "Expected \"");
          msg += cs;
          msg += "\"";
         fail(__FILE__, __LINE__,msg);
      }
      match++;
      mPosition++;
   }
   return CurrentPosition(*this);
}

ParseBuffer::CurrentPosition
ParseBuffer::skipNonWhitespace()
{
   assertNotEof();
   while (mPosition < mEnd)
   {
      switch (*mPosition)
      {
         case ' ' :
         case '\t' : 
         case '\r' : 
         case '\n' : 
            return CurrentPosition(*this);
         default : 
            mPosition++;
      }
   }
   return CurrentPosition(*this);
}

ParseBuffer::CurrentPosition
ParseBuffer::skipWhitespace()
{
   while (mPosition < mEnd)
   {
      switch (*mPosition)
      {
         case ' ' :
         case '\t' : 
         case '\r' : 
         case '\n' : 
         {
            mPosition++;
            break;
         }
         default : 
            return CurrentPosition(*this);
      }
   }
   return CurrentPosition(*this);
}

// "SIP header field values can be folded onto multiple lines if the
//  continuation line begins with a space or horizontal tab"

// CR can be quote with \ within "" and comments -- treat \CR as whitespace
ParseBuffer::CurrentPosition
ParseBuffer::skipLWS()
{
   enum State {WS, CR, LF};
   State state = WS;
   while (mPosition < mEnd)
   {
      char c = *mPosition++;
      if (c == '\\')
      {
         // treat escaped CR and LF as space
         c = *mPosition++;
         if (c == '\r' || c == '\n')
         {
            c = ' ';
         }
      }
      switch (*mPosition++)
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
               mPosition -= 3;
            }
            else
            {
               mPosition--;
            }
            return CurrentPosition(*this);
         }
      }
   }
   return CurrentPosition(*this);
}

static Data CRLF("\r\n");
ParseBuffer::CurrentPosition
ParseBuffer::skipToTermCRLF()
{
   while (mPosition < mEnd)
   {
      skipToChars(CRLF);
      mPosition += 2;
      if ((*mPosition != ' ' &&
           *mPosition != '\t' &&
           // check for \CRLF -- not terminating
           //           \\CRLF -- terminating
           ((mPosition-3 < mBuff || *(mPosition-3) != '\\') ||
            (mPosition-4 > mBuff && *(mPosition-4) == '\\'))))
      {
         mPosition -= 2;
         return CurrentPosition(*this);
      }
   }
   return CurrentPosition(*this);
}

ParseBuffer::CurrentPosition
ParseBuffer::skipToChars(const char* cs)
{
   resip_assert(cs);
   unsigned int l = (unsigned int)strlen(cs);

   const char* rpos;
   const char* cpos;
   while (mPosition < mEnd)
   {
      rpos = mPosition;
      cpos = cs;
      for (unsigned int i = 0; i < l; i++)
      {
         if (*cpos++ != *rpos++)
         {
            mPosition++;
            goto skip;
         }
      }
      return CurrentPosition(*this);
     skip: ;
   }
   return CurrentPosition(*this);
}

ParseBuffer::CurrentPosition
ParseBuffer::skipToChars(const Data& sub)
{
   const char* begSub = sub.mBuf;
   const char* endSub = sub.mBuf + sub.mSize;
   if(begSub == endSub)
   {
      fail(__FILE__, __LINE__, "ParseBuffer::skipToChars() called with an "
                                 "empty string. Don't do this!");
   }

   while (true)
   {
next:
     const char* searchPos = mPosition;
     const char* subPos = sub.mBuf;

     while (subPos != endSub) 
     {
         if (searchPos == mEnd)
         {
            // nope
            mPosition = mEnd;
            return CurrentPosition(*this);
         }
         if (*subPos++ != *searchPos++)
         {
            // nope, but try the next position
            ++mPosition;
            goto next;
         }
     }
     // found a match
     return CurrentPosition(*this);
   }
}

bool 
ParseBuffer::oneOf(char c, const char* cs)
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

bool 
ParseBuffer::oneOf(char c, const Data& cs)
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

ParseBuffer::CurrentPosition
ParseBuffer::skipToOneOf(const char* cs)
{
   while (mPosition < mEnd)
   {
      if (oneOf(*mPosition, cs))
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

ParseBuffer::CurrentPosition
ParseBuffer::skipToOneOf(const char* cs1,
                         const char* cs2)
{
   while (mPosition < mEnd)
   {
      if (oneOf(*mPosition, cs1) ||
          oneOf(*mPosition, cs2))
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

ParseBuffer::CurrentPosition
ParseBuffer::skipToOneOf(const Data& cs)
{
   while (mPosition < mEnd)
   {
      if (oneOf(*mPosition, cs))
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

ParseBuffer::CurrentPosition
ParseBuffer::skipToOneOf(const Data& cs1,
                         const Data& cs2)
{
   while (mPosition < mEnd)
   {
      if (oneOf(*mPosition, cs1) ||
          oneOf(*mPosition, cs2))
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

const char*
ParseBuffer::skipToEndQuote(char quote)
{
   while (mPosition < mEnd)
   {
      // !dlb! mark character encoding
      if (*mPosition == '\\')
      {
         mPosition += 2;
      }
      else if (*mPosition == quote)
      {
         return mPosition;
      }
      else
      {
         mPosition++;
      }
   }

   {
      Data msg("Missing '");
      msg += quote;
      msg += "'";
      fail(__FILE__,__LINE__,msg);
   }
   return 0;
}

const char*
ParseBuffer::skipBackChar()
{
   if (bof())
   {
      fail(__FILE__, __LINE__,"backed over beginning of buffer");
   }
   mPosition--;
   return mPosition;
}

const char*
ParseBuffer::skipBackWhitespace()
{
   while (!bof())
   {
      switch (*(--mPosition))
      {
         case ' ' :
         case '\t' : 
         case '\r' : 
         case '\n' : 
         {
            break;
         }
         default : 
            return ++mPosition;
      }
   }
   return mBuff;
}

// abcde
//     ^
// skipBackChar('d');
// abcde
//    ^
// skipChar('d');
// abcde
//     ^
const char*
ParseBuffer::skipBackChar(char c)
{
   if (bof())
   {
      fail(__FILE__, __LINE__,"backed over beginning of buffer");
   }
   if (*(--mPosition) != c)
   {
       Data msg( "Expected '");
       msg += c;
       msg += "'";
      fail(__FILE__, __LINE__,msg);
   }
   return mPosition;
}

// abcde
//      ^
// skipBackToChar('c');
// abcde
//    ^
const char*
ParseBuffer::skipBackToChar(char c)
{
   while (!bof())
   {
      if (*(--mPosition) == c)
      {
         return ++mPosition;
      }
   }
   return mBuff;
}

const char* 
ParseBuffer::skipBackToOneOf(const char* cs)
{
   while (!bof())
   {
      if (oneOf(*(--mPosition),cs))
      {
         return ++mPosition;
      }
   }
   return mBuff;
}

void
ParseBuffer::data(Data& data, const char* start) const
{
   if (!(mBuff <= start && start <= mPosition))
   {
      fail(__FILE__, __LINE__,"Bad anchor position");
   }

   if (data.mShareEnum == Data::Take)
   {
      delete[] data.mBuf;
   }
   data.mSize = (unsigned int)(mPosition - start);
   data.mBuf = const_cast<char*>(start);
   data.mCapacity = data.mSize;
   data.mShareEnum = Data::Share;
}

static const unsigned char hexToByte[256] = 
{
// 0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f
   'k','k','k','k','k','k','k','k','k','k','k','k','k','k','k','k',//0
   'k','k','k','k','k','k','k','k','k','k','k','k','k','k','k','k',//1
   'k','k','k','k','k','k','k','k','k','k','k','k','k','k','k','k',//2
   0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  'k','k','k','k','k','k', //3
   'k',0xA,0xB,0xC,0xD,0xE,0xF,'k','k','k','k','k','k','k','k','k', //4
   'k','k','k','k','k','k','k','k','k','k','k','k','k','k','k','k',//5
   'k',0xA,0xB,0xC,0xD,0xE,0xF,'k','k','k','k','k','k','k','k','k', //6
   'k','k','k','k','k','k','k','k','k','k','k','k','k','k','k','k',//8
   'k','k','k','k','k','k','k','k','k','k','k','k','k','k','k','k',//9
   'k','k','k','k','k','k','k','k','k','k','k','k','k','k','k','k',//a
   'k','k','k','k','k','k','k','k','k','k','k','k','k','k','k','k',//b
   'k','k','k','k','k','k','k','k','k','k','k','k','k','k','k','k',//c
   'k','k','k','k','k','k','k','k','k','k','k','k','k','k','k','k',//d
   'k','k','k','k','k','k','k','k','k','k','k','k','k','k','k','k',//e
   'k','k','k','k','k','k','k','k','k','k','k','k','k','k','k','k'   //f

};

void
ParseBuffer::dataUnescaped(Data& dataToUse, const char* start) const
{
   if (!(mBuff <= start && start <= mPosition))
   {
      fail(__FILE__, __LINE__, "Bad anchor position");
   }

   {
      const char* current = start;   
      while (current < mPosition)
      {
         if (*current == '%')
         {
            // needs to be unencoded
            goto copy;
         }
         current++;
      }
      // can use an overlay
      data(dataToUse, start);
      return;
   }

  copy:
   if ((size_t)(mPosition-start) > dataToUse.mCapacity)
   {
      dataToUse.resize(mPosition-start, false);
   }

   char* target = dataToUse.mBuf;
   const char* current = start;   
   while (current < mPosition)
   {
      if (*current == '%')
      {
         current++;
         if (mPosition - current < 2)
         {
            fail(__FILE__, __LINE__,"Illegal escaping");
         }
         const char high = hexToByte[(unsigned char)*current];
         const char low = hexToByte[(unsigned char)*(current + 1)];
         if (high!='k' && low!='k')
         {
            unsigned char escaped = 0;            
            escaped = high << 4 | low;
            // !bwc! I think this check is bogus, especially the ':' (58) check
            // You could maybe argue that the point of %-escaping is to allow
            // the use of UTF-8 data (including ASCII that is not allowed in an 
            // on-the-wire representation of whatever it is we're unescaping),
            // and not unprintable characters (the unprintable codes are not 
            // used by UTF-8). 
            if (escaped > 31 && escaped != 127 && escaped != 58)
            {
               *target++ = escaped;
               current+= 2;
            }
            else
            {
               *target++ = '%';
               *target++ = *current++;
               *target++ = *current++;
            }
         }
         else
         {
            fail(__FILE__, __LINE__,"Illegal escaping, not hex");
         }
      }
      else
      {
         *target++ = *current++;
      }
   }
   *target = 0;
   dataToUse.mSize = target - dataToUse.mBuf;   
}

Data
ParseBuffer::data(const char* start) const
{
   if (!(mBuff <= start && start <= mPosition))
   {
      
      fail(__FILE__, __LINE__,"Bad anchor position");
   }

   Data data(start, mPosition - start);
   return data;
}

int
ParseBuffer::integer()
{
   if (this->eof())
   {
      fail(__FILE__, __LINE__,"Expected a digit, got eof ");
   }

   int signum = 1;
   if (*mPosition == '-')
   {
      signum = -1;
      ++mPosition;
      assertNotEof();
   }
   else if (*mPosition == '+')
   {
      ++mPosition;
      assertNotEof();
   }

   if (!isdigit(*mPosition))
   {
       Data msg("Expected a digit, got: ");
       msg += Data(mPosition, (mEnd - mPosition));
      fail(__FILE__, __LINE__,msg);
   }
   
   int num = 0;
   int last=0;
   while (!eof() && isdigit(*mPosition))
   {
      last=num;
      num = num*10 + (*mPosition-'0');
      if(last > num)
      {
         fail(__FILE__, __LINE__,"Overflow detected.");
      }
      ++mPosition;
   }
   
   return signum*num;
}

UInt8
ParseBuffer::uInt8()
{
   const char* begin=mPosition;
   UInt8 num = 0;
   UInt8 last = 0;
   while (!eof() && isdigit(*mPosition))
   {
      last = num;
      num = num*10 + (*mPosition-'0');
      if(last>num)
      {
         fail(__FILE__, __LINE__,"Overflow detected.");
      }
      ++mPosition;
   }
   
   if(mPosition==begin)
   {
      fail(__FILE__, __LINE__,"Expected a digit");
   }
   return num;
}


//!dcm! -- merge these, ask about length checks
UInt32
ParseBuffer::uInt32()
{
   const char* begin=mPosition;
   UInt32 num = 0;
   while (!eof() && isdigit(*mPosition))
   {
      num = num*10 + (*mPosition-'0');
      ++mPosition;
   }

   switch(mPosition-begin)
   {
      case 0:
         fail(__FILE__, __LINE__,"Expected a digit");
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
      case 9:
         break;
      case 10:
         if(*begin<'4')
         {
            break;
         }
         else if(*begin=='4' && num >= 4000000000UL)
         {
            break;
         }
      default:
         fail(__FILE__, __LINE__,"Overflow detected");
   }

   return num;
}

UInt64
ParseBuffer::uInt64()
{
   const char* begin=mPosition;
   UInt64 num = 0;
   while (!eof() && isdigit(*mPosition))
   {
      num = num*10 + (*mPosition-'0');
      ++mPosition;
   }

   switch(mPosition-begin)
   {
      case 0:
         fail(__FILE__, __LINE__,"Expected a digit");
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
      case 9:
      case 10:
      case 11:
      case 12:
      case 13:
      case 14:
      case 15:
      case 16:
      case 17:
      case 18:
      case 19:
         break;
      case 20:
         if(*begin=='1' && num >= 10000000000000000000ULL)
         {
            break;
         }
      default:
         fail(__FILE__, __LINE__,"Overflow detected");
   }

   return num;
}

#ifndef RESIP_FIXED_POINT
float
ParseBuffer::floatVal()
{
   const char* s = mPosition;
   try
   {
      float mant = 0.0;
      int num = integer();

      if (*mPosition == '.')
      {
         skipChar();
         const char* pos = mPosition;
         mant = float(integer());
         int s = int(mPosition - pos);
         while (s--)
         {
            mant /= 10.0;
         }
      }
      return num + mant;
   }
   catch (ParseException&)
   {
      Data msg("Expected a floating point value, got: ");
      msg += Data(s, mPosition - s);
      fail(__FILE__, __LINE__,msg);
      return 0.0;
   }
}
#endif

int
ParseBuffer::qVal()
{
   // parse a qvalue into an integer between 0 and 1000  (ex: 1.0 -> 1000,  0.8 -> 800, 0.05 -> 50)
   const char* s = mPosition;
   try
   {
      int num = integer();
      if (num == 1)
      {
         num = 1000;
      }
      else if (num != 0)
      {
         // error: qvalue must start with 1 or 0
         return 0;
      }
      
      if (*mPosition == '.')
      {
         skipChar();
         
         int i = 100;
         while(!eof() && isdigit(*mPosition) && i)
         {
            num += (*mPosition-'0') * i;
            i /= 10;
            skipChar();
         }
      }
      return num;
   }
   catch (ParseException&)
   {
      Data msg("Expected a floating point value, got: ");
      msg += Data(s, mPosition - s);
      fail(__FILE__, __LINE__,msg);
      return 0;
   }
}
   

Data
spaces(unsigned int numSpaces)
{
   Data sps(numSpaces, Data::Preallocate);
   for (unsigned int i = 0; i < numSpaces; i++)
   {
      sps += ' ';
   }
   return sps;
}

Data 
escapeAndAnnotate(const char* buffer, 
                  Data::size_type size,
                  const char* position)
{ 
   Data ret(2*size+16, Data::Preallocate);

   const char* lastReturn = buffer;
   int lineCount = 0;
   bool doneAt = false;

   const char* p = buffer;
   for (unsigned int i = 0; i < size; i++)
   {
      unsigned char c = *p++;

      switch (c)
      {
         case 0x0D: // CR
         {
            continue;
         }
         case 0x0A: // LF
         {
            if (!doneAt && p >= position)
            {
               ret += "[CRLF]\n";
               ret += spaces((unsigned int)(position - lastReturn));
               ret += "^[CRLF]\n";
               doneAt = true;
            }
            else
            {
               lastReturn = p;
               ret += c;
            }
            lineCount++;
            continue;
         }
      }
      
      if (iscntrl(c) || (c >= 0x7F))
      {
         ret +='*'; // indicates unprintable character
         continue;
      }

      ret += c;
   }
   if (!doneAt && p >= position)
   {
      ret += "\n";
      ret += spaces((unsigned int)(position - lastReturn));
      ret += "^\n";
   }

   return ret;
}

void
ParseBuffer::fail(const char* file, unsigned int line, const Data& detail) const
{
    Data errmsg;
    {
       DataStream ds(errmsg);
       ds << file << ":" << line
          << ", Parse failed ";

       if (detail != Data::Empty) ds << detail << ' ' ;

       ds << "in context: " << mErrorContext
          << std::endl
          << escapeAndAnnotate(mBuff, mEnd - mBuff, mPosition);
          
       ds.flush();
   }
   DebugLog(<<errmsg);
   throw ParseException(errmsg, mErrorContext, file, line);
}

ParseBuffer::Pointer::Pointer(const ParseBuffer& pb,
                              const char* position,
                              bool atEof)
   : mPb(pb),
     mPosition(position),
     mIsValid(!atEof)
{}

ParseBuffer::Pointer::Pointer(const CurrentPosition& pos) :
   mPb(pos.mPb),
   mPosition(pos),
   mIsValid(pos.mPb.valid())
{}

const char& 
ParseBuffer::Pointer::operator*() const
{
   if (mIsValid)
   {
      return *mPosition;
   }
   else
   {
      throw ParseException(msg, mPb.getContext(), __FILE__, __LINE__);
   }
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
