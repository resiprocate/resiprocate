#ifndef HeaderFieldValue_hxx
#define HeaderFieldValue_hxx

#include <sipstack/ParameterList.hxx>
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
      HeaderFieldValue();
      HeaderFieldValue(const char* field, unsigned int fieldLength);
      HeaderFieldValue(const HeaderFieldValue& hfv);
      HeaderFieldValue(ParserCategory* parserCategory);

      ~HeaderFieldValue();

      HeaderFieldValue* clone() const;

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
      
      HeaderFieldValue* next;
      ParserCategory* mParserCategory;
      const char* mField;
      const unsigned int mFieldLength;

   private:
      bool mMine;

      friend std::ostream& operator<<(std::ostream&, HeaderFieldValue&);
};

std::ostream& operator<<(std::ostream& stream, 
			 HeaderFieldValue& hList);


}

#endif
