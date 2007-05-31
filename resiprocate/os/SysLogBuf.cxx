#if !defined(WIN32)
#include <syslog.h>
#endif

#include <cassert>
#include "resiprocate/os/SysLogBuf.hxx"

using resip::SysLogBuf;

SysLogBuf::SysLogBuf()
   : mFacility(LOG_LOCAL6),
     mLevel(LOG_DEBUG)
{
#if !defined(WIN32)
   setp(buffer, buffer+Size);
   openlog(0, LOG_NDELAY, mFacility);
#endif
}
      
SysLogBuf::SysLogBuf(int facility, int level)
   : mFacility(facility),
     mLevel(level)
{
#if !defined(WIN32)
   setp(buffer, buffer+Size);
   openlog(0, LOG_NDELAY, mFacility);
#endif
}

SysLogBuf::~SysLogBuf()
{
}
      
int 
SysLogBuf::sync()
{
#if !defined(WIN32)
   *(pptr()) = 0;
   syslog(mFacility | mLevel, "%s", pbase());
   setp(buffer, buffer+Size);
#else
   assert(0);
#endif
   return 0;
}

int 
SysLogBuf::overflow (int c)
{
   sync();
   if (c != EOF) 
   {
      *pptr() = static_cast<unsigned char>(c);
      pbump(1);
   }
   return c;
}
