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
  // has two independent intrusive lists, named read and write
  class FooFoo : public IntrusiveListElement<FooFoo*>, public IntrusiveListElement1<FooFoo*>
  {
     public:
        typedef IntrusiveListElement<FooFoo*> read;
        typedef IntrusiveListElement1<FooFoo*> write;

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

namespace resip
{

template <class P>
class IntrusiveListElement
{
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
         assert(!elem->IntrusiveListElement::mNext);

         elem->IntrusiveListElement::mPrev = elem;
         elem->IntrusiveListElement::mNext = elem;

         return elem;
      }

      bool empty() const
      {
         assert(mPrev);
         assert(mNext);

         return mNext == static_cast<P>(const_cast<IntrusiveListElement<P>*>(this));
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
               mPos = mPos->IntrusiveListElement::mNext;
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
         assert(mPrev);
         assert(mNext);
         return iterator(mNext);
      }

      iterator end()
      {
         assert(mPrev);
         assert(mNext);
         return iterator(static_cast<P>(this));
      }

      friend class iterator;

      // pushing an element onto the same list twice is undefined
      void push_front(P elem)
      {
         assert(mPrev);
         assert(mNext);

         elem->IntrusiveListElement::mNext = mNext;
         elem->IntrusiveListElement::mPrev = static_cast<P>(this);
         
         elem->IntrusiveListElement::mNext->IntrusiveListElement::mPrev = elem;
         elem->IntrusiveListElement::mPrev->IntrusiveListElement::mNext = elem;
      }

      // putting an element onto the same list twice is undefined
      void push_back(P elem)
      {
         assert(mPrev);
         assert(mNext);

         elem->IntrusiveListElement::mPrev = mPrev;
         elem->IntrusiveListElement::mNext = static_cast<P>(this);
         
         elem->IntrusiveListElement::mPrev->IntrusiveListElement::mNext = elem;
         elem->IntrusiveListElement::mNext->IntrusiveListElement::mPrev = elem;
      }

      void remove()
      {
         if (mNext)
         {
            // prev  -> this -> next
            //       <-      <-
            //
            // prev -> next
            //      <-
            mNext->IntrusiveListElement::mPrev = mPrev;
            mPrev->IntrusiveListElement::mNext = mNext;
         }

         mNext = 0;
         mPrev = 0;
      }

   protected:
      mutable P mNext;
      mutable P mPrev;
};

template <class P>
class IntrusiveListElement1
{
   public:
      IntrusiveListElement1() 
         : mNext(0),
           mPrev(0)
      {}

      virtual ~IntrusiveListElement1() 
      {
         remove();
      }

      // make this element an empty list
      static P makeList(P elem)
      {
         assert(!elem->IntrusiveListElement1::mNext);

         elem->IntrusiveListElement1::mPrev = elem;
         elem->IntrusiveListElement1::mNext = elem;

         return elem;
      }

      bool empty() const
      {
         assert(mPrev);
         assert(mNext);

         return mNext == static_cast<P>(const_cast<IntrusiveListElement1<P>*>(this));
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
               mPos = mPos->IntrusiveListElement1::mNext;
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
         assert(mPrev);
         assert(mNext);
         return iterator(mNext);
      }

      iterator end()
      {
         assert(mPrev);
         assert(mNext);
         return iterator(static_cast<P>(this));
      }

      friend class iterator;

      // pushing an element onto the same list twice is undefined
      void push_front(P elem)
      {
         assert(mPrev);
         assert(mNext);

         elem->IntrusiveListElement1::mNext = mNext;
         elem->IntrusiveListElement1::mPrev = static_cast<P>(this);
         
         elem->IntrusiveListElement1::mNext->IntrusiveListElement1::mPrev = elem;
         elem->IntrusiveListElement1::mPrev->IntrusiveListElement1::mNext = elem;
      }

      // putting an element onto the same list twice is undefined
      void push_back(P elem)
      {
         assert(mPrev);
         assert(mNext);

         elem->IntrusiveListElement1::mPrev = mPrev;
         elem->IntrusiveListElement1::mNext = static_cast<P>(this);
         
         elem->IntrusiveListElement1::mPrev->IntrusiveListElement1::mNext = elem;
         elem->IntrusiveListElement1::mNext->IntrusiveListElement1::mPrev = elem;
      }

      void remove()
      {
         if (mNext)
         {
            // prev  -> this -> next
            //       <-      <-
            //
            // prev -> next
            //      <-
            mNext->IntrusiveListElement1::mPrev = mPrev;
            mPrev->IntrusiveListElement1::mNext = mNext;
         }

         mNext = 0;
         mPrev = 0;
      }

   protected:
      mutable P mNext;
      mutable P mPrev;
};

template <class P>
class IntrusiveListElement2
{
   public:
      IntrusiveListElement2() 
         : mNext(0),
           mPrev(0)
      {}

      virtual ~IntrusiveListElement2() 
      {
         remove();
      }

      // make this element an empty list
      static P makeList(P elem)
      {
         assert(!elem->IntrusiveListElement2::mNext);

         elem->IntrusiveListElement2::mPrev = elem;
         elem->IntrusiveListElement2::mNext = elem;

         return elem;
      }

      bool empty() const
      {
         assert(mPrev);
         assert(mNext);

         return mNext == static_cast<P>(const_cast<IntrusiveListElement2<P>*>(this));
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
               mPos = mPos->IntrusiveListElement2::mNext;
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
         assert(mPrev);
         assert(mNext);
         return iterator(mNext);
      }

      iterator end()
      {
         assert(mPrev);
         assert(mNext);
         return iterator(static_cast<P>(this));
      }

      friend class iterator;

      // pushing an element onto the same list twice is undefined
      void push_front(P elem)
      {
         assert(mPrev);
         assert(mNext);

         elem->IntrusiveListElement2::mNext = mNext;
         elem->IntrusiveListElement2::mPrev = static_cast<P>(this);
         
         elem->IntrusiveListElement2::mNext->IntrusiveListElement2::mPrev = elem;
         elem->IntrusiveListElement2::mPrev->IntrusiveListElement2::mNext = elem;
      }

      // putting an element onto the same list twice is undefined
      void push_back(P elem)
      {
         assert(mPrev);
         assert(mNext);

         elem->IntrusiveListElement2::mPrev = mPrev;
         elem->IntrusiveListElement2::mNext = static_cast<P>(this);
         
         elem->IntrusiveListElement2::mPrev->IntrusiveListElement2::mNext = elem;
         elem->IntrusiveListElement2::mNext->IntrusiveListElement2::mPrev = elem;
      }

      void remove()
      {
         if (mNext)
         {
            // prev  -> this -> next
            //       <-      <-
            //
            // prev -> next
            //      <-
            mNext->IntrusiveListElement2::mPrev = mPrev;
            mPrev->IntrusiveListElement2::mNext = mNext;
         }

         mNext = 0;
         mPrev = 0;
      }

   protected:
      mutable P mNext;
      mutable P mPrev;
};
 
}

#endif


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
