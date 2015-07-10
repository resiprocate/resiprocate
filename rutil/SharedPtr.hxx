#if !defined(RESIP_SHAREDPTR_HXX)
#define RESIP_SHAREDPTR_HXX

/**
   @file
   @brief Defines a reference-counted pointer class.
   @note This implementation is a modified version of shared_ptr from
   Boost.org.  License text is below.

   http://www.boost.org/libs/smart_ptr/shared_ptr.htm
*/

#include "rutil/SharedCount.hxx"
#include <memory>               // for std::auto_ptr
#include <algorithm>            // for std::swap
#include <functional>           // for std::less
#include <typeinfo>             // for std::bad_cast
#include <iosfwd>               // for std::basic_ostream
#include "rutil/ResipAssert.h"

namespace resip
{

template<class T> class enable_shared_from_this;

struct static_cast_tag {};
struct const_cast_tag {};
struct dynamic_cast_tag {};
struct polymorphic_cast_tag {};

template<class T> struct SharedPtr_traits
{
   typedef T & reference;
};

template<> struct SharedPtr_traits<void>
{
   typedef void reference;
};

template<> struct SharedPtr_traits<void const>
{
   typedef void reference;
};

template<> struct SharedPtr_traits<void volatile>
{
   typedef void reference;
};

template<> struct SharedPtr_traits<void const volatile>
{
   typedef void reference;
};

// enable_shared_from_this support

template<class T, class Y> void sp_enable_shared_from_this( shared_count const & pn, resip::enable_shared_from_this<T> const * pe, Y const * px )
{
   if(pe != 0) pe->_internal_weak_this._internal_assign(const_cast<Y*>(px), pn);
}

inline void sp_enable_shared_from_this( shared_count const & /*pn*/, ... )
{
}

/**
   @brief Implements reference counted copy semantics for a class T.

   The object pointed to is deleted when the last SharedPtr pointing to it
   is destroyed or reset.
*/
template<class T> class SharedPtr
{
private:

   // Borland 5.5.1 specific workaround
   typedef SharedPtr<T> this_type;

public:

   typedef T element_type;
   typedef T value_type;
   typedef T * pointer;
   typedef typename SharedPtr_traits<T>::reference reference;

   SharedPtr(): px(0), pn() // never throws in 1.30+
   {
   }

   template<class Y>
   explicit SharedPtr(Y * p): px(p), pn(p, checked_deleter<Y>()) // Y must be complete
   {
       sp_enable_shared_from_this( pn, p, p );
   }

   //
   // Requirements: D's copy constructor must not throw
   //
   // SharedPtr will release p by calling d(p)
   //

   template<class Y, class D> SharedPtr(Y * p, D d): px(p), pn(p, d)
   {
       sp_enable_shared_from_this( pn, p, p );
   }

//  generated copy constructor, assignment, destructor are fine...

//  except that Borland C++ has a bug, and g++ with -Wsynth warns
#if defined(__BORLANDC__) || defined(__GNUC__)
   SharedPtr & operator=(SharedPtr const & r) // never throws
   {
       px = r.px;
       pn = r.pn; // shared_count::op= doesn't throw
       return *this;
   }
#endif

   template<class Y>
   SharedPtr(SharedPtr<Y> const & r): px(r.px), pn(r.pn) // never throws
   {
   }

   template<class Y>
   SharedPtr(SharedPtr<Y> const & r, static_cast_tag): px(static_cast<element_type *>(r.px)), pn(r.pn)
   {
   }

   template<class Y>
   SharedPtr(SharedPtr<Y> const & r, const_cast_tag): px(const_cast<element_type *>(r.px)), pn(r.pn)
   {
   }

   template<class Y>
   SharedPtr(SharedPtr<Y> const & r, dynamic_cast_tag): px(dynamic_cast<element_type *>(r.px)), pn(r.pn)
   {
      if(px == 0) // need to allocate new counter -- the cast failed
      {
         pn = resip::shared_count();
      }
   }

   template<class Y>
   SharedPtr(SharedPtr<Y> const & r, polymorphic_cast_tag): px(dynamic_cast<element_type *>(r.px)), pn(r.pn)
   {
      if(px == 0)
      {
         throw std::bad_cast();
      }
   }

   template<class Y>
   explicit SharedPtr(std::auto_ptr<Y> & r): px(r.get()), pn()
   {
      Y * tmp = r.get();
      pn = shared_count(r);
      sp_enable_shared_from_this( pn, tmp, tmp );
   }

   template<class Y>
   SharedPtr & operator=(SharedPtr<Y> const & r) // never throws
   {
      px = r.px;
      pn = r.pn; // shared_count::op= doesn't throw
      return *this;
   }

   template<class Y>
   SharedPtr & operator=(std::auto_ptr<Y> & r)
   {
      this_type(r).swap(*this);
      return *this;
   }

   void reset() // never throws in 1.30+
   {
      this_type().swap(*this);
   }

   template<class Y> void reset(Y * p) // Y must be complete
   {
      resip_assert(p == 0 || p != px); // catch self-reset errors
      this_type(p).swap(*this);
   }

   template<class Y, class D> void reset(Y * p, D d)
   {
      this_type(p, d).swap(*this);
   }

   reference operator* () const // never throws
   {
      resip_assert(px != 0);
      return *px;
   }

   T * operator-> () const // never throws
   {
      resip_assert(px != 0);
      return px;
   }
    
   T * get() const // never throws
   {
      return px;
   }

   // implicit conversion to "bool"
#if defined(__SUNPRO_CC) // BOOST_WORKAROUND(__SUNPRO_CC, <= 0x530)
   operator bool () const
   {
      return px != 0;
   }
#elif defined(__MWERKS__) // BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3003))
   typedef T * (this_type::*unspecified_bool_type)() const;
   operator unspecified_bool_type() const // never throws
   {
      return px == 0? 0: &this_type::get;
   }
#else 
   typedef T * this_type::*unspecified_bool_type;
   operator unspecified_bool_type() const // never throws
   {
      return px == 0? 0: &this_type::px;
   }
#endif

   // operator! is redundant, but some compilers need it
   bool operator! () const // never throws
   {
      return px == 0;
   }

   bool unique() const // never throws
   {
      return pn.unique();
   }

   long use_count() const // never throws
   {
      return pn.use_count();
   }

   void swap(SharedPtr<T> & other) // never throws
   {
      std::swap(px, other.px);
      pn.swap(other.pn);
   }

   template<class Y> bool _internal_less(SharedPtr<Y> const & rhs) const
   {
      return pn < rhs.pn;
   }

   void * _internal_get_deleter(std::type_info const & ti) const
   {
      return pn.get_deleter(ti);
   }

// Tasteless as this may seem, making all members public allows member templates
// to work in the absence of member template friends. (Matthew Langston)

private:

   template<class Y> friend class SharedPtr;

   T * px;                     // contained pointer
   shared_count pn;    // reference counter

};  // SharedPtr

template<class T, class U> inline bool operator==(SharedPtr<T> const & a, SharedPtr<U> const & b)
{
    return a.get() == b.get();
}

template<class T, class U> inline bool operator!=(SharedPtr<T> const & a, SharedPtr<U> const & b)
{
    return a.get() != b.get();
}

#if __GNUC__ == 2 && __GNUC_MINOR__ <= 96

// Resolve the ambiguity between our op!= and the one in rel_ops

template<class T> inline bool operator!=(SharedPtr<T> const & a, SharedPtr<T> const & b)
{
    return a.get() != b.get();
}

#endif

template<class T, class U> inline bool operator<(SharedPtr<T> const & a, SharedPtr<U> const & b)
{
    return a._internal_less(b);
}

template<class T> inline void swap(SharedPtr<T> & a, SharedPtr<T> & b)
{
    a.swap(b);
}

template<class T, class U> SharedPtr<T> static_pointer_cast(SharedPtr<U> const & r)
{
    return SharedPtr<T>(r, static_cast_tag());
}

template<class T, class U> SharedPtr<T> const_pointer_cast(SharedPtr<U> const & r)
{
    return SharedPtr<T>(r, const_cast_tag());
}

template<class T, class U> SharedPtr<T> dynamic_pointer_cast(SharedPtr<U> const & r)
{
    return SharedPtr<T>(r, dynamic_cast_tag());
}

// shared_*_cast names are deprecated. Use *_pointer_cast instead.

template<class T, class U> SharedPtr<T> shared_static_cast(SharedPtr<U> const & r)
{
    return SharedPtr<T>(r, static_cast_tag());
}

template<class T, class U> SharedPtr<T> shared_dynamic_cast(SharedPtr<U> const & r)
{
    return SharedPtr<T>(r, dynamic_cast_tag());
}

template<class T, class U> SharedPtr<T> shared_polymorphic_cast(SharedPtr<U> const & r)
{
    return SharedPtr<T>(r, polymorphic_cast_tag());
}

template<class T, class U> SharedPtr<T> shared_polymorphic_downcast(SharedPtr<U> const & r)
{
    resip_assert(dynamic_cast<T *>(r.get()) == r.get());
    return shared_static_cast<T>(r);
}

template<class T> inline T * get_pointer(SharedPtr<T> const & p)
{
    return p.get();
}

// operator<<
#if defined(__GNUC__) &&  (__GNUC__ < 3)
template<class Y> EncodeStream & operator<< (EncodeStream & os, SharedPtr<Y> const & p)
{
    os << p.get();
    return os;
}
#else
template<class E, class T, class Y> std::basic_ostream<E, T> & operator<< (std::basic_ostream<E, T> & os, SharedPtr<Y> const & p)
{
    os << p.get();
    return os;
}
#endif

// get_deleter (experimental)
#if (defined(__GNUC__) &&  (__GNUC__ < 3)) || (defined(__EDG_VERSION__) && (__EDG_VERSION__ <= 238))
// g++ 2.9x doesn't allow static_cast<X const *>(void *)
// apparently EDG 2.38 also doesn't accept it
template<class D, class T> D * get_deleter(SharedPtr<T> const & p)
{
    void const * q = p._internal_get_deleter(typeid(D));
    return const_cast<D *>(static_cast<D const *>(q));
}
#else
template<class D, class T> D * get_deleter(SharedPtr<T> const & p)
{
    return static_cast<D *>(p._internal_get_deleter(typeid(D)));
}
#endif

} // namespace resip

#endif

// Note:  This implementation is a modified version of shared_ptr from
// Boost.org
//
// http://www.boost.org/libs/smart_ptr/shared_ptr.htm
//

/* ====================================================================
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 * 
 * Permission is hereby granted, free of charge, to any person or organization
 * obtaining a copy of the software and accompanying documentation covered by
 * this license (the "Software") to use, reproduce, display, distribute,
 * execute, and transmit the Software, and to prepare derivative works of the
 * Software, and to permit third-parties to whom the Software is furnished to
 * do so, all subject to the following:
 * 
 * The copyright notices in the Software and this entire statement, including
 * the above license grant, this restriction and the following disclaimer,
 * must be included in all copies of the Software, in whole or in part, and
 * all derivative works of the Software, unless such copies or derivative
 * works are solely in the form of machine-executable object code generated by
 * a source language processor.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 * SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 * FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * ====================================================================
 */


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
