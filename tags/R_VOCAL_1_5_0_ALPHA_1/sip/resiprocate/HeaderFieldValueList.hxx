#ifndef HeaderFieldValueList_hxx
#define HeaderFieldValueList_hxx

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
      
      ParserContainerBase*
      getParserContainer();
      
      // insert at begining
      void push_front(HeaderFieldValue* header);

      // append to end
      void push_back(HeaderFieldValue* header);

      void pop_front();

      HeaderFieldValue* front() {return first;}

      bool moreThanOne() const
      {
         return first != 0 && first->next != 0;
      }
      
      HeaderFieldValue* first;
  
      HeaderFieldValue* last;

      std::ostream& encode(std::ostream& str) const;
   private:
      ParserContainerBase* mParserContainer;
};

}

std::ostream& operator<<(std::ostream& stream, 
			 Vocal2::HeaderFieldValueList& hList);

#endif
