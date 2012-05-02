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
 * This file contains the implementation of the call_accept function.
**/

// common header
#include "common.h"

// Netxx includes
#include "Accept.h"
#include "Netxx/SockOpt.h"
#include "Netxx/Types.h"
#include "SockAddr.h"
#include "Socket.h"
#include "OSError.h"

// system includes
#if defined (WIN32)
# include <winsock2.h>
#else
# include <errno.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <sys/param.h>
# include <sys/un.h>
#endif

namespace Netxx {

    //####################################################################
    Peer call_accept (Socket &socket, bool dont_block) {
	SockOpt socket_options(socket.get_socketfd(), true);
	if (dont_block) socket_options.set_non_blocking();

	SockAddr socket_address(socket.get_type());
	sockaddr *sa = socket_address.get_sa();
	size_type sa_size = socket_address.get_sa_size();

#	if defined(WIN32) || defined (__CYGWIN__)
	    int *sa_size_ptr = reinterpret_cast<int*>(&sa_size);
#	elif defined(__APPLE__) 
	    socklen_t *sa_size_ptr = reinterpret_cast<socklen_t*>(&sa_size); 
#	else
	    size_type *sa_size_ptr = &sa_size;
#	endif

	for (;;) {
	    socket_type client = accept(socket.get_socketfd(), sa, sa_size_ptr);
	    if (client >= 0) return Peer(client, sa, sa_size);

	    error_type error_code = get_last_error();

	    switch (error_code) {
		case EINTR:
		    continue;

		case EWOULDBLOCK:
		case ECONNABORTED:
		    return Peer();

		default:
		{
		    std::string error("accept(2) error: ");
		    error += strerror(error_code);
		    throw Netxx::Exception(error);
		}
	    }
	}
    }
    //####################################################################

} // end Netxx namespace
