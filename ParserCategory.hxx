#ifndef ParserCategory_hxx
#define ParserCategory_hxx

#include <sipstack/Data.hxx>
#include <iostream>

namespace Vocal2
{

class HeaderFieldValue;
class UnknownParameter;

class ParserCategory
{
   public:
      ParserCategory(HeaderFieldValue* headerFieldValue)
         : mHeaderField(headerFieldValue),
           mIsParsed(false)
      {}

      virtual ~ParserCategory() {}

      virtual void parse() = 0;
      virtual ParserCategory* clone(HeaderFieldValue*) const = 0;
      virtual std::ostream& encode(std::ostream& str) const = 0;
      
      bool isParsed() const
      {
         return mIsParsed;
      }

      void parseParameters(const char* start);

      UnknownParameter& operator[](const Data& param);

      HeaderFieldValue& getHeaderField() { return *mHeaderField; }
   protected:
      ParserCategory()
         : mHeaderField(0),
           mIsParsed(true)
      {}

      // call before every access 
      void checkParsed()
      {
         if (!mIsParsed)
         {
            mIsParsed = true;
            parse();
         }
      }

      HeaderFieldValue* mHeaderField;
   private:
      bool mIsParsed;
};

}

#endif
