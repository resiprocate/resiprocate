#include <util/ParseBuffer.hxx>
#include <assert.h>

using namespace Vocal2;

void
ParseBuffer::reset(const char* pos)
{
   assert(pos >= mBuff && pos <= mStart);
   mStart = pos;
}

const char* 
ParseBuffer::skipNonWhiteSpace()
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
ParseBuffer::skipWhiteSpace()
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
      if (c == *++cs)
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
ParseBuffer::skipToEndQuote(const char* cs)
{
   // !dlb! quote escape issues
   return skipToChar('"');
}

