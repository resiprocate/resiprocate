#if !defined(RESIP_SHAREDCOUNT_HXX)
#define RESIP_SHAREDCOUNT_HXX

/**
   @file 
   @brief Defines a threadsafe (shared) reference-count object.

   @note   This implementation is a modified version of shared_count from
   Boost.org
*/

#include <memory>           // std::auto_ptr, std::allocator
#include <functional>       // std::less
#include <exception>        // std::exception
#include <new>              // std::bad_alloc
#include <typeinfo>         // std::type_info in get_deleter
#include <cstddef>          // std::size_t
#include "rutil/Lock.hxx"
#include "rutil/Mutex.hxx"
//#include "rutil/Logger.hxx"

#ifdef __BORLANDC__
# pragma warn -8026     // Functions with excep. spec. are not expanded inline
# pragma warn -8027     // Functions containing try are not expanded inline
#endif

namespace resip
{

// verify that types are complete for increased safety
template<class T> inline void checked_delete(T * x)
{
   // intentionally complex - simplification causes regressions
   typedef char type_must_be_complete[ sizeof(T)? 1: -1 ];
   (void) sizeof(type_must_be_complete);
   delete x;
};

template<class T> struct checked_deleter
{
   typedef void result_type;
   typedef T * argument_type;

   void operator()(T * x) const
   {
       // resip:: disables ADL
       resip::checked_delete(x);
   }
};

// The standard library that comes with Borland C++ 5.5.1
// defines std::exception and its members as having C calling
// convention (-pc). When the definition of bad_weak_ptr
// is compiled with -ps, the compiler issues an error.
// Hence, the temporary #pragma option -pc below. The version
// check is deliberately conservative.
#if defined(__BORLANDC__) && __BORLANDC__ == 0x551
# pragma option push -pc
#endif

class bad_weak_ptr: public std::exception
{
public:

   virtual char const * what() const throw()
   {
       return "resip::bad_weak_ptr";
   }
};

#if defined(__BORLANDC__) && __BORLANDC__ == 0x551
# pragma option pop
#endif

class sp_counted_base
{
private:

public:

   sp_counted_base(): use_count_(1), weak_count_(1)
   {
   }

   virtual ~sp_counted_base() // nothrow
   {
   }

   // dispose() is called when use_count_ drops to zero, to release
   // the resources managed by *this.
   virtual void dispose() = 0; // nothrow

   // destruct() is called when weak_count_ drops to zero.
   virtual void destruct() // nothrow
   {
       delete this;
   }

   virtual void * get_deleter(std::type_info const & ti) = 0;

   void add_ref_copy()
   {
      Lock lock(mMutex); (void)lock;
      ++use_count_;
      //GenericLog(Subsystem::SIP, resip::Log::Info, << "********* SharedCount::add_ref_copy: " << use_count_);
   }

   void add_ref_lock()
   {
      Lock lock(mMutex); (void)lock;
      // if(use_count_ == 0) throw(resip::bad_weak_ptr());
      if (use_count_ == 0) throw resip::bad_weak_ptr();
      ++use_count_;
      //GenericLog(Subsystem::SIP, resip::Log::Info, << "********* SharedCount::add_ref_lock: " << use_count_);
   }

   void release() // nothrow
   {
      {
         Lock lock(mMutex); (void)lock;
         long new_use_count = --use_count_;
         //GenericLog(Subsystem::SIP, resip::Log::Info, << "********* SharedCount::release: " << use_count_);

         if(new_use_count != 0) return;
      }

      dispose();
      weak_release();
   }

   void weak_add_ref() // nothrow
   {
      Lock lock(mMutex); (void)lock;
      ++weak_count_;
   }

   void weak_release() // nothrow
   {
      long new_weak_count;

      {
         Lock lock(mMutex); (void)lock;
         new_weak_count = --weak_count_;
      }

      if(new_weak_count == 0)
      {
          destruct();
      }
   }

