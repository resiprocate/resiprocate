#ifndef ParserContainer_hxx
#define ParserContainer_hxx

#include <sipstack/HeaderFieldValueList.hxx>
#include <sipstack/ParserContainerBase.hxx>
#include <list>

namespace Vocal2
{

template<class T>
class ParserContainer : public ParserContainerBase
{
   public:
      ParserContainer() {}
      
      // private to SipMessage
      ParserContainer(HeaderFieldValueList* hfvs)
      {
         for (HeaderFieldValueList::iterator i = hfvs->begin();
              i != hfvs->end(); i++)
         {
            mParsers.push_back(T(*i));
         }
      }

      ParserContainer(const ParserContainer& other)
      {
         for (typename std::list<T>::const_iterator i = other.mParsers.begin(); 
              i != other.mParsers.end(); i++)
         {
            mParsers.push_back(*i);
         }
      }
      
      ParserContainer& operator=(const ParserContainer& other)
      {
         if (this != &other)
         {
            mParsers.clear();
            for (typename std::list<T>::const_iterator i = other.mParsers.begin(); 
                 i != other.mParsers.end(); i++)
            {
               push_back(*i);
            }
         }
         return *this;
      }
      
      bool empty() const { return mList->empty(); }
      int size() const { return mParsers.size(); }
      void clear() {mParsers.clear();}
      
      T& front() { return mParsers.front();}
      T& back() { return mParsers.back();}
      
      void push_front(const T & t) { mParsers.push_front(t); }
      void push_back(const T & t) { mParsers.push_back(t); }
      
      void pop_front() { mParsers.pop_front(); }
      void pop_back() { mParsers.pop_back(); }
      
      ParserContainer reverse()
      {
         ParserContainer tmp(*this);
         tmp.mParsers.reverse();
         return tmp;
      }
      
      typedef typename std::list<T>::iterator iterator;
      iterator begin() { return mParsers.begin(); }
      iterator end() { return mParser.end(); }

      virtual ParserContainerBase* clone() const
      {
         return new ParserContainer(*this);
      }

      virtual std::ostream& encode(std::ostream& str) const
      {
         for (typename std::list<T>::const_iterator i = mParsers.begin(); 
              i != mParsers.end(); i++)
         {
            if (i->isParsed())
            {
               i->encode(str);
            }
            else
            {
               i->encodeFromHeaderFieldValue(str);
            }
            str << Symbols::CRLF;
         }
         return str;
      }

   private:
      std::list<T> mParsers;
};
 
}

#endif
