#ifndef TimeAccumulate_hxx
#define TimeAccumulate_hxx

#include <map>

#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/Lock.hxx"
#include "resiprocate/os/Mutex.hxx"
#include "resiprocate/os/Timer.hxx"

namespace resip
{

/**
   Accumulates time by distinct string.
   The data is available statically.
   Use class::method as the key when possible to avoid collision.
*/
class TimeAccumulate
{
   public:
      TimeAccumulate(const resip::Data& name)
         : _name(name)
      {
         _start = resip::Timer::getTimeMs();
      }

      ~TimeAccumulate()
      {
         UInt64 end = resip::Timer::getTimeMs();
         end -= _start;
         resip::Lock lock(TimeAccumulate::mutex);
         TimeAccumulate::times[_name] = TimeAccumulate::times[_name] + end;
      }
      
      static UInt64 get(const resip::Data& name)
      {
         return TimeAccumulate::times[name];
      }
      
      static void dump();

   private:
      typedef std::map<resip::Data, UInt64> TimeMap;

      const resip::Data _name;
      UInt64 _start;

      static resip::Mutex mutex;
      static TimeMap times;
};

}
#endif
