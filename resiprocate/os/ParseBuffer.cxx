
#include <util/ParseBuffer.hxx>
#include <cassert>

//#include <iostream>
//using namespace std;

using namespace Vocal2;

const char* ParseBuffer::ParamTerm = ";?";
const char* ParseBuffer::Whitespace = " \t\r\n";

void
ParseBuffer::reset(const char* pos)
{
   assert(pos >= mBuff && pos <= mStart);
   mStart = pos;
}

const char*
ParseBuffer::skipChar(char c)
{
   if (*position() != c)
   {
      throw Exception("Expected '" + Data(1, c) + "'", __FILE__, __LINE__);
   }
   return ++mStart;
}

const char* 
ParseBuffer::skipNonWhitespace()
{
   while (mStart < mEnd)
   {
      switch (*position())
      {
         case ' ' :
         case '\t' : 
         case '\r' : 
         case '\n' : 
            return position();
         default : 
            mStart++;
      }
   }
   return position();
}

const char* 
ParseBuffer::skipWhitespace()
{
   while (mStart < mEnd)
   {
      switch (*position())
      {
         case ' ' :
         case '\t' : 
         case '\r' : 
         case '\n' : 
         {
            mStart++;
            break;
         }
         default : 
            return position();
      }
   }
   return position();
}

const char* 
ParseBuffer::skipToChar(char c)
{
   while (mStart < mEnd)
   {
      if (*position() == c)
      {
         return position();
      }
      else
      {
         mStart++;
      }
   }
   return position();
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

const char* 
ParseBuffer::skipToOneOf(const char* cs)
{
   while (mStart < mEnd)
   {
      if (oneOf(*position(), cs))
      {
         return position();
      }
      else
      {
         mStart++;
      }
   }
   return position();
}

const char* 
ParseBuffer::skipToOneOf(const char* cs1,
                         const char* cs2)
{
   while (mStart < mEnd)
   {
      if (oneOf(*position(), cs1) ||
          oneOf(*position(), cs2))
      {
         return position();
      }
      else
      {
         mStart++;
      }
   }
   return position();
}

const char* 
ParseBuffer::skipToEndQuote(char quote)
{
   while (mStart < mEnd)
   {
      if (*position() == '\\')
      {
         mStart += 2;
      }
      else if (*position() == quote)
      {
         return position();
      }
      else
      {
         mStart++;
      }
   }

   throw Exception("Missing '" + Data(1, quote) + "'", __FILE__, __LINE__);
}
