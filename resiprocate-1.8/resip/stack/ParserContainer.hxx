#ifndef RESIP_ParserContainer_hxx
#define RESIP_ParserContainer_hxx

#include <algorithm>
#include <iterator>

#include "resip/stack/HeaderFieldValueList.hxx"
#include "resip/stack/ParserContainerBase.hxx"

namespace resip
{
using std::ptrdiff_t;
/**
   @brief Container class for ParserCategory, used by SipMessage to represent
      multi-valued headers (Contact, Via, etc).

   This has an interface that is similar to stl containers, but not as complete.

   @ingroup resip_crit
*/
template<class T>
class ParserContainer : public ParserContainerBase
{
   public:
      typedef T value_type;
      typedef value_type* pointer;
      typedef const value_type* const_pointer;
      typedef value_type& reference;
      typedef const value_type& const_reference;
      typedef ptrdiff_t difference_type;

      /**
         @brief Default c'tor.
      */
      ParserContainer()
         : ParserContainerBase(Headers::UNKNOWN)
      {}
      
      ParserContainer(PoolBase& pool)
         : ParserContainerBase(Headers::UNKNOWN,pool)
      {}
      
      /** 
         @internal
         @brief Used by SipMessage (using this carries a high risk of blowing 
            your feet off).
      */
      ParserContainer(HeaderFieldValueList* hfvs,
                      Headers::Type type = Headers::UNKNOWN)
         : ParserContainerBase(type)
      {
         mParsers.reserve(hfvs->size());
         for (HeaderFieldValueList::iterator i = hfvs->begin();
              i != hfvs->end(); i++)
         {
            // create, store without copying -- 
            // keeps the HeaderFieldValue from reallocating its buffer
            mParsers.push_back(HeaderKit::Empty);
            mParsers.back().hfv.init(i->getBuffer(),i->getLength(),false);
         }
      }

      ParserContainer(HeaderFieldValueList* hfvs,
                      Headers::Type type,
                      PoolBase& pool)
         : ParserContainerBase(type,pool)
      {
         mParsers.reserve(hfvs->size());
         for (HeaderFieldValueList::iterator i = hfvs->begin();
              i != hfvs->end(); i++)
         {
            // create, store without copying -- 
            // keeps the HeaderFieldValue from reallocating its buffer
            mParsers.push_back(HeaderKit::Empty);
            mParsers.back().hfv.init(i->getBuffer(),i->getLength(),false);
         }
      }

      /**
         @brief Copy c'tor.
      */
      ParserContainer(const ParserContainer& other)
         : ParserContainerBase(other)
      {}

      /**
         @brief Copy c'tor.
      */
      ParserContainer(const ParserContainer& other, PoolBase& pool)
         : ParserContainerBase(other, pool)
      {}

      /**
         @brief Assignment operator.
      */
      ParserContainer& operator=(const ParserContainer& other)
      {
         return static_cast<ParserContainer&>(ParserContainerBase::operator=(other));
      }

      /**
         @brief Returns the first header field value in this container.
      */
      T& front() 
      {
         return ensureInitialized(mParsers.front(),this);
      }
      
      /**
         @brief Returns the last header field value in this container.
      */
      T& back() 
      { 
         return ensureInitialized(mParsers.back(),this);
      }
      
      /**
         @brief Returns the first header field value in this container.
      */
      const T& front() const 
      { 
         return ensureInitialized(mParsers.front(),this);
      }
      
      /**
         @brief Returns the last header field value in this container.
      */
      const T& back() const 
      { 
         return ensureInitialized(mParsers.back(),this);
      }
      
      /**
         @brief Inserts a header field value at the front of this container.
      */
      void push_front(const T & t) 
      { 
         mParsers.insert(mParsers.begin(), HeaderKit::Empty);
         mParsers.front().pc=makeParser(t);
      }

      /**
         @brief Inserts a header field value at the back of this container.
      */
      void push_back(const T & t) 
      { 
         mParsers.push_back(HeaderKit::Empty);
         mParsers.back().pc=makeParser(t);
      }
            
      /**
         @brief Returns a copy of this ParserContainer, in reverse order.
         @todo !bwc! optimize this (we are copying each ParserContainer twice)
      */
      ParserContainer reverse() const
      {
         ParserContainer tmp(*this);
         std::reverse(tmp.mParsers.begin(), tmp.mParsers.end());
         return tmp;
      }

      typedef ParserContainerBase::Parsers Parsers;
      // .dlb. these can be partially hoisted as well
      class const_iterator;
      
      /**
         @brief An iterator class, derived from std::iterator (bidirectional)
      */
      class iterator : public std::iterator<std::bidirectional_iterator_tag, T>
      {
         public:
            iterator(typename Parsers::iterator i,ParserContainer* ref) : mIt(i),mRef(ref){}
            iterator() : mRef(0) {}
            iterator(const iterator& orig) : mIt(orig.mIt), mRef(orig.mRef) {}

            iterator operator++() {iterator it(++mIt,mRef); return it;}
            iterator operator++(int) {iterator it(mIt++,mRef); return it;}
            iterator operator--() {iterator it(--mIt,mRef); return it;}
            iterator operator--(int) {iterator it(mIt--,mRef); return it;}
            bool operator!=(const iterator& rhs) { return mIt != rhs.mIt; }
            bool operator==(const iterator& rhs) { return mIt == rhs.mIt; }
            bool operator!=(const const_iterator& rhs) { return mIt != rhs.mIt; }
            bool operator==(const const_iterator& rhs) { return mIt == rhs.mIt; }
            iterator& operator=(const iterator& rhs) 
            {
               mIt = rhs.mIt; 
               mRef = rhs.mRef;
               return *this;
            }
            T& operator*() {return ensureInitialized(*mIt,mRef);}
            T* operator->() {return &ensureInitialized(*mIt,mRef);}
         private:
            typename Parsers::iterator mIt;
            ParserContainer* mRef;
            friend class const_iterator;
            friend class ParserContainer;
      };

