#ifndef RESIP_IntrusiveListElement
#define RESIP_IntrusiveListElement

/*
  Heritable intrusive doubly linked list element template.

  For a class that is an element in a single list, use like this:
  class Foo : public IntrusiveListElement<Foo*>
  {
     ...
  };

  Foo* fooHead = new Foo(-1);
  // initialize header to cycle -- represent an empty list
  fooHead->init();

  For a class that is an element of multiple lists, use like this:
  // has two independent intrusive lists, named reader and writer
  class FooFoo : public IntrusiveListElement<Foo*, 1>, public IntrusiveListElement<Foo*, 2>
  {
     public:
        typedef IntrusiveListElement<FooFoo*, 1> reader;
        typedef IntrusiveListElement<FooFoo*, 2> writer;

     ...
  };

  FooFoo* fooFooHead = new FooFoo(-1);
  // initialize header to cycle -- represent two empty lists
  fooFooHead->reader::push_front(fooFooHead);
  fooFooHead->writer::push_front(fooFooHead);
  for (FooFoo::reader::iterator f = fooFooHead->reader::begin(); f != fooFooHead->reader::end(); f++)
  {
     ...
  };

  // elsewhere:
  FooFoo::reader* mReadHead;
  FooFoo::writer* mWriteHead;

  mReadHead->init(); // use methods directly
  mWriteHead->init(); // use methods directly

  FooFoo element* = new FooFoo();
  mReadHead->push_back(element);
  mWriteHead->push_back(element);

  element->writer::remove();  // use named methods

*/

template <class P, int N=0>
class IntrusiveListElement
{
      // .dlb. should not be necessary -- compiler issue?
      typedef IntrusiveListElement<P,N> me;
   public:
      IntrusiveListElement() 
         : mNext(0),
           mPrev(0)
      {}

      virtual ~IntrusiveListElement() 
      {
         remove();
      }

      void init()
      {
         mPrev = static_cast<P>(this);
         mNext = static_cast<P>(this);
      }

      bool empty() const
      {
         return me::mNext == static_cast<P>(const_cast<me*>(this));
      }

      // .dlb. add reverse_iterator?

      class iterator
      {
         public:
            explicit iterator(const P start)
               : mPos(start)
            {}

            iterator& operator=(const iterator& rhs)
            {
               mPos = rhs.mPos;
               return *this;
            }

            iterator& operator++()
            {
               mPos = mPos->me::mNext;
               return *this;
            }

            bool operator==(const iterator& rhs)
            {
               return mPos == rhs.mPos;
            }

            bool operator!=(const iterator& rhs)
            {
               return mPos != rhs.mPos;
            }

            P operator*()
            {
               return mPos;
            }

         private:
            P mPos;
      };

      iterator begin()
      {
         return iterator(me::mNext);
      }

      iterator end()
      {
         return iterator(static_cast<P>(this));
      }

      friend class iterator;

      // pushing an element onto the same list twice is undefined
      void push_front(P elem)
      {
         elem->me::mNext = me::mNext;
         elem->me::mPrev = static_cast<P>(this);
         
         elem->me::mNext->me::mPrev = elem;
         elem->me::mPrev->me::mNext = elem;
      }

      // putting an element onto the same list twice is undefined
      void push_back(P elem)
      {
         elem->me::mPrev = me::mPrev;
         elem->me::mNext = static_cast<P>(this);
         
         elem->me::mPrev->me::mNext = elem;
         elem->me::mNext->me::mPrev = elem;
      }

      void remove()
      {
         if (me::mNext)
         {
            // prev  -> this -> next
            //       <-      <-
            //
            // prev -> next
            //      <-
            me::mNext->me::mPrev = me::mPrev;
            me::mPrev->me::mNext = me::mNext;
         }

         me::mNext = 0;
         me::mPrev = 0;
      }

   private:
      mutable P mNext;
      mutable P mPrev;
};

#endif
