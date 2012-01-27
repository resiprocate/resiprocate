/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#if !defined(RESIP_SHAREDPTR_HXX)
#define RESIP_SHAREDPTR_HXX

/**
   @file
   @brief Defines a reference-counted pointer class.
   @note This implementation is a modified version of shared_ptr from
         The doxygen comments are based on the original version.
         The Boost.org.  License text is below.

   http://www.boost.org/libs/smart_ptr/shared_ptr.htm
   @ingroup threading
   @see SharedCount
*/

#include "rutil/SharedCount.hxx"
#include <memory>               // for std::auto_ptr
#include <algorithm>            // for std::swap
#include <functional>           // for std::less
#include <typeinfo>             // for std::bad_cast
#include <iosfwd>               // for std::basic_ostream
#include <cassert>

namespace resip
{

template<class T> class enable_shared_from_this;

/// @internal used only internally within the SharedPtr class
struct static_cast_tag {};
/// @internal used only internally within the SharedPtr class
struct const_cast_tag {};
/// @internal used only internally within the SharedPtr class
struct dynamic_cast_tag {};
/// @internal used only internally within the SharedPtr class
struct polymorphic_cast_tag {};

/// @internal used only internally within the SharedPtr class
template<class T> struct SharedPtr_traits
{
   typedef T & reference;
};

/// @internal used only internally within the SharedPtr class
template<> struct SharedPtr_traits<void>
{
   typedef void reference;
};

/// @internal used only internally within the SharedPtr class
template<> struct SharedPtr_traits<void const>
{
   typedef void reference;
};

/// @internal used only internally within the SharedPtr class
template<> struct SharedPtr_traits<void volatile>
{
   typedef void reference;
};

/// @internal used only internally within the SharedPtr class
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
   /// Provides the type of the template parameter T.
   typedef T element_type;
   typedef T value_type;
   typedef T * pointer;
   typedef typename SharedPtr_traits<T>::reference reference;

   /**
     @brief Effects: Constructs an empty SharedPtr.
     @post  Postconditions: use_count() == 0 && get() == 0.
     @throw nothing.
     @note [The nothrow guarantee is important, since reset() is specified 
      in terms of the default constructor; this implies that the constructor 
      must not allocate memory.]
     */
   SharedPtr(): px(0), pn() // never throws in 1.30+
   {
   }

   /**
     @brief Requirements: p must be convertible to T *. Y must be a complete 
      type. The expression delete p must be well-formed, must not invoke 
      undefined behavior, and must not throw exceptions.
      Effects: Constructs a SharedPtr that owns the pointer p.
    @post Postconditions: use_count() == 1 && get() == p.
    @throw std::bad_alloc, or an implementation-defined exception when a 
      resource other than memory could not be obtained.
    @note Exception safety: If an exception is thrown, delete p is called.
      p must be a pointer to an object that was allocated via a C++ new 
      expression or be 0. The postcondition that use count is 1 holds even 
      if p is 0; invoking delete on a pointer that has a value of 0 is 
      harmless.
      [This constructor has been changed to a template in order to remember 
      the actual pointer type passed. The destructor will call delete with 
      the same pointer, complete with its original type, even when T does 
      not have a virtual destructor, or is void.
      The optional intrusive counting support has been dropped as it 
      exposes too much implementation details and doesn't interact well 
      with weak_ptr. The current implementation uses a different mechanism, 
      enable_shared_from_this, to solve the "SharedPtr from this" problem.]
     */
   template<class Y>
   explicit SharedPtr(Y * p): px(p), pn(p, checked_deleter<Y>()) // Y must be complete
   {
       sp_enable_shared_from_this( pn, p, p );
   }

