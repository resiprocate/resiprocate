#ifndef HeaderFieldValueList_hxx
#define HeaderFieldValueList_hxx

#include <iostream>
#include <list>

namespace Vocal2
{

class ParserContainerBase;
class HeaderFieldValue;

class HeaderFieldValueList
{
   public:
      HeaderFieldValueList()
         : mParserContainer(0)
      {}

      ~HeaderFieldValueList();
      HeaderFieldValueList(const HeaderFieldValueList& rhs);

      void setParserContainer(ParserContainerBase* parser) {mParserContainer = parser;}
      ParserContainerBase* getParserContainer() const {return mParserContainer;}

      bool empty() const {return mHeaders.empty();}
      size_t size() const {return mHeaders.size();}
      void push_front(HeaderFieldValue* header) {mHeaders.push_front(header);}
      void push_back(HeaderFieldValue* header) {mHeaders.push_back(header);}
      void pop_front() {mHeaders.pop_front();}
      void pop_back() {mHeaders.pop_back();};
      HeaderFieldValue* front() {return mHeaders.front();}
      HeaderFieldValue* back() {return mHeaders.back();}
      const HeaderFieldValue* front() const {return mHeaders.front();}
      const HeaderFieldValue* back() const {return mHeaders.back();}

   private:
      typedef std::list<HeaderFieldValue*> ListImpl;
   public:
      typedef ListImpl::iterator iterator;
      typedef ListImpl::const_iterator const_iterator;

      iterator begin() {return mHeaders.begin();}
      iterator end() {return mHeaders.end();}
      const_iterator begin() const {return mHeaders.begin();}
      const_iterator end() const {return mHeaders.end();}

   private:
      std::list<HeaderFieldValue*> mHeaders;
      ParserContainerBase* mParserContainer;
};

}

#endif
