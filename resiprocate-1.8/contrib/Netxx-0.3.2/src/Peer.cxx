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
 * This file contains the implementation of the Netxx::Peer class.
**/

// common header
#include "common.h"

// Netxx includes
#include <Netxx/Peer.h>

// system includes
#if defined(WIN32)
# include <winsock2.h>
#else
# include <errno.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/uio.h>
# include <sys/time.h>
# include <unistd.h>
# include <sys/un.h>
#endif

// standard includes
#include <algorithm>
#include <iostream>
#include <cstdlib>
#include <new>
#include <cstring>

//####################################################################
namespace {
    const unsigned int const_max_sockaddr_size = 128;
}
//###########################################################################
Netxx::Peer::Peer (void)
    : okay_(false), port_(0), socketfd_(-1), sockaddr_(0), sockaddr_size_(0)
{ }
//###########################################################################
Netxx::Peer::Peer (const char *addr, port_type port, void *saddr, size_type saddr_size)
    : okay_(true), addr_(addr), port_(port), socketfd_(-1), sockaddr_size_(saddr_size)
{
    // TODO should this be sockaddr_ = new char[saddr_size];?
    sockaddr_ = std::malloc(saddr_size);
    if (!sockaddr_) throw std::bad_alloc();

    std::memcpy(sockaddr_, saddr, saddr_size);
}
//###########################################################################
Netxx::Peer::Peer (socket_type socketfd, void *saddr, size_type saddr_size) 
    : okay_(true), port_(0), socketfd_(socketfd), sockaddr_size_(saddr_size)
{
    // TODO should this be sockaddr_ = new char[saddr_size];?
    sockaddr_ = std::malloc(saddr_size);
    if (!sockaddr_) throw std::bad_alloc();

    std::memcpy(sockaddr_, saddr, saddr_size);
    sockaddr *sa = static_cast<sockaddr*>(sockaddr_);

    switch (sa->sa_family) {
	case AF_INET:
	{
	    char buffer[INET_ADDRSTRLEN];
	    if (inet_ntop(AF_INET, &(reinterpret_cast<sockaddr_in*>(sa)->sin_addr), buffer, sizeof(buffer))) {
		addr_ = buffer;
		port_ = ntohs(reinterpret_cast<sockaddr_in*>(sa)->sin_port);
	    }
	}
	break;

#   ifndef NETXX_NO_INET6
	case AF_INET6:
	{
	    char buffer[INET6_ADDRSTRLEN];
	    if (inet_ntop(AF_INET6, &(reinterpret_cast<sockaddr_in6*>(sa)->sin6_addr), buffer, sizeof(buffer))) {
		addr_ = buffer;
		port_ = ntohs(reinterpret_cast<sockaddr_in6*>(sa)->sin6_port);
	    }
	}
	break;
#   endif

#   ifndef WIN32
	case AF_LOCAL:
	    addr_ = reinterpret_cast<sockaddr_un*>(sa)->sun_path;
	    break;
#   endif
    }
}
//###########################################################################
Netxx::Peer::Peer (const Peer &other)
    : okay_(other.okay_), addr_(other.addr_), port_(other.port_), 
    socketfd_(other.socketfd_), sockaddr_(0), sockaddr_size_(other.sockaddr_size_)
{
    if (other.sockaddr_) {
	sockaddr_ = std::malloc(sockaddr_size_);
	if (!sockaddr_) throw std::bad_alloc();
	std::memcpy(sockaddr_, other.sockaddr_, sockaddr_size_);
    }
}
//###########################################################################
Netxx::Peer& Netxx::Peer::operator= (const Peer &other) {
    Peer tmp(other); swap(tmp);
    return *this;
}
//###########################################################################
void Netxx::Peer::swap (Peer &other) {
    std::swap(okay_, other.okay_);
    std::swap(addr_, other.addr_);
    std::swap(port_, other.port_);
    std::swap(socketfd_, other.socketfd_);
    std::swap(sockaddr_, other.sockaddr_);
    std::swap(sockaddr_size_, other.sockaddr_size_);
}
//###########################################################################
Netxx::Peer::~Peer (void) {
    if (sockaddr_) std::free(sockaddr_);
}
//###########################################################################
const char* Netxx::Peer::get_address (void) const {
    return addr_.c_str();
}
//###########################################################################
Netxx::port_type Netxx::Peer::get_port (void) const {
    return port_;
}
//###########################################################################
Netxx::port_type Netxx::Peer::get_local_port (void) const {
    size_type sa_size = const_max_sockaddr_size;

    union {
	sockaddr sa;
	char data[const_max_sockaddr_size];
    } sau;


#   if defined(WIN32) || defined(__CYGWIN__)
	int *sa_size_ptr = reinterpret_cast<int*>(&sa_size);
#   elif defined(__APPLE__) 
        socklen_t *sa_size_ptr = reinterpret_cast<socklen_t*>(&sa_size);
#   else
	size_type *sa_size_ptr = &sa_size;
#   endif

    int rc;

    if ( (rc = getsockname(get_socketfd(), reinterpret_cast<sockaddr*>(sau.data), sa_size_ptr))) {
	    throw Exception(strerror(errno));
    }

   switch (sau.sa.sa_family) {
       case AF_INET:
	   return ntohs(reinterpret_cast<sockaddr_in*>(&sau.sa)->sin_port);

#   ifndef NETXX_NO_INET6
       case AF_INET6:
	   return ntohs(reinterpret_cast<sockaddr_in6*>(&sau.sa)->sin6_port);
#   endif

       default:
	   return 0;
    }
}
//###########################################################################
Netxx::socket_type Netxx::Peer::get_socketfd (void) const {
    return socketfd_;
}
//###########################################################################
Netxx::Peer::operator bool (void) const {
    return okay_;
}
//###########################################################################
const void* Netxx::Peer::get_sa (void) const {
    return sockaddr_;
}
//###########################################################################
Netxx::size_type Netxx::Peer::get_sa_size() const {
    return sockaddr_size_;
}
//###########################################################################
namespace Netxx {

    //###########################################################################
    std::ostream& operator<< (std::ostream &stream, const Peer &peer) {
	const sockaddr *sa = reinterpret_cast<const sockaddr*>(peer.get_sa());

	if (peer) {
	    if (sa && sa->sa_family == AF_LOCAL) {
		if (peer.get_address()[0] == 0) stream << "AF_LOCAL";
		else stream << peer.get_address();
	    } else {
		stream << peer.get_address() << ":" << peer.get_port();
	    }
	}

	return stream;
    }
    //###########################################################################
    
}
