#ifndef SysLogBuf_hxx
#define SysLogBuf_hxx

#include <iostream>
#include <cassert>
#include <syslog.h>

namespace Vocal2
{

class SysLogBuf : public std::streambuf 
{
   public:
      SysLogBuf ()
      {
         setp(buffer,buffer+Size);
         openlog (0, LOG_NDELAY, LOG_LOCAL6);
      }
      
      ~SysLogBuf()
      {
         //closelog();
      }
      
      inline int sync ();
      inline int overflow (int ch);

      // Defining xsputn is an optional optimization.
      // (streamsize was recently added to ANSI C++, not portable yet.)
      //inline streamsize xsputn (char* text, streamsize n);

   private:
      enum { Size=4095 }; 
      char buffer[Size+1];
};

     
inline int SysLogBuf::sync ()
{
   *(pptr()) = 0;
   syslog (LOG_LOCAL6 | LOG_DEBUG, pbase());
   setp(buffer, buffer+Size);
   return 0;
}
     
inline int SysLogBuf::overflow (int c)
{
   sync();
   if (c != EOF) 
   {
      *pptr() = static_cast<unsigned char>(c);
      pbump(1);
   }
   return c;
}
 
}

#endif
