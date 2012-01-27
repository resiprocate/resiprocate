#ifndef StartLine_Include_Guard
#define StartLine_Include_Guard

#include "resip/stack/LazyParser.hxx"

namespace resip
{
class StartLine : public LazyParser
{
   public:
      StartLine() {}
      explicit StartLine(const HeaderFieldValue& headerFieldValue) :
         LazyParser(headerFieldValue)
      {}
      StartLine(const char* buf, int length) :
         LazyParser(buf, length)
      {}
      virtual ~StartLine(){}

      virtual EncodeStream& encodeParsed(EncodeStream& str) const=0;
      virtual void parse(ParseBuffer& pb)=0;
      virtual const Data& errorContext() const=0;
      virtual StartLine* clone() const=0;
      virtual StartLine* clone(void* location) const=0;


}; // class StartLine

} // namespace resip

#endif // include guard
