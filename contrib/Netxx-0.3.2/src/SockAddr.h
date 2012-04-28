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
 * This file defines the Netxx::SockAddr class.
**/

#ifndef _Netxx_SockAddr_h_
#define _Netxx_SockAddr_h_

// Netxx includes
#include "Socket.h"
#include "Netxx/Types.h"

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

namespace Netxx {

/**
 * The SockAddr class is a thin wrapper around the sockaddr_* structs. It is
 * mainly used to create and cleanup after these structs.
**/
class SockAddr {
public:
    explicit SockAddr (Socket::Type type, port_type port=0);

    explicit SockAddr (int af_type, port_type port=0);

    sockaddr* get_sa (void);
    size_type get_sa_size (void);
    
    void setup (int af_type, port_type port);
private:
    union {
	sockaddr_in sa_in;

#   ifndef NETXX_NO_INET6
	sockaddr_in6 sa_in6;
#   endif

#   ifndef WIN32
	sockaddr_un sa_un;
#   endif

    } sa_union_;

    sockaddr *sa_;
    size_type sa_size_;

    SockAddr (const SockAddr &);
    SockAddr& operator= (const SockAddr &);
}; // end Netxx::SockAddr class

} // end Netxx namespace
#endif
