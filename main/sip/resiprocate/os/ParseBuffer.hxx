#ifndef ParseBuffer_hxx
#define ParseBuffer_hxx
#include <string.h>
#include <util/Data.hxx>
#include <util/VException.hxx>

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

      class Exception : public VException
      {
         public:
            Exception(const Data& msg, const Data& file, const int line)
               : VException(msg, file, line) {}

            Data getName() const { return "ParseBuffer::Exception"; }
      };
      
      // allow the buffer to be rolled back
      void reset(const char* pos);

      bool eof() { return mStart >= mEnd;}
      const char* position() { return mStart; }
      const char* end() { return mEnd; }

      const char* skipChar() { return ++mStart; }
      const char* skipChar(char c);
      const char* skipNonWhitespace();
      const char* skipWhitespace();
      const char* skipToChar(char c);
      const char* skipToOneOf(const char* cs);
      const char* skipToOneOf(const char* cs1, const char* cs2);

      const char* skipToEndQuote(char quote = '"');
      //create a data from start to the current position
      Data data(const char* start) const { return Data(start, mStart - start); }

      static const char* Whitespace;
      static const char* ParamTerm;
   private:
      Data tmpData;
      
      const char* mBuff;
      const char* mStart;
      const char* mEnd;
};

}

#endif
