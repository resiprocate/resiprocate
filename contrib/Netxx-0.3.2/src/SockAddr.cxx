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
 * This file contains the implementation of the Netxx::SockAddr class.
**/

// common header
#include "common.h"

// Netxx includes
#include "SockAddr.h"

// system includes
#if defined(WIN32)
# include <winsock2.h>
#else
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <sys/param.h>
# include <sys/un.h>
#endif

// standard includes
#include <cstring>

//####################################################################
Netxx::SockAddr::SockAddr (Socket::Type type, port_type port)
    : sa_(0), sa_size_(0)
{
    switch (type) {
	case Socket::TCP:
	case Socket::UDP:
	    setup(AF_INET, port);
	    break;
	
#   ifndef NETXX_NO_INET6
	case Socket::TCP6:
	case Socket::UDP6:
	    setup(AF_INET6, port);
	    break;
#   endif

#   ifndef WIN32
	case Socket::LOCALSTREAM:
	case Socket::LOCALDGRAM:
	    setup(AF_LOCAL, 0);
	    break;
#   endif

	default:
	    setup(AF_INET, port);
	    break;
    }
}
//####################################################################
Netxx::SockAddr::SockAddr (int af_type, port_type port) 
    : sa_(0), sa_size_(0)
{
    setup(af_type, port);
}
//####################################################################
sockaddr* Netxx::SockAddr::get_sa (void) {
    return sa_;
}
//####################################################################
Netxx::size_type Netxx::SockAddr::get_sa_size (void) {
    return sa_size_;
}
//####################################################################
void Netxx::SockAddr::setup (int af_type, port_type port) {
    switch (af_type) {
	case AF_INET:
	    std::memset(&(sa_union_.sa_in), 0, sizeof(sa_union_.sa_in));
	    sa_union_.sa_in.sin_family = AF_INET;
	    if (port) sa_union_.sa_in.sin_port = htons(port);

	    sa_size_ = sizeof(sa_union_.sa_in);
	    sa_ = reinterpret_cast<sockaddr*>(&(sa_union_.sa_in));
	    break;


# ifndef NETXX_NO_INET6
	case AF_INET6:
	    std::memset(&(sa_union_.sa_in6), 0, sizeof(sa_union_.sa_in6));
	    sa_union_.sa_in6.sin6_family = AF_INET6;
	    if (port) sa_union_.sa_in6.sin6_port = htons(port);

	    sa_size_ = sizeof(sa_union_.sa_in6);
	    sa_ = reinterpret_cast<sockaddr*>(&(sa_union_.sa_in6));
	    break;
# endif


#   ifndef WIN32
	case AF_LOCAL:
	    std::memset(&(sa_union_.sa_un), 0, sizeof(sa_union_.sa_un));
	    sa_union_.sa_un.sun_family = AF_LOCAL;

	    sa_size_ = sizeof(sa_union_.sa_un);
	    sa_ = reinterpret_cast<sockaddr*>(&(sa_union_.sa_un));
	    break;
#   endif


	default:
	    throw Exception("Netxx bug: bad socket type given to Netxx:SockAddr");
    }
}
