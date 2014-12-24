#ifndef resip_CancelableTimerQueue_hxx
#define resip_CancelableTimerQueue_hxx

#include <map>
#include <limits.h>
#include <iosfwd>

#include "rutil/Timer.hxx"
#include "rutil/HashMap.hxx"
#include "rutil/Inserter.hxx"

using namespace std;

namespace resip
{

template<class T>
class CancelableTimerQueue
{
    public:
      CancelableTimerQueue() : mNextId(0) {};
      ~CancelableTimerQueue() {};

      typedef long Id;
   private:
      typedef std::multimap< UInt64, std::pair<T, Id> > TimerMap;
      typedef HashMap<Id, UInt64> IdToTimer;

   public:
      Id addRelative(T msg,
                     unsigned int offset)
      {
         return addTimer(msg, resip::Timer::getTimeMs() + offset);
      }

      //returns true if the id existed
      bool cancel(Id id)
      {
         typename IdToTimer::iterator j = mIdToTimer.find(id);
         if (j == mIdToTimer.end())
         {
            return false;
         }
         
         for (typename TimerMap::iterator i = mTimerMap.lower_bound(j->second); i != mTimerMap.upper_bound(j->second); i++)
         {
            if (i->second.second == id)
            {
               mTimerMap.erase(i);
               mIdToTimer.erase(j);
               return true;
            }
         }
         //cerr << "mIdToTimer: " << Inserter(mIdToTimer) << endl;
         //cerr << "Searching for ID: " << id << endl;
         //cerr << "TimerMap state: " << Inserter(mTimerMap) << endl;
         resip_assert(mIdToTimer.size() == mTimerMap.size());
         resip_assert(0);
         return false;
      }

      //get the number of milliseconds until the next event, returns -1 if no
      //event is available
      int getTimeout() const
      {
         if (mTimerMap.empty())
         {
            return -1;
         }
         else
         {
            int timeout = (int)(mTimerMap.begin()->first - resip::Timer::getTimeMs());
            if (timeout < 0)
            {
               return 0;
            }
            else
            {
               return timeout;
            }
         }
      }
         
      bool available() const
      {
         return (!mTimerMap.empty() && 
                 mTimerMap.begin()->first <= resip::Timer::getTimeMs());
      }

      T getNext()
      {
         resip_assert(mIdToTimer.size() == mTimerMap.size());

         resip_assert(available());

         typename TimerMap::iterator it = mTimerMap.begin();
         mIdToTimer.erase(it->second.second);

         T msg = it->second.first;
         mTimerMap.erase(it);

         resip_assert(mIdToTimer.size() == mTimerMap.size());
         
         return msg;
      }

      void clear()
      {
         mTimerMap.clear();
         mIdToTimer.clear();
      }

      bool empty() const
      {
         return mTimerMap.empty();
      }

      size_t size() const
      {
         return mTimerMap.size();
      }

   private:
      
      Id addTimer(T msg, UInt64 expiry)
      {
         Id id = getNextId();
         mTimerMap.insert(std::make_pair(expiry, std::make_pair(msg, id)));
         mIdToTimer[id] = expiry;
         resip_assert(mIdToTimer.size() == mTimerMap.size());
         return id;
      }

      Id getNextId()
      {
         mNextId++;
         if (mNextId == INT_MAX)
         {
            mNextId = 1;
         }
         return mNextId;
      }
         
      Id mNextId;
      IdToTimer mIdToTimer;
      TimerMap mTimerMap;
};

}

#endif
/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2004 PurpleComm, Inc.  All rights reserved.
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
 */
