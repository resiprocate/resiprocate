
#include <util/Socket.hxx>

#include <cassert>
#include <iostream>
#include <stdio.h>
#include <util/Data.hxx>

#ifndef WIN32
#include <sys/time.h>
#endif

#include <sys/types.h>
#include <time.h>

#include <util/Log.hxx>
#include <util/Lock.hxx>

using namespace Vocal2;
using namespace std;

Log::Level Log::_level = Log::DEBUG;
Log::Type Log::_type = COUT;
Data Log::_appName;
Data Log::_hostname;

#ifndef WIN32
pid_t Log::_pid=0;
#else
int Log::_pid=0;
#endif

const char
Log::_descriptions[][32] = {"EMERG", "ALERT", "CRIT", "ERR", "WARNING", "NOTICE", "INFO", "DEBUG", "DEBUG_STACK", ""}; 

Mutex Log::_mutex;

void 
Log::initialize(Type type, Level level, const Data& appName)
{
   string copy(appName.c_str());
   
   _type = type;
   _level = level;
   _appName = copy.substr(copy.find_last_of("/")+1);
   
 
#ifdef WIN32 
   // TODO FIX  
  assert(0); 
  _hostname = "Unkown"; 
  _pid = 0;
#else  
  char buffer[1024];  
  gethostname(buffer, sizeof(buffer));
   _hostname = buffer;
   _pid = getpid();
#endif

}

void
Log::setLevel(Level level)
{
   Lock lock(_mutex);
   _level = level; 
}

Data
Log::toString(Level l)
{
   return Data("LOG_") + _descriptions[l];
}

Log::Level
Log::toLevel(const Data& l)
{
   string pri = "l";
   if (pri.find("LOG_", 0) == 0)
   {
      pri.erase(0, 4);
   }
   
   int i=0;
   while (string(_descriptions[i]).size())
   {
      if (pri == string(_descriptions[i])) 
      {
         return Level(i);
      }
      i++;
   }

   cerr << "Choosing Debug level since string was not understood: " << l << endl;
   return Log::DEBUG;
}


ostream&
Log::tags(Log::Level level, const Subsystem& subsystem, ostream& strm) 
{
#ifdef WIN32
	strm << _descriptions[level] << DELIM
        << timestamp() << DELIM
        << subsystem << DELIM ;
#else   
     strm << _descriptions[level] << DELIM
        << timestamp() << DELIM  
        << _hostname << DELIM  
        << _appName << DELIM
        << subsystem << DELIM 
        << _pid << DELIM
		<< pthread_self() << DELIM;
#endif 
  return strm;
}

Data
Log::timestamp() 
{
   const unsigned int DATEBUF_SIZE=256;
   char datebuf[DATEBUF_SIZE];
   
#ifdef WIN32 
 int result = 1; 
  struct { int tv_sec; int tv_usec; } tv = {0,0};
#else 
  struct timeval tv; 
  int result = gettimeofday (&tv, NULL);
#endif   

   if (result == -1)
   {
      /* If we can't get the time of day, don't print a timestamp.
        Under Unix, this will never happen:  gettimeofday can fail only
        if the timezone is invalid which it can't be, since it is
        uninitialized]or if tv or tz are invalid pointers. */
        datebuf [0] = 0;
    }
    else
    {
       /* The tv_sec field represents the number of seconds passed since
          the Epoch, which is exactly the argument gettimeofday needs. */
       const time_t timeInSeconds = (time_t) tv.tv_sec;
       strftime (datebuf,
                 DATEBUF_SIZE,
                 "%Y%m%d-%H%M%S", /* guaranteed to fit in 256 chars,
                                     hence don't check return code */
                 localtime (&timeInSeconds));
    }
   
   char msbuf[5];
   /* Dividing (without remainder) by 1000 rounds the microseconds
      measure to the nearest millisecond. */
   sprintf(msbuf, ".%3.3ld", long(tv.tv_usec / 1000));
   
   int datebufCharsRemaining = DATEBUF_SIZE - strlen (datebuf);
   strncat (datebuf, msbuf, datebufCharsRemaining - 1);
   datebuf[DATEBUF_SIZE - 1] = '\0'; /* Just in case strncat truncated msbuf,
                                        thereby leaving its last character at
                                        the end, instead of a null terminator */

   return datebuf;
}
   
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