   /**
     @brief Requirements: D's copy constructor must not throw
      SharedPtr will release p by calling d(p)
     @details: Requirements: p must be convertible to T *. D must be 
      CopyConstructible. The copy constructor and destructor of D must not 
      throw. The expression d(p) must be well-formed, must not invoke 
      undefined behavior, and must not throw exceptions. A must be an 
      Allocator, as described in section 20.1.5 (Allocator requirements) of 
      the C++ Standard.
      Effects: Constructs a SharedPtr that owns the pointer p and the 
      deleter d. The second constructor allocates memory using a copy of a.
    @post Postconditions: use_count() == 1 && get() == p.
    @throw std::bad_alloc, or an implementation-defined exception when a 
      resource other than memory could not be obtained.
    @note Exception safety: If an exception is thrown, d(p) is called.
      When the the time comes to delete the object pointed to by p, the 
      stored copy of d is invoked with the stored copy of p as an argument.
      [Custom deallocators allow a factory function returning a SharedPtr 
      to insulate the user from its memory allocation strategy. Since the 
      deallocator is not part of the type, changing the allocation strategy 
      does not break source or binary compatibility, and does not require a 
      client recompilation. For example, a "no-op" deallocator is useful when 
      returning a SharedPtr to a statically allocated object, and other 
      variations allow a SharedPtr to be used as a wrapper for another 
      smart pointer, easing interoperability.
      The support for custom deallocators does not impose significant 
      overhead. Other SharedPtr features still require a deallocator to be 
      kept.
      The requirement that the copy constructor of D does not throw comes 
      from the pass by value. If the copy constructor throws, the pointer 
      is leaked. Removing the requirement requires a pass by 
      (const) reference.
      The main problem with pass by reference lies in its interaction with 
      rvalues. A const reference may still cause a copy, and will require 
      a const operator(). A non-const reference won't bind to an rvalue at all.
     */
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
   /**
     @brief Effects: If r is empty, constructs an empty SharedPtr; 
      otherwise, constructs a SharedPtr that shares ownership with r.
     @post Postconditions: get() == r.get() && use_count() == r.use_count().
     @throw nothing.
     */
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

   /**
    @brief Effects: Constructs a SharedPtr, as if by storing a copy of 
     r.release().
    @post Postconditions: use_count() == 1.
    @throw std::bad_alloc, or an implementation-defined exception when a 
     resource other than memory could not be obtained.
    @note Exception safety: If an exception is thrown, the constructor has 
     no effect.
     [This constructor takes a the source auto_ptr by reference and not by 
     value, and cannot accept auto_ptr temporaries. This is by design, as the 
     constructor offers the strong guarantee; an rvalue reference would 
     solve this problem, too.]
     */
   template<class Y>
   explicit SharedPtr(std::auto_ptr<Y> & r): px(r.get()), pn()
   {
      Y * tmp = r.get();
      pn = shared_count(r);
      sp_enable_shared_from_this( pn, tmp, tmp );
   }

   /**
     @brief Effects: Equivalent to SharedPtr(r).swap(*this).
     @return *this.
     @note The use count updates caused by the temporary object construction 
           and destruction are not considered observable side effects, and 
           the implementation is free to meet the effects (and the implied 
           guarantees) via different means, without creating a temporary. 
           In particular, in the example:
           @code
           SharedPtr<int> p(new int);
           SharedPtr<void> q(p);
           p = p;
           q = p;
           @endcode
    both assignments may be no-ops.
     */
   template<class Y>
   SharedPtr & operator=(SharedPtr<Y> const & r) // never throws
   {
      px = r.px;
      pn = r.pn; // shared_count::op= doesn't throw
      return *this;
   }

   /**
     @brief Effects: Equivalent to SharedPtr(r).swap(*this).
     @return *this.
     @note The use count updates caused by the temporary object construction 
           and destruction are not considered observable side effects, and 
           the implementation is free to meet the effects (and the implied 
           guarantees) via different means, without creating a temporary. 
           In particular, in the example:
           @code
           SharedPtr<int> p(new int);
           SharedPtr<void> q(p);
           p = p;
           q = p;
           @endcode
    both assignments may be no-ops.
     */
   template<class Y>
   SharedPtr & operator=(std::auto_ptr<Y> & r)
   {
      this_type(r).swap(*this);
      return *this;
   }

   /**
     @brief Effects: Equivalent to SharedPtr().swap(*this).
      never throws in 1.30+
     */
   void reset() // never throws in 1.30+
   {
      this_type().swap(*this);
   }

   /**
     @brief Effects: Equivalent to SharedPtr(p).swap(*this).
     */
   template<class Y> void reset(Y * p) // Y must be complete
   {
      assert(p == 0 || p != px); // catch self-reset errors
      this_type(p).swap(*this);
   }

