#if !defined(WIN32)
#include <syslog.h>
#endif

#include <cassert>
#include "resiprocate/os/SysLogBuf.hxx"

using resip::SysLogBuf;

SysLogBuf::SysLogBuf ()
{
#if !defined(WIN32)
   setp(buffer,buffer+Size);
   openlog (0, LOG_NDELAY, LOG_LOCAL6);
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
   syslog (LOG_LOCAL6 | LOG_DEBUG, pbase());
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
