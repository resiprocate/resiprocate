#include "HeapInstanceCounter.hxx"
#include "resiprocate/os/Mutex.hxx"
#if !defined(DISABLE_RESIP_LOG)
#include "resiprocate/os/Logger.hxx"
#endif
#include "resiprocate/os/Data.hxx"

#include <assert.h>
#include <map>

using namespace std;
using namespace resip;

#if !defined(DISABLE_RESIP_LOG)
#define RESIPROCATE_SUBSYSTEM resip::Subsystem::STATS
#endif

namespace   //  unnamed namespace
{
struct InstanceCounts
{
      InstanceCounts()
         : total(0),
           outstanding(0)
      {}

      size_t total;
      size_t outstanding;
};

// .dlb. should be using comparitor on typeinfo
typedef	map<Data, InstanceCounts> AllocationMap;
Mutex allocationMutex;
AllocationMap allocationMap;
}

#ifdef RESIP_HEAP_COUNT
void
HeapInstanceCounter::dump()
{
   Lock l(allocationMutex);
   if (allocationMap.empty())
   {
#if !defined(DISABLE_RESIP_LOG)
      WarningLog(<< "No allocations.");
#endif
   }
   else
   {
      AllocationMap::iterator i = allocationMap.begin();
      for (; i != allocationMap.end(); ++i)
      {
         if (i->second.total)
         {
            //abi::__cxa_demangle(typeid(obj).name(), 0, 0, &status);
#if !defined(DISABLE_RESIP_LOG)
            WarningLog(<< i->first << " " << i->second.total << " > " << i->second.outstanding);
#endif
         }
      }
   }
}

void* 
HeapInstanceCounter::allocate(size_t bytes, 
                              const type_info& ti)
{
   {
      // WarningLog(<< "allocated " << ti.name());
      Lock l(allocationMutex);

      const Data name(Data::Share, ti.name(), strlen(ti.name()));
      allocationMap[name].total += 1;
      allocationMap[name].outstanding += 1;
   }

   void* addr = ::operator new(bytes);
   return addr;
}

void
HeapInstanceCounter::deallocate(void* addr, 
                                const type_info& ti)
{
   {
      // WarningLog(<< "deallocated " << ti.name());
      Lock l(allocationMutex);
      const Data name(Data::Share, ti.name(), strlen(ti.name()));
      allocationMap[name].outstanding -= 1;
   }
   ::operator delete(addr);
}

#else // RESIP_HEAP_COUNT
void
HeapInstanceCounter::dump()
{}

#endif // RESIP_HEAP_COUNT

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