   /**
     @brief Effects: Equivalent to SharedPtr(p,d).swap(*this).
     */
   template<class Y, class D> void reset(Y * p, D d)
   {
      this_type(p, d).swap(*this);
   }

   /**
     @brief
      Requirements: The stored pointer must not be 0.
      Returns: a reference to the object pointed to by the stored pointer.
      Throws: nothing.
     */
   reference operator* () const // never throws
   {
      assert(px != 0);
      return *px;
   }

   /**
     @brief
      Requirements: The stored pointer must not be 0.
      Returns: the stored pointer
      Throws: nothing.
     */
   T * operator-> () const // never throws
   {
      assert(px != 0);
      return px;
   }
    
   /**
     @brief
      Returns: the stored pointer
      Throws: nothing.
     */
   T * get() const // never throws
   {
      return px;
   }

   /**
     @return an unspecified value that, when used in boolean contexts, is 
      equivalent to get() != 0.
     @throw  nothing.
     @note   This conversion operator allows SharedPtr objects to be used in 
      boolean contexts, like if (p && p->valid()) {}. The actual target type 
      is typically a pointer to a member function, avoiding many of the 
      implicit conversion pitfalls.
      [The conversion to bool is not merely syntactic sugar. It allows 
      SharedPtrs to be declared in conditions when using dynamic_pointer_cast 
      or weak_ptr::lock.]
     */
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
   /**
     @note operator! is redundant, but some compilers need it. never throws.
     */
   bool operator! () const // never throws
   {
      return px == 0;
   }

   /**
     @brief
     @return  use_count() == 1.
     @throw   nothing.
     @note    unique() may be faster than use_count(). 
              If you are using unique() to implement copy on write, do not 
              rely on a specific value when the stored pointer is zero.
     */
   bool unique() const // never throws
   {
      return pn.unique();
   }

   /**
     @brief
     @return  the number of SharedPtr objects, *this included, that share 
              ownership with *this, or an unspecified nonnegative value 
              when *this is empty.
     @throw   nothing.
     @note    use_count() is not necessarily efficient. Use only for 
              debugging and testing purposes, not for production code.
     */
   long use_count() const // never throws
   {
      return pn.use_count();
   }

   /**
     @brief Effects: Exchanges the contents of the two smart pointers.
     @throw nothing.
     */
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

   T * px;             /// contained pointer
   shared_count pn;    /// reference counter

};  // SharedPtr

/**
    @returns bool
    @retval  a.get() == b.get().
    @throw   nothing.
  */
template<class T, class U> inline bool operator==(SharedPtr<T> const & a, SharedPtr<U> const & b)
{
    return a.get() == b.get();
}

/**
    @returns bool
    @retval  a.get() != b.get().
    @throw   nothing.
  */
template<class T, class U> inline bool operator!=(SharedPtr<T> const & a, SharedPtr<U> const & b)
{
    return a.get() != b.get();
}

#if __GNUC__ == 2 && __GNUC_MINOR__ <= 96
/**
  @todo Resolve the ambiguity between our op!= and the one in rel_ops
  */
template<class T> inline bool operator!=(SharedPtr<T> const & a, SharedPtr<T> const & b)
{
    return a.get() != b.get();
}

#endif

/**
    @return an unspecified value such that
     * operator< is a strict weak ordering as described in section 25.3 
       [lib.alg.sorting] of the C++ standard;
     * under the equivalence relation defined by operator<, !(a < b) && 
       !(b < a), two SharedPtr instances are equivalent if and only if they 
      share ownership or are both empty.
    @throw nothing.
    @note Allows SharedPtr objects to be used as keys in associative 
          containers.
          Operator< has been preferred over a std::less specialization for 
          consistency and legality reasons, as std::less is required to return
          the results of operator<, and many standard algorithms use 
          operator< instead of std::less for comparisons when a predicate is 
          not supplied. Composite objects, like std::pair, also implement their 
          operator< in terms of their contained subobjects' operator<.
  */
template<class T, class U> inline bool operator<(SharedPtr<T> const & a, SharedPtr<U> const & b)
{
    return a._internal_less(b);
}