      /**
         @brief A const_iterator class, derived from std::iterator 
            (bidirectional)
      */
      class const_iterator : public std::iterator<std::bidirectional_iterator_tag, T>
      {
         public:
            const_iterator(Parsers::const_iterator i,const ParserContainer* ref) : mIt(i),mRef(ref){}
            const_iterator(const const_iterator& orig) : mIt(orig.mIt), mRef(orig.mRef) {}
            const_iterator(const iterator& orig) : mIt(orig.mIt), mRef(orig.mRef) {}
            const_iterator() : mRef(0) {}

            const_iterator operator++() {const_iterator it(++mIt,mRef); return it;}
            const_iterator operator++(int) {const_iterator it(mIt++,mRef); return it;}
            const_iterator operator--() {const_iterator it(--mIt,mRef); return it;}
            const_iterator operator--(int) {const_iterator it(mIt--,mRef); return it;}
            bool operator!=(const const_iterator& rhs) { return mIt != rhs.mIt; }
            bool operator==(const const_iterator& rhs) { return mIt == rhs.mIt; }
            bool operator!=(const iterator& rhs) { return mIt != rhs.mIt; }
            bool operator==(const iterator& rhs) { return mIt == rhs.mIt; }
            const_iterator& operator=(const const_iterator& rhs) 
            {
               mIt = rhs.mIt;
               mRef = rhs.mRef;
               return *this;
            }
            const_iterator& operator=(const iterator& rhs) 
            {
               mIt = rhs.mIt; 
               mRef = rhs.mRef;
               return *this;
            }
            const T& operator*() {return ensureInitialized(*mIt,mRef);}
            const T* operator->() {return &ensureInitialized(*mIt,mRef);}
         private:
            friend class iterator;
            typename Parsers::const_iterator mIt;
            const ParserContainer* mRef;
      };

      /**
         @brief Returns an iterator pointing to the first header field value.
      */
      iterator begin() { return iterator(mParsers.begin(),this); }

      /**
         @brief Returns an iterator pointing to the last header field value.
      */
      iterator end() { return iterator(mParsers.end(),this); }

      /**
         @brief Erases the header field value pointed to by i. Invalidates all
            existing iterators.
      */
      iterator erase(iterator i)
      {
         freeParser(*i.mIt);
         return iterator(mParsers.erase(i.mIt),this);
      }

      /**
         @brief Finds the first header field value that matches rhs.
      */
      bool find(const T& rhs) const
      {
         for (typename Parsers::const_iterator i = mParsers.begin();
              i != mParsers.end(); ++i)
         {
            // operator== defined by default, but often not usefully
            if (rhs.isEqual(ensureInitialized(*i,this)))
            {
               return true;
            }
         }

         return false;
      }

      /**
         @brief Triggers a parse of all contained header field values.
         @throw ParseException if any header field value is malformed.
      */
      virtual void parseAll()
      {
         for (typename Parsers::const_iterator i = mParsers.begin();
              i != mParsers.end(); ++i)
         {
            ensureInitialized(*i,this).checkParsed();
         }
      }

      /**
         @brief Returns a const_iterator pointing to the first header field 
            value.
      */
      const_iterator begin() const { return const_iterator(mParsers.begin(),this); }

      /**
         @brief Returns a const_iterator pointing to the first header field 
            value.
      */
      const_iterator end() const { return const_iterator(mParsers.end(),this); }

      /**
         @brief Clones this container, and all contained header field values.
      */
      virtual ParserContainerBase* clone() const
      {
         return new ParserContainer(*this);
      }

   private:
      friend class ParserContainer<T>::iterator;
      friend class ParserContainer<T>::const_iterator;

      /**
         @internal
      */
      static T& ensureInitialized(HeaderKit& kit, ParserContainer* ref)
      {
         if(!kit.pc)
         {
            if(ref)
            {
               PoolBase* pool(ref->mPool);
               kit.pc=new (pool) T(kit.hfv, ref->mType, pool);
            }
            else
            {
               kit.pc=new T(kit.hfv, Headers::NONE);
            }
         }
         return *static_cast<T*>(kit.pc);
      }

      static const T& ensureInitialized(const HeaderKit& kit, 
                                 const ParserContainer* ref)
      {
         if(!kit.pc)
         {
            HeaderKit& nc_kit(const_cast<HeaderKit&>(kit));
            if(ref)
            {
               ParserContainer* nc_ref(const_cast<ParserContainer*>(ref));
               PoolBase* pool(nc_ref->mPool);
               nc_kit.pc=new (pool) T(kit.hfv, ref->mType, pool);
            }
            else
            {
               nc_kit.pc=new T(kit.hfv, Headers::NONE);
            }
         }
         return *static_cast<T*>(kit.pc);
      }
};

template <class T>
EncodeStream&
insert(EncodeStream& s, const resip::ParserContainer<T>& c)
{
   s << "[";
   for (typename resip::ParserContainer <T>::const_iterator i = c.begin();
        i != c.end(); i++) 
   {
      if (i != c.begin()) 
      {
         s << ", ";
      }
      // recurse
      insert(s, *i);
   }
   s << "]";
   return s;
}
 
}

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005
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
