#ifndef ParserCategory_hxx
#define ParserCategory_hxx

#include <sipstack/Data.hxx>
#include <iostream>

namespace Vocal2
{

class HeaderFieldValue;
class UnknownSubComponent;

class ParserCategory
{
   public:
      
      ParserCategory(HeaderFieldValue* headerFieldValue)
         : mHeaderField(headerFieldValue) 
      {}

      //!dcm! -- will need to add different type of clones to HeaderFieldValue
      //in order to write copy constructor

      virtual ~ParserCategory() {}
      virtual ParserCategory* clone(HeaderFieldValue*) const = 0;

      virtual std::ostream& encode(std::ostream& str) const 
      { // !dcm!  this is just so things are compiled until all parses get written
         assert(0);
         return str;
      }
      
      bool isParsed() const
      {
         return mIsParsed;
      }
      
      virtual void parse() = 0;
      HeaderFieldValue& getHeaderField() { return *mHeaderField; }
   protected:
      ParserCategory() 
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
