#ifndef ParserCategory_hxx
#define ParserCategory_hxx

#include <string>

namespace Vocal2
{

class HeaderFieldValue;
class UnknownSubComponent;

class ParserCategory
{
   public:
      virtual ~ParserCategory() {}
      virtual ParserCategory* clone(HeaderFieldValue*) const = 0;

   protected:
      // call before every access 
      void checkParsed()
      {
         if (!mIsParsed)
         {
            mIsParsed = true;
            parse();
         }
      }
      virtual void parse() = 0;

      HeaderFieldValue* mHeaderField;
   private:
      bool mIsParsed;
};

}

#endif
