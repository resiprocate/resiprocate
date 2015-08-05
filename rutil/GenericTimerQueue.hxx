#ifndef RUTIL_GENERICTIMERQUEUE_HXX
#define RUTIL_GENERICTIMERQUEUE_HXX

#include "rutil/Timer.hxx"
#include <set>
#include <vector>

namespace resip {

// !dcm! -- hoist?
template<class T>
class GenericTimerQueue
{
   public:

      // !dcm! -- can def. hoist
      template<class E>
      class TimerEntry
      {
         public:
            TimerEntry(unsigned long tms, E* event) :
               mWhen(tms + Timer::getTimeMs()),
               mEvent(event)
            {
            }
            
            E* getEvent() const
            {
               return mEvent;
            }
            
            bool operator<(const TimerEntry<E>& rhs) const
            {
               return mWhen < rhs.mWhen;
            }
            UInt64 mWhen;
            E* mEvent;
      };
           
      /// deletes the message associated with the timer as well.
      virtual ~GenericTimerQueue()
      {
         for (typename std::multiset<TimerEntry<T> >::iterator i = mTimers.begin(); i !=  mTimers.end(); ++i)
         {
            delete i->mEvent;            
         }
      }
      
      virtual void process()
      {
         if (!mTimers.empty() && msTillNextTimer() == 0)
         {
            TimerEntry<T> now(0, 0);

            // hacky solution, needs work, insertion from processTimer
            // caused bizarre infinite loop issues in previous revision;
            // this code is in desperate need of help
            typedef typename std::multiset<TimerEntry<T> > TimerSet;
            typedef std::vector<typename TimerSet::iterator> ItVector;
            ItVector iterators;

            typename TimerSet::iterator end = mTimers.upper_bound(now);
            typename TimerSet::iterator begin = mTimers.begin();

            for (typename TimerSet::iterator i = begin; i != end; ++i)
            {
               iterators.push_back(i);
            }

            for (typename ItVector::iterator i = iterators.begin(); i != iterators.end(); ++i)
            {
               resip_assert((*i)->getEvent());
               processTimer((*i)->getEvent());               
            }

            //!dcm! -- erasing the range didn't work...upper bound was prob. end()
            for (typename ItVector::iterator i = iterators.begin(); i != iterators.end(); ++i)
            {
               mTimers.erase(*i);
            }
         }
      }

      virtual void processTimer(T*)=0;      

	  void add(T* event, unsigned long msOffset)
	  {
   	      TimerEntry<T> t(msOffset, event);
		  mTimers.insert(t);
	  }

      int size() const
      {
         return mTimers.size();
      }
      
      bool empty() const
      {
         return mTimers.empty();
      }
      
      unsigned int msTillNextTimer()
      {
         if (!mTimers.empty())
         {
            UInt64 next = mTimers.begin()->mWhen;
            UInt64 now = Timer::getTimeMs();
            if (now > next) 
            {
               return 0;
            }
            else
            {
               UInt64 ret64 = next - now;
               if ( ret64 > UInt64(INT_MAX) )
               {
                  return INT_MAX;
               }
               else
               { 
                  int ret = int(ret64);
                  return ret;
               }
            }
         }
         else
         {
            return INT_MAX;
         }
      }

      
   protected:
//      friend std::ostream& operator<<(std::ostream&, const GenericTimerQueue&);
      std::multiset<TimerEntry<T> > mTimers;
};

}

#endif // RUTIL_GENERICTIMERQUEUE_HXX
