#ifndef TimeAccumulate_hxx
#define TimeAccumulate_hxx

#include <map>

#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/Lock.hxx"
#include "resiprocate/os/Mutex.hxx"
#include "resiprocate/os/Timer.hxx"
#include <iosfwd>

namespace resip
{

class DelayOutputBase
{
   public:
      virtual ~DelayOutputBase() {}
      virtual void put(std::ostream& str) const = 0;
};

std::ostream&
operator<<(std::ostream&, const DelayOutputBase& iib);

template <class T>
class DelayOutput : public DelayOutputBase
{
   public:
      DelayOutput()
         : mT(0)
      {}

      DelayOutput(const T& t)
         : mT(&t)
      {}

      virtual void put(std::ostream& str) const
      {
         str << *myT;
      }

   private:
      const T* mT;
};

/**
   Accumulates time and count by distinct string.
   The data is available statically.
   Use class::method as the key when possible to avoid collision.
*/
class TimeAccumulate
{
   private:
      struct Accumulator
      {
            UInt64 totalTime;
            size_t count;
      };

   public:
#if 1
      TimeAccumulate(const Data& name)
         : mName(name),
           mStart(Timer::getTimeMs()),
           mTooLong(0),
           mTooLongOutputter(0)
      {}

      TimeAccumulate(const Data& name,
                     unsigned int tooLong,
                     const DelayOutputBase* outputter)
         : mName(name),
           mStart(Timer::getTimeMs()),
           mTooLongOutputter(outputter)
      {}

      ~TimeAccumulate();
#else
      TimeAccumulate(const char* chars)
         : mName(Data::Empty),
           mStart(0)
      {}

      TimeAccumulate(const Data& name)
         : mName(Data::Empty),
           mStart(0)
      {}

      ~TimeAccumulate()
      {
      }
#endif
      
      static UInt64 getTime(const Data& name)
      {
         Lock lock(TimeAccumulate::mMutex);
         return TimeAccumulate::mTimes[name].totalTime;
      }
      
      static size_t getCount(const Data& name)
      {
         Lock lock(TimeAccumulate::mMutex);
         return TimeAccumulate::mTimes[name].count;
      }

      static void dump();
      static void clear();

      class Guard
      {
         public:
            explicit Guard(UInt64& accumulator)
               : mAccumulator(accumulator)
            {
               mAccumulator -= Timer::getTimeMs();
            }
            ~Guard()
            {
               mAccumulator += Timer::getTimeMs();
            }

         private:
            UInt64& mAccumulator;
      };

   private:
      typedef std::map<Data, Accumulator> TimeMap;

      const Data mName;
      const UInt64 mStart;
      const UInt64 mTooLong;
      const DelayOutputBase *mTooLongOutputter;

      static Mutex mMutex;
      static TimeMap mTimes;
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
