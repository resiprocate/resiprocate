#include <util/ParseBuffer.hxx>
#include <assert.h>
//#include <iostream>
//using namespace std;

using namespace Vocal2;

const char* ParseBuffer::WhitespaceOrParamTerm = " \t\r\n;?";
const char* ParseBuffer::WhitespaceOrSlash = " \t\r\n/";
const char* ParseBuffer::WhitespaceOrSemi = " \t\r\n;";
const char* ParseBuffer::Whitespace = " \t\r\n";
const char* ParseBuffer::SemiColonOrColon = ";:";

void
ParseBuffer::reset(const char* pos)
{
   assert(pos >= mBuff && pos <= mStart);
   mStart = pos;
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
ParseBuffer::skipToEndQuote()
{
   while (mStart < mEnd)
   {
      switch (*position())
      {
         case '\\' :
         {
            mStart += 2;
            break;
         }
         case '"' :
         {
            return position();
         }
         default :
         {
            mStart++;
         }
      }
   }
   return position();
}
