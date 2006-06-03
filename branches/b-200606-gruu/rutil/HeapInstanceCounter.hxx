#ifndef RESIP_HeapInstanceCounter_hxx
#define RESIP_HeapInstanceCounter_hxx

#ifdef RESIP_HEAP_COUNT

#include <typeinfo>
#include <cstddef>

/** Counts heap instances. Place RESIP_HeapCount(ClassName) in public section
    of class to be watched. 

    call HeapInstanceCounter::dump() to output results as WarningLog.
    
    dump output is of the form: MangledClassName [total instances allocated] > [number outstanding]

    RESIP_HEAP_COUNT macro variable controls the heap counter at compile time.
*/
#define RESIP_HeapCount(type_)                                                  \
      static void* operator new (size_t bytes)                                  \
      {                                                                         \
          return  resip::HeapInstanceCounter::allocate(bytes, typeid(type_));   \
      }                                                                         \
      static void* operator new (size_t bytes, void* p)                         \
      {                                                                         \
          return  p;                                                            \
      }                                                                         \
      static void* operator new[] (size_t bytes)                                \
      {                                                                         \
          return  resip::HeapInstanceCounter::allocate(bytes, typeid(type_));   \
      }                                                                         \
      static void operator delete (void* addr)                                  \
      {                                                                         \
          resip::HeapInstanceCounter::deallocate(addr, typeid(type_));          \
      }                                                                         \
      static void operator delete[] (void* addr)                                \
      {                                                                         \
          resip::HeapInstanceCounter::deallocate(addr, typeid(type_));          \
      }
#else
#if defined (__SUNPRO_CC) 
#define RESIP_HeapCount(type_)class type_
#else
#define RESIP_HeapCount(type_)
#endif
#endif // RESIP_HEAP_COUNT

namespace resip
{

class HeapInstanceCounter
{
   public:
      static void dump();

#ifdef RESIP_HEAP_COUNT
      static void* allocate(size_t bytes, const std::type_info& ti);
      static void deallocate(void* addr, const std::type_info& ti);
#endif

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
