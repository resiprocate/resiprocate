#ifndef ParserCategory_hxx
#define ParserCategory_hxx

#include <sipstack/Data.hxx>
#include <ostream>

namespace Vocal2
{

class HeaderFieldValue;
class UnknownSubComponent;

class ParserCategory
{
   public:
      virtual ~ParserCategory() {}
      virtual ParserCategory* clone(HeaderFieldValue*) const = 0;

      virtual std::ostream& encode(std::ostream& str) const 
      {
         assert(0);
         return str;
      }

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
