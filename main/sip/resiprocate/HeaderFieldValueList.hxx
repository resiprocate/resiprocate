#ifndef HFVLIST_HXX
#define HFVLIST_HXX

#include <iostream>
#include <sipstack/HeaderFieldValue.hxx>

namespace Vocal2
{

class ParserContainerBase;

class HeaderFieldValueList
{
   public:
      HeaderFieldValueList();

      HeaderFieldValueList(const HeaderFieldValueList& other);
      
      ~HeaderFieldValueList();

      HeaderFieldValueList* clone() const;

      void
      setParserContainer(ParserContainerBase* parser)
      {
         mParserContainer = parser;
      }
      
      ParserCategory*
      getParserCategory();
      
      // insert at begining
      void push_front(HeaderFieldValue* header);

      // append to end
      void push_back(HeaderFieldValue* header);

      void pop_front();

      HeaderFieldValue* first;
  
      HeaderFieldValue* last;

   private:
      ParserContainerBase* mParserContainer;
};

}

std::ostream& operator<<(std::ostream& stream, 
			 Vocal2::HeaderFieldValueList& hList);

#endif
