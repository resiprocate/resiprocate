/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#ifndef RESIP_IntrusiveListElement
#define RESIP_IntrusiveListElement

/**
   @file
  @brief Heritable intrusive doubly linked list element templates.

  For a class that is an element in a single list, use like this:

  @code
  class Foo : public IntrusiveListElement<Foo*>
  {
     ...
  };

  Foo* fooHead = new Foo();
  // initialize head to cycle -- represent an empty list
  Foo::makeList(fooHead);
  @endcode

  For a class that is an element of multiple lists, use like this:
  has two independent intrusive lists, named read and write
  @code
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
  @endcode

   @ingroup data_structures
*/

namespace resip
{

/**
   @ingroup data_structures
   @brief Templated wrapper class which acts as a cooperatice double-linked list container.
   @details there is no centralized linked list countainer class, instead linked elements
   intrusively alter each others' previous and next element pointers to maintain proper
   linkage.
*/
template <class P>
class IntrusiveListElement
{
   public:
       /** @brief constructor */
      IntrusiveListElement() 
         : mNext(0),
           mPrev(0)
      {}
      /**
        @brief destructor
        @note calls remove()
        @sa remove()
      */
      virtual ~IntrusiveListElement() 
      {
         remove();
      }

      /**
      @brief make this element into a new list
      @param elem the element to turn into a list
      */
      static P makeList(P elem)
      {
         assert(!elem->IntrusiveListElement<P>::mNext);

         elem->IntrusiveListElement<P>::mPrev = elem;
         elem->IntrusiveListElement<P>::mNext = elem;

         return elem;
      }
      /**
      @brief is the queue empty?
      @return true if the queue is empty
      */
      bool empty() const
      {
         assert(mPrev);
         assert(mNext);

         return static_cast<const IntrusiveListElement<P>*>(mNext) == static_cast<const IntrusiveListElement<P>*>(this);
      }
      
      /**
        @brief iterator for traversing an IntrusiveListElement list
      */
      class iterator
      {
         public:
            /**
            @brief constructor, creates a iterator at a given location
            @param start the initial position of the new iterator
            */
            explicit iterator(const P start)
               : mPos(start)
            {}
            /**
            @brief assignment operator sets the position of this iterator to the position of the rhs
            @param rhs iterator to copy the position of
            @return a reference to the current iterator
            */
            iterator& operator=(const iterator& rhs)
            {
               mPos = rhs.mPos;
               return *this;
            }
            /**
            @brief advances the iterator in the list
            @return a refrence to the the iterator
            */
            iterator& operator++()
            {
               mPos = mPos->IntrusiveListElement<P>::mNext;
               return *this;
            }
            /**
            @brief checks if this iterator is at the same position as another iterator
            @param rhs the iterator to check equality with
            @return true if and only if rhs is at the same postion
            */
            bool operator==(const iterator& rhs)
            {
               return mPos == rhs.mPos;
            }
            /**
            @brief checks if this iterator is at the same position as another iterator
            @param rhs the iterator to check inequality with
            @return true if and only if rhs is at a different postion
            */
            bool operator!=(const iterator& rhs)
            {
               return mPos != rhs.mPos;
            }
            /**
            @brief get the item at the current location
            @return the item at the current location
            */
            P operator*()
            {
               return mPos;
            }

         private:
            P mPos;
      };
      
      /**
      @brief get a start iterator for looping
      @todo rename this to mean next. Perhaps next()?
      @note returns an iterator pointing at the next position in the queue
      @return an iterator for looping through the list
      */
      iterator begin()
      {
         assert(mPrev);
         assert(mNext);
         return iterator(mNext);
      }
      /**
      @brief get a end iterator for looping
      @todo rename this too
      @note returns an iterator pointing to the current position in the queue
      @return an iterator for looping through the list
      */
      iterator end()
      {
         assert(mPrev);
         assert(mNext);
         return iterator(static_cast<P>(this));
      }

      friend class iterator;

      /**
        @brief insert a an element ahead of this element in the list
        @param elem the element to insert
        @note pushing an element onto the same list twice is undefine
      */
      void push_front(P elem)
      {
         assert(mPrev);
         assert(mNext);

         elem->IntrusiveListElement<P>::mNext = mNext;
         elem->IntrusiveListElement<P>::mPrev = static_cast<P>(this);
         
         elem->IntrusiveListElement<P>::mNext->IntrusiveListElement<P>::mPrev = elem;
         elem->IntrusiveListElement<P>::mPrev->IntrusiveListElement<P>::mNext = elem;
      }