/**
    @brief Equivalent to a.swap(b).
    @throw nothing.
    @note  Matches the interface of std::swap. Provided as an aid to generic 
           programming.
           swap is defined in the same namespace as SharedPtr as this is 
           currently the only legal way to supply a swap function that has 
           a chance to be used by the standard library.
  */
template<class T> inline void swap(SharedPtr<T> & a, SharedPtr<T> & b)
{
    a.swap(b);
}

/**
  @brief Requires that the expression static_cast<T*>(r.get()) must be 
         well-formed.
         Returns: If r is empty, an empty SharedPtr<T>; otherwise, 
         a SharedPtr<T> object that stores a copy of static_cast<T*>(r.get()) 
         and shares ownership with r.
  @throw nothing.
  @note  the seemingly equivalent expression
         SharedPtr<T>(static_cast<T*>(r.get()))
         will eventually result in undefined behavior, attempting to delete 
         the same object twice.

  */
template<class T, class U> SharedPtr<T> static_pointer_cast(SharedPtr<U> const & r)
{
    return SharedPtr<T>(r, static_cast_tag());
}

/**
  @brief Requires that the expression const_cast<T*>(r.get()) must be 
         well-formed.
         Returns: If r is empty, an empty SharedPtr<T>; otherwise, 
         a SharedPtr<T> object that stores a copy of const_cast<T*>(r.get()) 
         and shares ownership with r.
  @throw nothing.
  @note  the seemingly equivalent expression
         SharedPtr<T>(const_cast<T*>(r.get()))
         will eventually result in undefined behavior, attempting to delete 
         the same object twice.
  */
template<class T, class U> SharedPtr<T> const_pointer_cast(SharedPtr<U> const & r)
{
    return SharedPtr<T>(r, const_cast_tag());
}

/**
    @brief Requires that the expression dynamic_cast<T*>(r.get()) must be 
     well-formed and its behavior defined.
     Returns
        * When dynamic_cast<T*>(r.get()) returns a nonzero value, 
          a SharedPtr<T> object that stores a copy of it and shares 
          ownership with r;
        * Otherwise, an empty SharedPtr<T> object.
    @throw nothing.
    @note  the seemingly equivalent expression
           SharedPtr<T>(dynamic_cast<T*>(r.get()))
           will eventually result in undefined behavior, attempting to delete 
           the same object twice.
  */
template<class T, class U> SharedPtr<T> dynamic_pointer_cast(SharedPtr<U> const & r)
{
    return SharedPtr<T>(r, dynamic_cast_tag());
}

/**
  @deprecated shared_*_cast names are deprecated. Use *_pointer_cast instead.
  */
template<class T, class U> SharedPtr<T> shared_static_cast(SharedPtr<U> const & r)
{
    return SharedPtr<T>(r, static_cast_tag());
}

/**
  @deprecated shared_*_cast names are deprecated. Use *_pointer_cast instead.
  */
template<class T, class U> SharedPtr<T> shared_dynamic_cast(SharedPtr<U> const & r)
{
    return SharedPtr<T>(r, dynamic_cast_tag());
}

/**
  @deprecated shared_*_cast names are deprecated. Use *_pointer_cast instead.
  */
template<class T, class U> SharedPtr<T> shared_polymorphic_cast(SharedPtr<U> const & r)
{
    return SharedPtr<T>(r, polymorphic_cast_tag());
}

/**
  @deprecated shared_*_cast names are deprecated. Use *_pointer_cast instead.
  */
template<class T, class U> SharedPtr<T> shared_polymorphic_downcast(SharedPtr<U> const & r)
{
    assert(dynamic_cast<T *>(r.get()) == r.get());
    return shared_static_cast<T>(r);
}

/**
    @retval p.get().
    @throw  nothing.
    @note   Provided as an aid to generic programming. Used by mem_fn.
  */
template<class T> inline T * get_pointer(SharedPtr<T> const & p)
{
    return p.get();
}

// operator<<
/**
  @brief 
    Effects: os << p.get();.
    Returns: os.
  */
#if defined(__GNUC__) &&  (__GNUC__ < 3)
template<class Y> std::ostream & operator<< (std::ostream & os, SharedPtr<Y> const & p)
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
/**
 @brief 
 @retval &d If *this owns a deleter d  of type (cv-unqualified) D
 @retval 0  otherwise
 */
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
