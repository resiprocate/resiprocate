#ifndef ParserContainer_hxx
#define ParserContainer_hxx

#include <sip2/sipstack/HeaderFieldValue.hxx>

template<class T, class IList>
ParserContainer
{
  public:
   ParserContainer(IList& list)
      : mList(list)
      {}
   
   T& front() { return *dynamic_cast<T*>(mList.first->parserCategory) }
   T& back() { return *dynamic_cast<T*>(mList.last->parserCategory) }
   
   bool empty() { return (mList.first == 0) }
   
   
   void push_front(T & t) { mList.push_front(new HeaderFieldValue(t.clone())); }
   void push_back(T & t) { mList.push_front(new HeaderFieldValue(t.clone())); }

   void pop_front(T & t) { mList.pop_front(); }
//   void pop_back(T & t) { mList.pop_back(); } ?? do we need this?

   class Iterator
   {
      private:
         Iterator(HeaderFieldValue* p);
      public:
         Iterator& operator++(int)
         {
            Iterator tmp(mPtr);
            mPtr = mPtr->next;
            return tmp;
         }
         
         Iterator& operator++()
         {
            mPtr = mPtr->next;
            return this;
         }

         T& operator*()
         {
            return *dynamic_cast<T*>(mPtr->parserCategory);
         }

         T& operator->()
         {
            return *dynamic_cast<T*>(mPtr->parserCategory);
         }
         
         bool operator==(Iterator& i)
         {
            return mPtr == i.mPtr;
         }
      private:
         HeaderFieldValue* mPtr;
   };
   
   Iterator begin() { return Iterator(mList.first); }
   Iterator end() { return Iterator(0); }
   typedef Iterator iterator;
};
}






#endif
