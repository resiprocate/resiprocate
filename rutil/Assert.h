
#ifndef __ASSERTION_H
#define __ASSERTION_H

/**
 * This file needs to work for both C and C++ code.
 *
 * The resip_assert() macro is preferred over regular assert()
 * for all library and application code.  Unit tests should continue
 * to use regular assert().  Third party code placed on contrib
 * should not be adapted to use resip_assert()
 *
 * It is possible to redefine resip_assert() to provide additional logging
 * or throw an exception such as std::runtime_error() as desired.
 *
 * On UNIX systems, this could add a call to syslog.  On Windows,
 * it could be a notification in the system's event log.
 *
 * CAUTION: do not call other reSIProcate stack methods from here.
 * For example, do not try to use the reSIProcate logger or even
 * basic classes like resip::Data.  If the resip_assert() was
 * invoked from one of those other low-level classes then the stack
 * may be in such a bad state that they behave unpredictably
 * or even worse, recursion may occur if resip_assert() is invoked
 * again.  We may do further work on this code to make it work
 * safely with the logger but at present it is not ready for that.
 */


#include <assert.h>
#define resip_assert(x) assert(x)

#endif

/* ====================================================================
 *
 * Copyright (c) 2014 Daniel Pocock  All rights reserved.
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */

