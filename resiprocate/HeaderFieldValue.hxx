#ifndef HeaderFieldValue_hxx
#define HeaderFieldValue_hxx

#include <sipstack/ParseException.hxx>
#include <sipstack/ParameterTypes.hxx>

#include <iostream>

namespace Vocal2
{

class ParserCategory;
class UnknownParameter;
class ParseBuffer;

class HeaderFieldValue
{
   public:
      HeaderFieldValue()
         : mParserCategory(0),
           mField(0),
           mFieldLength(0),
           mMine(false)
      {}
      HeaderFieldValue(const char* field, unsigned int fieldLength);
      HeaderFieldValue(const HeaderFieldValue& hfv, ParserCategory* assignParser);

      ~HeaderFieldValue();

      bool isParsed() const;

      ParserCategory* getParserCategory()
      {
         return mParserCategory;
      }

      void setParserCategory(ParserCategory* parser)
      {
         mParserCategory = parser;
      }

      std::ostream& encode(std::ostream& str) const;
      
      ParserCategory* mParserCategory;
      const char* mField;
      const unsigned int mFieldLength;

   private:
      HeaderFieldValue(const HeaderFieldValue&);
      HeaderFieldValue& operator=(const HeaderFieldValue&);
      
      bool mMine;

      friend std::ostream& operator<<(std::ostream&, HeaderFieldValue&);
};

std::ostream& operator<<(std::ostream& stream, 
			 HeaderFieldValue& hList);


}

#endif
