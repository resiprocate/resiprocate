#include <cassert>

#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

const char* ParseBuffer::ParamTerm = ";?"; // maybe include "@>,"?
const char* ParseBuffer::Whitespace = " \t\r\n";
const Data ParseBuffer::Pointer::msg("dereferenced ParseBuffer eof");

ParseBuffer::ParseBuffer(const char* buff, unsigned int len, 
                         const Data& errorContext)
   : mBuff(buff),
     mPosition(buff),
     mEnd(buff+len),
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

const Data& 
ParseBuffer::getContext() const
{
   return mErrorContext;
}

void
ParseBuffer::reset(const char* pos)
{
   assert( mBuff <= mEnd);
   assert( (pos >= mBuff) && (pos <= mEnd) );
   mPosition = pos;
}

ParseBuffer::Pointer 
ParseBuffer::skipChar()
{
   if (eof())
   {
      fail(__FILE__, __LINE__,"skipped over eof");
   }
   return Pointer(*this, ++mPosition, eof());
}

ParseBuffer::Pointer
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
   return Pointer(*this, ++mPosition, eof());
}

ParseBuffer::Pointer 
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
   return Pointer(*this, mPosition, eof());
}

ParseBuffer::Pointer 
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
   return Pointer(*this, mPosition, eof());
}

ParseBuffer::Pointer 
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
            return Pointer(*this, mPosition, false);
         default : 
            mPosition++;
      }
   }
   return Pointer(*this, mPosition, eof());
}

ParseBuffer::Pointer 
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
            return Pointer(*this, mPosition, false);
      }
   }
   return Pointer(*this, mPosition, true);
}

// "SIP header field values can be folded onto multiple lines if the
//  continuation line begins with a space or horizontal tab"

// CR can be quote with \ within "" and comments -- treat \CR as whitespace
ParseBuffer::Pointer 
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
            return Pointer(*this, mPosition, false);
         }
      }
   }
   return Pointer(*this, mPosition, true);
}

ParseBuffer::Pointer
ParseBuffer::skipToTermCRLF()
{
   while (mPosition < mEnd)
   {
      static Data CRLF("\r\n");
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
         return Pointer(*this, mPosition, false);
      }
   }
   return Pointer(*this, mPosition, true);
}

ParseBuffer::Pointer 
ParseBuffer::skipToChar(char c)
{
   while (mPosition < mEnd)
   {
      if (*mPosition == c)
      {
         return Pointer(*this, mPosition, false);
      }
      else
      {
         mPosition++;
      }
   }
   return Pointer(*this, mEnd, true);
}

ParseBuffer::Pointer
ParseBuffer::skipToChars(const char* cs)
{
   assert(cs);
   unsigned int l = strlen(cs);

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
      return Pointer(*this, mPosition, false);
     skip: ;
   }
   return Pointer(*this, mPosition, true);
}

ParseBuffer::Pointer
ParseBuffer::skipToChars(const Data& sub)
{
   const char* begSub = sub.mBuf;
   const char* endSub = sub.mBuf + sub.mSize;
   assert(begSub != endSub);

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
	   return Pointer(*this, mPosition, true);
	}
	if (*subPos++ != *searchPos++)
	{
	   // nope, but try the next position
	   ++mPosition;
	   goto next;
	}
     }
     // found a match
     return Pointer(*this, searchPos, false);
   }
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

ParseBuffer::Pointer 
ParseBuffer::skipToOneOf(const char* cs)
{
   while (mPosition < mEnd)
   {
      if (oneOf(*mPosition, cs))
      {
         return Pointer(*this, mPosition, false);
      }
      else
      {
         mPosition++;
      }
   }
   return Pointer(*this, mPosition, true);
}

ParseBuffer::Pointer 
ParseBuffer::skipToOneOf(const char* cs1,
                         const char* cs2)
{
   while (mPosition < mEnd)
   {
      if (oneOf(*mPosition, cs1) ||
          oneOf(*mPosition, cs2))
      {
         return Pointer(*this, mPosition, false);
      }
      else
      {
         mPosition++;
      }
   }
   return Pointer(*this, mPosition, true);
}

