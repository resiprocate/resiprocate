#ifndef HeaderFieldValue
#define HeaderFieldValue

namespace Vocal2
{

class HeaderFieldValue
{
   public:
      HeaderFieldValue(const char* field, uint fieldLength);
      HeaderFieldValue(const HeaderFieldValue& hfv);
      HeaderFieldValue(ParserCategory* parserCategory);

      HeaderFieldValue* clone() const;
      ParameterList& getParameters();
      ParameterList& getUnknownParameters();
      bool isParsed() const;
  
  HeaderFieldValue* next;

   private:
      const char* mField;
      const uint mFieldLength;
      SubComponentList mSubComponentList;
      SubComponentList mUnknownSubComponentList;
      ParserCategory* mParserCategory;
};


}

std::ostream& operator<<(std::ostream& stream, 
			 Vocal2::HeaderFieldValue& hList);


#endif
