#include "resiprocate/TimeAccumulate.hxx"
#include "resiprocate/os/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::STATS

using namespace resip;

resip::Mutex TimeAccumulate::mutex;

TimeAccumulate::TimeMap TimeAccumulate::times;

void
TimeAccumulate::dump()
{
   Lock lock(mutex);
   InfoLog(<< "Accumulated times:");
   for (TimeMap::const_iterator i = TimeAccumulate::times.begin();
        i != TimeAccumulate::times.end(); i++)
   {
      WarningLog(<< "        " << i->first << " = " << i->second);
   }
}
