#ifndef ParserContainer_hxx
#define ParserContainer_hxx

#include "sip2/sipstack/HeaderFieldValueList.hxx"
#include "sip2/sipstack/ParserContainerBase.hxx"
#include "sip2/sipstack/Symbols.hxx"
#include "sip2/sipstack/Embedded.hxx"
#include <cassert>
#include <list>

namespace Vocal2
{

template<class T>
class ParserContainer : public ParserContainerBase
{
   public:
      typedef T value_type;
      typedef value_type* pointer;
      typedef const value_type* const_pointer;
      typedef value_type& reference;
      typedef const value_type& const_reference;
      typedef size_t size_type;
      typedef ptrdiff_t difference_type;

      ParserContainer()
         : ParserContainerBase(Headers::UNKNOWN)
      {}
      
      // private to SipMessage
      ParserContainer(HeaderFieldValueList* hfvs,
                      Headers::Type type = Headers::UNKNOWN)
         : ParserContainerBase(type)
      {
         for (HeaderFieldValueList::iterator i = hfvs->begin();
              i != hfvs->end(); i++)
         {
            // create, store without copying -- 
            // keeps the HeaderFieldValue from reallocating its buffer
            mParsers.push_back(new T(*i, type));
         }
      }

      ParserContainer(const ParserContainer& other)
         : ParserContainerBase(other)
      {
         for (typename std::list<T*>::const_iterator i = other.mParsers.begin(); 
              i != other.mParsers.end(); i++)
         {
            mParsers.push_back(new T(**i));
         }
      }

      ~ParserContainer()
      {
         clear();
      }
      
      ParserContainer& operator=(const ParserContainer& other)
      {
         if (this != &other)
         {
            clear();
            for (typename std::list<T*>::const_iterator i = other.mParsers.begin(); 
                 i != other.mParsers.end(); i++)
            {
               mParsers.push_back(new T(**i));
            }
         }
         return *this;
      }
      
      bool empty() const { return mParsers.empty(); }
      size_type size() const { return mParsers.size(); }
      void clear()
      {
         for (typename std::list<T*>::const_iterator i = mParsers.begin(); 
              i != mParsers.end(); i++)
         {
            delete *i;
         }
         mParsers.clear();
      }
      
      T& front() { return *mParsers.front();}
      T& back() { return *mParsers.back();}
      const T& front() const { return *mParsers.front();}
      const T& back() const { return *mParsers.back();}
      
      void push_front(const T & t) { mParsers.push_front(new T(t)); }
      void push_back(const T & t) { mParsers.push_back(new T(t)); }
      
      void pop_front() { delete mParsers.front(); mParsers.pop_front(); }
      void pop_back() { delete mParsers.back(); mParsers.pop_back(); }
      
      ParserContainer reverse()
      {
         ParserContainer tmp(*this);
         tmp.mParsers.reverse();
         return tmp;
      }
      
      class const_iterator;
      
      class iterator
      {
         public:
            iterator(typename std::list<T*>::iterator i)
               : mIt(i)
            {}

            iterator operator++() {iterator it(++mIt); return it;}
            iterator operator++(int) {iterator it(mIt++); return it;}
            iterator operator--() {iterator it(--mIt); return it;}
            iterator operator--(int) {iterator it(mIt--); return it;}
            bool operator!=(const iterator& rhs) { return mIt != rhs.mIt; }
            bool operator==(const iterator& rhs) { return mIt == rhs.mIt; }
            bool operator!=(const const_iterator& rhs) { return mIt != rhs.mIt; }
            bool operator==(const const_iterator& rhs) { return mIt == rhs.mIt; }
            iterator& operator=(const iterator& rhs) { mIt = rhs.mIt; return *this;}
            T& operator*() {return **mIt;}
            T* operator->() {return *mIt;}
         private:
            typename std::list<T*>::iterator mIt;
            friend class const_iterator;
      };

      class const_iterator
      {
         public:
            const_iterator(typename std::list<T*>::const_iterator i)
               : mIt(i)
            {}

            const_iterator operator++() {const_iterator it(++mIt); return it;}
            const_iterator operator++(int) {const_iterator it(mIt++); return it;}
            const_iterator operator--() {const_iterator it(--mIt); return it;}
            const_iterator operator--(int) {const_iterator it(mIt--); return it;}
            bool operator!=(const const_iterator& rhs) { return mIt != rhs.mIt; }
            bool operator==(const const_iterator& rhs) { return mIt == rhs.mIt; }
            bool operator!=(const iterator& rhs) { return mIt != rhs.mIt; }
            bool operator==(const iterator& rhs) { return mIt == rhs.mIt; }
            const_iterator& operator=(const const_iterator& rhs) { mIt = rhs.mIt; return *this;}
            const T& operator*() {return **mIt;}
            const T* operator->() {return *mIt;}
         private:
            friend class iterator;
            typename std::list<T*>::const_iterator mIt;
      };
      
      iterator begin() { return iterator(mParsers.begin()); }
      iterator end() { return iterator(mParsers.end()); }

      const_iterator begin() const { return const_iterator(mParsers.begin()); }
      const_iterator end() const { return const_iterator(mParsers.end()); }

      virtual ParserContainerBase* clone() const
      {
         return new ParserContainer(*this);
      }

      virtual std::ostream& encode(const Data& headerName, std::ostream& str) const
      {
         for (typename std::list<T*>::const_iterator i = mParsers.begin(); 
              i != mParsers.end(); i++)
         {
            if (!headerName.empty())
            {
               str << headerName << Symbols::COLON << Symbols::SPACE;
            }

            (*i)->encode(str);

            str << Symbols::CRLF;
         }
         return str;
      }

      virtual std::ostream& encodeEmbedded(const Data& headerName, std::ostream& str) const
      {
         assert(!headerName.empty());

         bool first = true;
         for (typename std::list<T*>::const_iterator i = mParsers.begin(); 
              i != mParsers.end(); i++)
         {
            if (first)
            {
               first = false;
            }
            else
            {
               str << Symbols::AMPERSAND;
            }

            str << headerName << Symbols::EQUALS;
            Data buf;
            {
               DataStream s(buf);
               (*i)->encode(s);
            }
            str << Embedded::encode(buf);
         }
         return str;
      }

   private:
      std::list<T*> mParsers;
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