      // putting an element onto the same list twice is undefined
      void push_back(P elem)
      {
         assert(mPrev);
         assert(mNext);

         elem->IntrusiveListElement<P>::mPrev = mPrev;
         elem->IntrusiveListElement<P>::mNext = static_cast<P>(this);
         
         elem->IntrusiveListElement<P>::mPrev->IntrusiveListElement<P>::mNext = elem;
         elem->IntrusiveListElement<P>::mNext->IntrusiveListElement<P>::mPrev = elem;
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
            mNext->IntrusiveListElement<P>::mPrev = mPrev;
            mPrev->IntrusiveListElement<P>::mNext = mNext;
         }

         mNext = 0;
         mPrev = 0;
      }

   protected:
      mutable P mNext;
      mutable P mPrev;
};

/**
   @ingroup data_structures
   @brief Mirror of IntrusiveListElement please see IntrusiveListElement for details.
   @sa IntrusiveListElement
*/
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
         assert(!elem->IntrusiveListElement1<P>::mNext);

         elem->IntrusiveListElement1<P>::mPrev = elem;
         elem->IntrusiveListElement1<P>::mNext = elem;

         return elem;
      }

      bool empty() const
      {
         assert(mPrev);
         assert(mNext);

         return static_cast<const IntrusiveListElement1<P>*>(mNext) == static_cast<const IntrusiveListElement1<P>*>(this);
      }

      // .dlb. add reverse_iterator?
      /**
         @brief Mirror of IntrusiveListElement::iterator please see IntrusiveListElement::iterator for details.
         @sa IntrusiveListElement::iterator
      */
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
               mPos = mPos->IntrusiveListElement1<P>::mNext;
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

         elem->IntrusiveListElement1<P>::mNext = mNext;
         elem->IntrusiveListElement1<P>::mPrev = static_cast<P>(this);
         
         elem->IntrusiveListElement1<P>::mNext->IntrusiveListElement1<P>::mPrev = elem;
         elem->IntrusiveListElement1<P>::mPrev->IntrusiveListElement1<P>::mNext = elem;
      }

      // putting an element onto the same list twice is undefined
      void push_back(P elem)
      {
         assert(mPrev);
         assert(mNext);

         elem->IntrusiveListElement1<P>::mPrev = mPrev;
         elem->IntrusiveListElement1<P>::mNext = static_cast<P>(this);
         
         elem->IntrusiveListElement1<P>::mPrev->IntrusiveListElement1<P>::mNext = elem;
         elem->IntrusiveListElement1<P>::mNext->IntrusiveListElement1<P>::mPrev = elem;
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
            mNext->IntrusiveListElement1<P>::mPrev = mPrev;
            mPrev->IntrusiveListElement1<P>::mNext = mNext;
         }

         mNext = 0;
         mPrev = 0;
      }

   protected:
      mutable P mNext;
      mutable P mPrev;
};

/**
   @ingroup data_structures
   @brief Mirror of IntrusiveListElement please see IntrusiveListElement for details.
   @sa IntrusiveListElement
*/
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
         assert(!elem->IntrusiveListElement2<P>::mNext);

         elem->IntrusiveListElement2<P>::mPrev = elem;
         elem->IntrusiveListElement2<P>::mNext = elem;

         return elem;
      }

      bool empty() const
      {
         assert(mPrev);
         assert(mNext);

         return static_cast<const IntrusiveListElement2<P>*>(mNext) == static_cast<const IntrusiveListElement2<P>*>(this);
      }

      // .dlb. add reverse_iterator?

      /**
         @brief Mirror of IntrusiveListElement::iterator please see IntrusiveListElement::iterator for details.
         @sa IntrusiveListElement::iterator
      */
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
               mPos = mPos->IntrusiveListElement2<P>::mNext;
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

         elem->IntrusiveListElement2<P>::mNext = mNext;
         elem->IntrusiveListElement2<P>::mPrev = static_cast<P>(this);
         
         elem->IntrusiveListElement2<P>::mNext->IntrusiveListElement2<P>::mPrev = elem;
         elem->IntrusiveListElement2<P>::mPrev->IntrusiveListElement2<P>::mNext = elem;
      }

      // putting an element onto the same list twice is undefined
      void push_back(P elem)
      {
         assert(mPrev);
         assert(mNext);

         elem->IntrusiveListElement2<P>::mPrev = mPrev;
         elem->IntrusiveListElement2<P>::mNext = static_cast<P>(this);
         
         elem->IntrusiveListElement2<P>::mPrev->IntrusiveListElement2<P>::mNext = elem;
         elem->IntrusiveListElement2<P>::mNext->IntrusiveListElement2<P>::mPrev = elem;
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
            mNext->IntrusiveListElement2<P>::mPrev = mPrev;
            mPrev->IntrusiveListElement2<P>::mNext = mNext;
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
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
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
