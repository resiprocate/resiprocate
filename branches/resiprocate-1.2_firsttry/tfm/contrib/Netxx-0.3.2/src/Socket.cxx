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
 * This file contains the implementation for the Netxx::Socket class along
 * with some helper functions.
**/

// common header
#include "common.h"

// Netxx includes
#include "Socket.h"
#include "Netxx/Timeout.h"
#include "OSError.h"

// system includes
#if defined(WIN32)
# include <winbase.h>
# include <winsock2.h>
#else
# include <errno.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/uio.h>
# include <sys/time.h>
# include <unistd.h>
#endif

// standard includes
#include <algorithm>
#include <string>
#include <cstring>
#include <cstdio>

//####################################################################
namespace {
    void get_domain_and_type (Netxx::Socket::Type stype, int &domain, int &type);
}
//####################################################################
Netxx::Socket::Socket (void)
    : socketfd_(-1), probe_ready_(false), type_ready_(false)
{
#   if defined(WIN32)
	WSADATA wsdata;
	
	if (WSAStartup(MAKEWORD(2,2), &wsdata) != 0) {
	    throw Exception("failed to load WinSock");
	}
#   endif
}
//####################################################################
Netxx::Socket::Socket (Type type)
    : socketfd_(-1), probe_ready_(false), type_ready_(true), type_(type)
{
#   if defined(WIN32)
	WSADATA wsdata;
	
	if (WSAStartup(MAKEWORD(2,2), &wsdata) != 0) {
	    throw Exception("failed to load WinSock");
	}
#   endif

    int socket_domain=0;
    socket_type socket_type=0, socket_fd=0;

    get_domain_and_type(type, socket_domain, socket_type);

    if ( (socket_fd = socket(socket_domain, socket_type, 0)) < 0) {
	std::string error("failure from socket(2): ");
	error += strerror(get_last_error());
	throw Exception(error);
    }

    socketfd_ = socket_fd;
}
//####################################################################
Netxx::Socket::Socket (int socketfd)
    : socketfd_(socketfd), probe_ready_(false), type_ready_(false)
{
#   if defined(WIN32)
	WSADATA wsdata;
	
	if (WSAStartup(MAKEWORD(2,2), &wsdata) != 0) {
	    throw Exception("failed to load WinSock");
	}
#   endif
}
//####################################################################
Netxx::Socket::Socket (const Socket &other) 
    : probe_ready_(false), type_ready_(other.type_ready_), type_(other.type_)
{
    if (other.socketfd_ != -1) {
	socket_type dup_socket;

#	if defined(WIN32)
	    WSADATA wsdata;
	    
	    if (WSAStartup(MAKEWORD(2,2), &wsdata) != 0)
		throw Exception("failed to load WinSock");

	    WSAPROTOCOL_INFO proto_info;
	    std::memset(&proto_info, 0, sizeof(proto_info));

	    if (WSADuplicateSocket(other.socketfd_, GetCurrentProcessId(), &proto_info)) {
		throw Exception("failed to duplicate socket");
	    }
	    
	    if ( (dup_socket = WSASocket(FROM_PROTOCOL_INFO, FROM_PROTOCOL_INFO,
		    FROM_PROTOCOL_INFO, &proto_info, 0, WSA_FLAG_OVERLAPPED)) == INVALID_SOCKET)
	    {
		throw Exception("failed to init duplicated socket");
	    }
#	else
	    dup_socket = dup(other.socketfd_);

	    if (dup_socket < 0) {
		std::string error("dup(2) call failed: ");
		error += strerror(get_last_error());
		throw Exception(error);
	    }
#	endif

	socketfd_ = dup_socket;
    } else {
	socketfd_ = -1;
    }
}
//####################################################################
Netxx::Socket& Netxx::Socket::operator= (const Socket &other) {
    Socket tmp(other); swap(tmp);
    return *this;
}
//####################################################################
void Netxx::Socket::swap (Socket &other) {
    std::swap(socketfd_, other.socketfd_);
    std::swap(type_ready_, other.type_ready_);
    std::swap(type_, other.type_);

    probe_ready_ = other.probe_ready_ = false;
    probe_.clear(); other.probe_.clear();
}
//####################################################################
Netxx::Socket::~Socket (void) {
    close();

#   if defined(WIN32)
	WSACleanup();
#   endif
}
//####################################################################
Netxx::signed_size_type Netxx::Socket::write (const void *buffer, size_type length, const Timeout &timeout) {
    const char *buffer_ptr = static_cast<const char*>(buffer);
    signed_size_type rc, bytes_written=0;

    while (length) {
	if (timeout && !writable(timeout)) return -1;

	if ( (rc = send(socketfd_, buffer_ptr, length, 0)) < 0) {
	    error_type error_code = get_last_error();
	    if (error_code == EWOULDBLOCK) error_code = EAGAIN;

	    switch (error_code) {
		case EPIPE:
		case ECONNRESET:
		    return 0;

		case EINTR:
		    continue;

		case EAGAIN:
		    return -1;

#if defined(WIN32)
		case WSAENETRESET:
		case WSAESHUTDOWN:
		case WSAEHOSTUNREACH:
		case WSAECONNABORTED:
		case WSAETIMEDOUT:
		    return 0;
#endif

		default:
		{
		    std::string error("send failed: ");
		    error += strerror(error_code);
		    throw Exception(error);
		}
	    }
	}

	buffer_ptr    += rc;
	bytes_written += rc;
	length        -= rc;
    }

    return bytes_written;
}
//####################################################################
Netxx::signed_size_type Netxx::Socket::read (void *buffer, size_type length, const Timeout &timeout) {
    signed_size_type rc;

#   if defined(WIN32)
	char *buffer_ptr = static_cast<char*>(buffer);
#   else
	void *buffer_ptr(buffer);
#   endif

    for (;;) {
	if (timeout && !readable(timeout)) return -1;

	if ( (rc = recv(socketfd_, buffer_ptr, length, 0)) < 0) {
	    error_type error_code = get_last_error();
	    if (error_code == EWOULDBLOCK) error_code = EAGAIN;

	    switch (error_code) {
		case ECONNRESET:
		    return 0;

		case EINTR:
		    continue;

		case EAGAIN:
		    return -1;

#if defined(WIN32)

		case WSAEMSGSIZE:
		    return length;

		case WSAENETRESET:
		case WSAESHUTDOWN:
		case WSAECONNABORTED:
		case WSAETIMEDOUT: // FIXME should this be return -1?
		    return 0;

#endif

		default:
		{
		    std::string error("recv failure: ");
		    error += strerror(error_code);
		    throw Exception(error);
		}
	    }
	}

	break;
    }

    return rc;
}
//####################################################################
bool Netxx::Socket::readable (const Timeout &timeout) {
    if (!probe_ready_) {
	probe_.add(socketfd_);
	probe_ready_ = true;
    }

    Probe_impl::probe_type pt = probe_.probe(timeout, Probe::ready_read);
    if (pt.empty()) return false;
    return true;
}
//####################################################################
bool Netxx::Socket::writable (const Timeout &timeout) {
    if (!probe_ready_) {
	probe_.add(socketfd_);
	probe_ready_ = true;
    }

    Probe_impl::probe_type pt = probe_.probe(timeout, Probe::ready_write);
    if (pt.empty()) return false;
    return true;
}
//####################################################################
bool Netxx::Socket::readable_or_writable (const Timeout &timeout) {
    if (!probe_ready_) {
	probe_.add(socketfd_);
	probe_ready_ = true;
    }

    Probe_impl::probe_type pt = probe_.probe(timeout, Probe::ready_read|Probe::ready_write);
    if (pt.empty()) return false;
    return true;
}
//####################################################################
void Netxx::Socket::close (void) {
    if (socketfd_ != -1) {
#	if defined(WIN32)
	    closesocket(socketfd_);
#	else
	    ::close(socketfd_);
#	endif

	socketfd_ = -1;
    }
}
//####################################################################
void Netxx::Socket::release (void) {
  socketfd_ = -1;
}
//####################################################################
Netxx::Socket::Type Netxx::Socket::get_type (void) const {
    if (!type_ready_) throw Exception("Netxx bug: Socket::get_type called when !type_ready_");
    return type_;
}
//####################################################################
int Netxx::Socket::get_socketfd (void) const {
    return socketfd_;
}
//####################################################################
void Netxx::Socket::set_socketfd (socket_type socketfd) {
    close();
    probe_ready_ = false;
    socketfd_ = socketfd;
    probe_.clear();
}
//####################################################################
bool Netxx::Socket::operator! (void) const {
    return socketfd_ == -1;
}
//####################################################################
bool Netxx::Socket::operator< (const Socket &other) const {
    return socketfd_ < other.socketfd_;
}
//####################################################################
bool Netxx::Socket::operator< (socket_type socketfd) const {
    return socketfd_ < socketfd;
}
//####################################################################
bool Netxx::operator== (const Netxx::Socket &first, const Netxx::Socket &second) {
    return first.get_socketfd() == second.get_socketfd();
}
//####################################################################
bool Netxx::operator!= (const Netxx::Socket &first, const Netxx::Socket &second) {
    return !(first == second);
}
//####################################################################
namespace {

    //####################################################################
    void get_domain_and_type (Netxx::Socket::Type stype, int &domain, int &type) {
	switch (stype) {
	    case Netxx::Socket::TCP:
		domain = PF_INET;
		type	= SOCK_STREAM;
		break;

	    case Netxx::Socket::UDP:
		domain	= PF_INET;
		type	= SOCK_DGRAM;
		break;

#   ifndef NETXX_NO_INET6
	    case Netxx::Socket::TCP6:
		domain	= PF_INET6;
		type	= SOCK_STREAM;
		break;

	    case Netxx::Socket::UDP6:
		domain	= PF_INET6;
		type	= SOCK_DGRAM;
		break;
#   endif
#   ifndef WIN32
	    case Netxx::Socket::LOCALSTREAM:
		domain	= PF_LOCAL;
		type	= SOCK_STREAM;
		break;

	    case Netxx::Socket::LOCALDGRAM:
		domain	= PF_LOCAL;
		type	= SOCK_DGRAM;
		break;
#   endif
	    default:
		domain = PF_INET;
		type	= SOCK_STREAM;
		break;
	}
    }
    //####################################################################
    
} // end anonymous namespace
