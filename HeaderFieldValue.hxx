#ifndef HeaderFieldValue_hxx
#define HeaderFieldValue_hxx

#include <iostream>
#include <sipstack/ParameterList.hxx>
#include <sipstack/ParseException.hxx>


namespace Vocal2
{

class ParserCategory;
class UnknownParameter;

class HeaderFieldValue
{
   public:
      HeaderFieldValue();
      HeaderFieldValue(const char* field, unsigned int fieldLength);
      HeaderFieldValue(const HeaderFieldValue& hfv);
      HeaderFieldValue(ParserCategory* parserCategory);

      ~HeaderFieldValue();

      HeaderFieldValue* clone() const;
      ParameterList& getParameters();
      ParameterList& getUnknownParameters();
      bool isParsed() const;

      bool exists(const Data& subcomponent);
      bool exists(const ParameterTypes::Type type);
      
      UnknownParameter* get(const Data& type);
      
      HeaderFieldValue* next;

      ParserCategory* getParserCategory()
      {
         return mParserCategory;
      }

      void setParserCategory(ParserCategory* parser)
      {
         mParserCategory = parser;
      }

      void parseParameters(const char* start);
      
      std::ostream& encode(std::ostream& str) const;
      
      friend std::ostream& operator<<(std::ostream&, HeaderFieldValue&);

      ParserCategory* mParserCategory;
      const char* mField;
      const unsigned int mFieldLength;
   private:
      ParameterList mParameterList;
      ParameterList mUnknownParameterList;
      bool mMine;
};

std::ostream& operator<<(std::ostream& stream, 
			 HeaderFieldValue& hList);


}



#endif