   long use_count() const // nothrow
   {
      Lock lock(mMutex); (void)lock;
      return use_count_;
   }

private:

   sp_counted_base(sp_counted_base const &);
   sp_counted_base & operator= (sp_counted_base const &);

   long use_count_;        // #shared
   long weak_count_;       // #weak + (#shared != 0)

   mutable Mutex mMutex;
};

//
// Borland's Codeguard trips up over the -Vx- option here:
//
#ifdef __CODEGUARD__
# pragma option push -Vx-
#endif

template<class P, class D> class sp_counted_base_impl: public sp_counted_base
{
private:

   P ptr; // copy constructor must not throw
   D del; // copy constructor must not throw

   sp_counted_base_impl(sp_counted_base_impl const &);
   sp_counted_base_impl & operator= (sp_counted_base_impl const &);

   typedef sp_counted_base_impl<P, D> this_type;

public:

   // pre: initial_use_count <= initial_weak_count, d(p) must not throw
   sp_counted_base_impl(P p, D d): ptr(p), del(d)
   {
   }

   virtual void dispose() // nothrow
   {
      del(ptr);
   }

   virtual void * get_deleter(std::type_info const & ti)
   {
      return ti == typeid(D)? &del: 0;
   }

   void * operator new(size_t)
   {
      return std::allocator<this_type>().allocate(1, static_cast<this_type *>(0));
   }

   void operator delete(void * p)
   {
      std::allocator<this_type>().deallocate(static_cast<this_type *>(p), 1);
   }

};

class shared_count
{
private:

   sp_counted_base * pi_;

public:

   shared_count(): pi_(0) // nothrow
   {
   }

   template<class P, class D> shared_count(P p, D d): pi_(0)
   {
      try
      {
         pi_ = new sp_counted_base_impl<P, D>(p, d);
      }
      catch(...)
      {
         d(p); // delete p
         throw;
      }
   }

   // auto_ptr<Y> is special cased to provide the strong guarantee
   template<class Y>
   explicit shared_count(std::auto_ptr<Y> & r): pi_(new sp_counted_base_impl< Y *, checked_deleter<Y> >(r.get(), checked_deleter<Y>()))
   {
      r.release();
   }

   ~shared_count() // nothrow
   {
      if(pi_ != 0) pi_->release();
   }

   shared_count(shared_count const & r): pi_(r.pi_) // nothrow
   {
      if(pi_ != 0) pi_->add_ref_copy();
   }

   shared_count & operator= (shared_count const & r) // nothrow
   {
      sp_counted_base * tmp = r.pi_;
      if(tmp != 0) tmp->add_ref_copy();
      if(pi_ != 0) pi_->release();
      pi_ = tmp;

      return *this;
   }

   void swap(shared_count & r) // nothrow
   {
      sp_counted_base * tmp = r.pi_;
      r.pi_ = pi_;
      pi_ = tmp;
   }

   long use_count() const // nothrow
   {
      return pi_ != 0? pi_->use_count(): 0;
   }

   bool unique() const // nothrow
   {
      return use_count() == 1;
   }

   friend inline bool operator==(shared_count const & a, shared_count const & b)
   {
      return a.pi_ == b.pi_;
   }

   friend inline bool operator<(shared_count const & a, shared_count const & b)
   {
      return std::less<sp_counted_base *>()(a.pi_, b.pi_);
   }

   void * get_deleter(std::type_info const & ti) const
   {
      return pi_? pi_->get_deleter(ti): 0;
   }
};

#ifdef __CODEGUARD__
# pragma option pop
#endif

} // namespace resip

#ifdef __BORLANDC__
# pragma warn .8027     // Functions containing try are not expanded inline
# pragma warn .8026     // Functions with excep. spec. are not expanded inline
#endif


#endif

// Note:  This implementation is a modified version of shared_count from
// Boost.org
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
