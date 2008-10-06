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
 * The file contains structs, defines and functions to fake INET6
 * for systems that do not yet support INET6.
**/

// common header
#include "common.h"

// declaration include
#include "inet6.h"

// system includes
#if defined(WIN32)
# include <winsock2.h>
# include <stdio.h>
# define snprintf _snprintf
#else
# include <stdio.h>
# include <errno.h>
#endif

// standard includes
#include <cstring>

//####################################################################
int inet_pton (int family, const char *strptr, void *addrptr) {
    if (family == AF_INET) {
	unsigned long in_val;

	if ( (in_val = inet_addr(strptr)) != INADDR_NONE) {
	    in_addr in_struct;
	    std::memset(&in_struct, 0, sizeof(in_struct));
	    in_struct.s_addr = in_val;
	    std::memcpy(addrptr, &in_struct, sizeof(in_struct));
	    return 1;
	}

	return 0;
    }

#   ifndef WIN32
	errno = EAFNOSUPPORT;
#   endif

    return -1;
}
//####################################################################
const char *inet_ntop (int family, const void *addrptr, char *strptr, size_t len) {
    const unsigned char *p = reinterpret_cast<const unsigned char*>(addrptr);

    if (family == AF_INET) {
	char temp[INET_ADDRSTRLEN];
	snprintf(temp, sizeof(temp), "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);

	if (std::strlen(temp) >= len) {
#	    ifndef WIN32
		errno = ENOSPC;
#	    endif
	    return 0;
	}

	std::strcpy(strptr, temp);
	return strptr;
    }

#   ifndef WIN32
	errno = EAFNOSUPPORT;
#   endif

    return 0;
}
