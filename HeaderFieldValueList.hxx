#ifndef HFVLIST_HXX
#define HFVLIST_HXX

#include <ostream>
#include <sip2/sipstack/HeaderFieldValue.hxx>

namespace Vocal2
{

class HeaderFieldValueList
{
   public:
      HeaderFieldValueList();

      HeaderFieldValueList(const HeaderFieldValueList& other);
      
      ~HeaderFieldValueList();

      HeaderFieldValueList* clone() const;

      void
      setParserCategory(ParserCategory* parser)
      {
         mParserCategory = parser;
      }
      
      ParserCategory&
      getParserCategory();
      
      // insert at begining
      void push_front(HeaderFieldValue* header);

      // append to end
      void push_back(HeaderFieldValue* header);

      void pop_front();

      HeaderFieldValue* first;
  
      HeaderFieldValue* last;

      ParserCategory* mParserCategory;
};

}

std::ostream& operator<<(std::ostream& stream, 
			 Vocal2::HeaderFieldValueList& hList);

#endif
