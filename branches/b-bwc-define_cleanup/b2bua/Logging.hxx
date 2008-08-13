
/*

  Logging.h provides definitions for logging.

  Possible implementations of these definitions are:
  - no logging - empty definitions for production use
  - syslog - pass all messages to syslog
  - file or stream - log all messages to a file or stream
  - resip - log all messages to the underlying resiprocate logging method
  - Log4C - pass all logging to Log4c
*/

#ifndef __Logging_h
#define __Logging_h


// syslog implementation 

#include <syslog.h>

#define B2BUA_LOG_INIT(n) openlog(n, LOG_ODELAY | LOG_PID, LOG_LOCAL0)

#define B2BUA_LOG_DEBUG(fmt, ...) syslog(LOG_DEBUG, "b2bua:%s:%d: " #fmt, __FILE__, __LINE__, ## __VA_ARGS__)
#define B2BUA_LOG_INFO(fmt, ...) syslog(LOG_INFO, "b2bua:%s:%d: " #fmt, __FILE__, __LINE__, ## __VA_ARGS__)
#define B2BUA_LOG_NOTICE(fmt, ...) syslog(LOG_NOTICE, "b2bua:%s:%d: " #fmt, __FILE__, __LINE__, ## __VA_ARGS__)
#define B2BUA_LOG_WARNING(fmt, ...) syslog(LOG_WARNING, "b2bua:%s:%d: " #fmt, __FILE__, __LINE__, ## __VA_ARGS__)
#define B2BUA_LOG_ERR(fmt, ...) syslog(LOG_ERR, "b2bua:%s:%d: " #fmt, __FILE__, __LINE__, ## __VA_ARGS__)
#define B2BUA_LOG_CRIT(fmt, ...) syslog(LOG_CRIT, "b2bua:%s:%d: " #fmt, __FILE__, __LINE__, ## __VA_ARGS__)


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

