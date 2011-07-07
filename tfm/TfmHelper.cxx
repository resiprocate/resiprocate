#include "TfmHelper.hxx"

#ifdef WIN32
#  include <windows.h>
#else
#include <unistd.h>
#endif

void sleepSeconds(unsigned int seconds)
{
#ifdef WIN32
   Sleep(seconds*1000);
#else
   sleep(seconds);
#endif
}
