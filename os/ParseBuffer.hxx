#ifndef ParseBuffer_hxx
#define ParseBuffer_hxx

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

      // allow the buffer to be rolled back
      void reset(const char* pos);

      bool eof() { return mStart == mEnd;}
      const char* position() { return mStart; }

      const char* skipChar() { return mStart++; }
      const char* skipNonWhiteSpace();
      const char* skipWhiteSpace();
      const char* skipToChar(char c);
      const char* skipToOneOf(const char* cs);
      const char* skipToEndQuote(const char* cs);

   private:
      const char* mBuff;
      const char* mStart;
      const char* mEnd;
};

}

#endif
