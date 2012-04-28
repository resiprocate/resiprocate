/*
 * Copyright (C) 2001-2003 Peter J Jones (pjones@pmade.org)
 * All Rights Reserved
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the Author nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/** @file
 * This file contains definitions for OS error functions.
**/

#ifndef _OSError_h_
#define _OSError_h_

#if defined (WIN32)
# include <winsock2.h>
# include <errno.h>

# undef  EINTR
# define EINTR WSAEINTR

# undef  EWOULDBLOCK
# define EWOULDBLOCK WSAEWOULDBLOCK

# undef  EINPROGRESS
# define EINPROGRESS WSAEINPROGRESS

# undef  EAFNOSUPPORT
# define EAFNOSUPPORT WSAEAFNOSUPPORT

# undef  ENOSPC
# define ENOSPC WSAENOSPC

# undef  ECONNRESET
# define ECONNRESET WSAECONNRESET

# undef  ECONNABORTED
# define ECONNABORTED WSAECONNABORTED
#else
# include <errno.h>
#endif

namespace Netxx {

    typedef int error_type;
    error_type get_last_error (void);

} // end Netxx namespace

#endif
