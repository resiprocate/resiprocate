#ifndef ParseBuffer_hxx
#define ParseBuffer_hxx
#include <string.h>
#include <util/Data.hxx>

namespace Vocal2
{

class ParseBuffer
{
   public:
      ParseBuffer(const char* buff,
                  unsigned int len)
         : mBuff(buff),
           mStart(buff),
           mEnd(buff+len)
      {}

      // for test
      explicit ParseBuffer(const char* b)
         : tmpData(b),
           mBuff(tmpData.c_str()),
           mStart(tmpData.c_str()),
           mEnd(tmpData.c_str() + tmpData.size())
      {}

      // allow the buffer to be rolled back
      void reset(const char* pos);

      bool eof() { return mStart >= mEnd;}
      const char* position() { return mStart; }
      const char* end() { return mEnd; }

      const char* skipChar() { return ++mStart; }
      const char* skipNonWhitespace();
      const char* skipWhitespace();
      const char* skipToChar(char c);
      const char* skipToOneOf(const char* cs);
      const char* skipToEndQuote();

      static const char* WhitespaceOrParamTerm;
      static const char* WhitespaceOrSlash;
      static const char* WhitespaceOrSemiColon;
      static const char* WhitespaceOrColonOrSemiColon;
      static const char* WhitespaceOrSemiColonOrRAQuote;
      static const char* WhitespaceOrColonOrSemiColonOrRAQuote;
      static const char* Whitespace;
      static const char* SemiColonOrColon;
      static const char* ColonOrAtSign;
   private:
      Data tmpData;
      
      const char* mBuff;
      const char* mStart;
      const char* mEnd;
};

}

#endif
