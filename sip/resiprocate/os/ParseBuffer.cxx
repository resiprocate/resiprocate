#include <util/Logger.hxx>
#include <util/ParseBuffer.hxx>
#include <cassert>


using namespace Vocal2;

#define VOCAL_SUBSYSTEM Subsystem::SIP

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
      DebugLog (<< "Expected " << c );
      throw Exception("parse error", __FILE__, __LINE__);
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

   DebugLog (<< "Missing '" << quote);
   throw Exception("Missing quote", __FILE__,__LINE__);
}