ParseBuffer::Pointer 
ParseBuffer::skipToOneOf(const Data& cs)
{
   while (mPosition < mEnd)
   {
      if (oneOf(*mPosition, cs))
      {
         return Pointer(*this, mPosition, false);
      }
      else
      {
         mPosition++;
      }
   }
   return Pointer(*this, mPosition, true);
}

ParseBuffer::Pointer 
ParseBuffer::skipToOneOf(const Data& cs1,
                         const Data& cs2)
{
   while (mPosition < mEnd)
   {
      if (oneOf(*mPosition, cs1) ||
          oneOf(*mPosition, cs2))
      {
         return Pointer(*this, mPosition, false);
      }
      else
      {
         mPosition++;
      }
   }
   return Pointer(*this, mPosition, true);
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

ParseBuffer::Pointer 
ParseBuffer::skipN(int count)
{
   mPosition += count;
   if (mPosition > mEnd)
   {

      fail(__FILE__, __LINE__, "skipped eof");
   }
   return Pointer(*this, mPosition, eof());
}

ParseBuffer::Pointer 
ParseBuffer::skipToEnd()
{
   mPosition = mEnd;
   return Pointer(*this, mPosition, true);
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

const char*
ParseBuffer::skipBackN(int count)
{
   mPosition -= count;
   if (bof())
   { 
     fail(__FILE__, __LINE__,"backed over beginning of buffer");
   }
   return mPosition;
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

void
ParseBuffer::data(Data& data, const char* start) const
{
   if (!(mBuff <= start && start <= mPosition))
   {
      fail(__FILE__, __LINE__,"Bad anchor position");
   }

   if (data.mMine == Data::Take)
   {
      delete[] data.mBuf;
   }
   data.mSize = (unsigned int)(mPosition - start);
   data.mBuf = const_cast<char*>(start);
   data.mCapacity = data.mSize;
   data.mMine = Data::Share;
}

static const unsigned char isHex[256] = 
{
// 0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  //0
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  //1
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  //2
   1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  //3
   0,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  //4
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  //5
   0,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  //6
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  //8
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  //9
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  //a
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  //b
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  //c
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  //d
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  //e
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0   //f

};

static const unsigned char hexToByte[128] = 
{
 // 0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f
   'k','k','k','k','k','k','k','k','k','k','k','k','k','k','k','k', //0
   'k','k','k','k','k','k','k','k','k','k','k','k','k','k','k','k', //1
   'k','k','k','k','k','k','k','k','k','k','k','k','k','k','k','k', //2
   0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  'k','k','k','k','k','k', //3
   'k',0xA,0xB,0xC,0xD,0xE,0xF,'k','k','k','k','k','k','k','k','k', //4
   'k','k','k','k','k','k','k','k','k','k','k','k','k','k','k','k', //5
   'k',0xA,0xB,0xC,0xD,0xE,0xF,'k','k','k','k','k','k','k','k','k', //6
   'k','k','k','k','k','k','k','k','k','k','k','k','k','k','k','k'  //7
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
         const char high = *current;
         const char low = *(current + 1);         
         if (isHex[(size_t)high] && isHex[(size_t)low])
         {
            unsigned char escaped = 0;            
            assert(hexToByte[(size_t)high] != 'k');
            assert(hexToByte[(size_t)low] != 'k');
            escaped = hexToByte[(size_t)high] << 4 | hexToByte[(size_t)low];
            if (escaped > 31 && escaped != 127 && escaped != 58)
            {
               *target++ = escaped;
               current+= 2;
            }
            else
            {
               *target++ = '%';
               *target++ = high;
               *target++ = low;               
               current += 2;
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

   char c = *position();

   int signum = 1;
   if (c == '-')
   {
      signum = -1;
      skipChar();
      c = *position();
   }
   else if (c == '+')
   {
      skipChar();
      c = *position();
   }

   if (!isdigit(c))
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
      if(signum*last > signum*num)
      {
         fail(__FILE__, __LINE__,"Overflow detected.");
      }
      skipChar();
   }
   
   return signum*num;
}

UInt8
ParseBuffer::uInt8()
{
   if (this->eof())
   {

      fail(__FILE__, __LINE__,"Expected a digit, got eof ");
   }

   const char* p = mPosition;
   assert(p);
   char c = *p;

   if (!isdigit(c))
   {
      Data msg("Expected a digit, got: ");
      msg += Data(mPosition, (mEnd - mPosition));
      fail(__FILE__, __LINE__,msg);
   }
   
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
      skipChar();
   }
   
   return num;
}


//!dcm! -- merge these, ask about length checks
UInt32
ParseBuffer::uInt32()
{
   if (this->eof())
   {

      fail(__FILE__, __LINE__,"Expected a digit, got eof ");
   }

   const char* p = mPosition;
   assert(p);
   char c = *p;

   if (!isdigit(c))
   {
      Data msg("Expected a digit, got: ");
      msg += Data(mPosition, (mEnd - mPosition));
      fail(__FILE__, __LINE__,msg);
   }
   
   UInt32 num = 0;
   UInt32 last = 0;
   while (!eof() && isdigit(*mPosition))
   {
      last = num;
      num = num*10 + (*mPosition-'0');
      if(last>num)
      {
         fail(__FILE__, __LINE__,"Overflow detected.");
      }
      skipChar();
   }
   
   return num;
}

UInt64
ParseBuffer::uInt64()
{
   if (this->eof())
   {

      fail(__FILE__, __LINE__,"Expected a digit, got eof ");
   }

   const char* p = mPosition;
   assert(p);
   char c = *p;

   if (!isdigit(c))
   {
      Data msg("Expected a digit, got: ");
      msg += Data(mPosition, (mEnd - mPosition));
      fail(__FILE__, __LINE__,msg);
   }
   
   UInt64 num = 0;
   UInt64 last = 0;
   while (!eof() && isdigit(*mPosition))
   {
      last = num;
      num = num*10 + (*mPosition-'0');
      if(last>num)
      {
         fail(__FILE__, __LINE__,"Overflow detected.");
      }
      skipChar();
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
         int s = mPosition - pos;
         while (s--)
         {
            mant /= 10.0;
         }
      }
      return num + mant;
   }
   catch (Exception&)
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
   catch (Exception&)
   {
      Data msg("Expected a floating point value, got: ");
      msg += Data(s, mPosition - s);
      fail(__FILE__, __LINE__,msg);
      return 0;
   }
}
   

void
ParseBuffer::assertEof() const
{
   if (!eof())
   {

      fail(__FILE__, __LINE__,"expected eof");
   }      
}

void
ParseBuffer::assertNotEof() const
{
   if (eof())
   {
      fail(__FILE__, __LINE__,"unexpected eof");
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
                  unsigned int size,
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
    DataStream ds(errmsg);
    ds << file << ":" << line
       << ", Parse failed ";

    if (detail != Data::Empty) ds << detail << ' ' ;

    ds << "in context: " << mErrorContext
       << std::endl
       << escapeAndAnnotate(mBuff, mEnd - mBuff, mPosition);
    ds.flush();

   throw Exception(errmsg, mErrorContext, file, line);
}

ParseBuffer::Exception::Exception(const Data& msg, const Data& context, const Data& file, const int line)
   : resip::BaseException(msg, file, line) 
{}

ParseBuffer::Exception::~Exception() throw() 
{}

const char* 
ParseBuffer::Exception::name() const 
{ 
   return "ParseBuffer::Exception"; 
}

const Data& 
ParseBuffer::Exception::getContext() const
{
   return mContext;
}

ParseBuffer::Pointer::Pointer(const ParseBuffer& pb,
                              const char* position,
                              bool atEof)
   : mPb(pb),
     mPosition(position),
     mIsValid(!atEof)
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
      throw ParseBuffer::Exception(msg, mPb.getContext(), __FILE__, __LINE__);
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
