#ifndef HeaderFieldValue_hxx
#define HeaderFieldValue_hxx

#include <sip2/sipstack/SubComponentList.hxx>
#include <sip2/sipstack/ParseException.hxx>


namespace Vocal2
{

class ParserCategory;
class UnknownSubComponent;

class HeaderFieldValue
{
   public:
      HeaderFieldValue(const char* field, unsigned int fieldLength);
      HeaderFieldValue(const HeaderFieldValue& hfv);
      HeaderFieldValue(ParserCategory* parserCategory);

      ~HeaderFieldValue();

      HeaderFieldValue* clone() const;
      SubComponentList& getSubComponents();
      SubComponentList& getUnknownSubComponents();
      bool isParsed() const;

      bool exists(const std::string& subcomponent);
      bool exists(const SubComponent::Type type);
      
      UnknownSubComponent* get(const std::string& type);
      
      HeaderFieldValue* next;
      friend std::ostream& operator<<(std::ostream&, HeaderFieldValue&);

   private:
      const char* mField;
      const unsigned int mFieldLength;
      SubComponentList mSubComponentList;
      SubComponentList mUnknownSubComponentList;
      ParserCategory* mParserCategory;
      bool mMine;
};


std::ostream& operator<<(std::ostream& stream, 
			 HeaderFieldValue& hList);


}



#endif
