#ifndef RESIP_IntrusiveListElement
#define RESIP_IntrusiveListElement

/*
  Heritable intrusive doubly linked list element template.

  // For a class that is an element in a single list, use like this:
  class Foo : public IntrusiveListElement<Foo*>
  {
     ...
  };

  Foo* fooHead = new Foo();
  // initialize head to cycle -- represent an empty list
  Foo::makeList(fooHead);

  // For a class that is an element of multiple lists, use like this:
  // has two independent intrusive lists aspects, named read and write
  class FooFoo : public IntrusiveListElement<Foo*, 1>, public IntrusiveListElement<Foo*, 2>
  {
     public:
        typedef IntrusiveListElement<FooFoo*, 1> read;
        typedef IntrusiveListElement<FooFoo*, 2> write;

     ...
  };

  FooFoo* fooFooHead = new FooFoo();
  // initialize head to cycle -- represent two empty lists
  FooFoo::read::makeList(fooFooHead);
  FooFoo::write::makeList(fooFooHead);
  for (FooFoo::read::iterator f = fooFooHead->read::begin(); f != fooFooHead->read::end(); f++)
  {
     ...
  };

  // elsewhere:
  FooFoo head;
  FooFoo::read* mReadHead = FooFoo::read::makeList(&head);
  FooFoo::write* mWriteHead = FooFoo::write::makeList(&head);

  FooFoo element* = new FooFoo();
  // don't need to disambiguate methods
  mReadHead->push_back(element);
  mWriteHead->push_back(element);

  // element could be in either list, so use aspect
  element->write::remove();  

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

      // make this element an empty list
      static P makeList(P elem)
      {
         assert(!elem->me::mPrev);
         assert(!elem->me::mNext);

         elem->me::mPrev = elem;
         elem->me::mNext = elem;

         return elem;
      }

      bool empty() const
      {
         assert(me::mPrev);
         assert(me::mNext);

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
         assert(me::mPrev);
         assert(me::mNext);
         return iterator(me::mNext);
      }

      iterator end()
      {
         assert(me::mPrev);
         assert(me::mNext);
         return iterator(static_cast<P>(this));
      }

      friend class iterator;

      // pushing an element onto the same list twice is undefined
      void push_front(P elem)
      {
         assert(me::mPrev);
         assert(me::mNext);

         elem->me::mNext = me::mNext;
         elem->me::mPrev = static_cast<P>(this);
         
         elem->me::mNext->me::mPrev = elem;
         elem->me::mPrev->me::mNext = elem;
      }

      // putting an element onto the same list twice is undefined
      void push_back(P elem)
      {
         assert(me::mPrev);
         assert(me::mNext);

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
