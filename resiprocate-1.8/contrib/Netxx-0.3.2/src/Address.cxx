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
 * This file contains the implementation for the Netxx::Address class and
 * some of the necessary helper functions.
**/

// common header
#include "common.h"

// Netxx includes
#include "Netxx/Address.h"
#include "Netxx/Types.h"
#include "SockAddr.h"
#include "Resolve.h"

// system includes
#if defined(WIN32)
# include <winsock2.h>
#else
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <sys/un.h>
#endif

// standard includes
#include <cstdlib>
#include <cstring>

namespace {
    const char const_local_service[] = "local";

    bool parse_uri (const char *uri, std::string &protocol, std::string &name, Netxx::port_type &port, std::string &path);
    void make_uds (const char *filename, Netxx::Address::container_type &addrs);
}

//####################################################################
Netxx::Address::Address (const char *uri, port_type default_port, bool use_ipv6)
    : port_(0), ipv6_(use_ipv6)
{
#   if defined(WIN32)
	WSADATA wsdata;
	
	if (WSAStartup(MAKEWORD(2,2), &wsdata) != 0) {
	    throw Exception("failed to load WinSock");
	}
#   endif

    add_address(uri, default_port);
}
//####################################################################
Netxx::Address::Address (bool use_ipv6)
    : port_(0), ipv6_(use_ipv6)
{
#   if defined(WIN32)
	WSADATA wsdata;
	
	if (WSAStartup(MAKEWORD(2,2), &wsdata) != 0) {
	    throw Exception("failed to load WinSock");
	}
#   endif
}
//####################################################################
Netxx::Address::~Address (void) {
#   if defined(WIN32)
	WSACleanup();
#   endif
}
//####################################################################
const char* Netxx::Address::get_protocol (void) const {
    if (!protocol_.empty()) return protocol_.c_str();
    return 0;
}
//####################################################################
const char* Netxx::Address::get_name (void) const {
    if (!name_.empty()) return name_.c_str();
    return 0;
}
//####################################################################
const char* Netxx::Address::get_path (void) const {
    if (!path_.empty()) return path_.c_str();
    return 0;
}
//####################################################################
Netxx::port_type Netxx::Address::get_port (void) const {
    return port_;
}
//####################################################################
Netxx::Address::const_iterator Netxx::Address::begin (void) const {
    return addrs_.begin();
}
//####################################################################
Netxx::Address::const_iterator Netxx::Address::end (void) const {
    return addrs_.end();
}
//####################################################################
Netxx::size_type Netxx::Address::size (void) const {
    return addrs_.size();
}
//####################################################################
void Netxx::Address::add_address(const char *uri, port_type default_port) {
    port_type tmp_port=0;
    std::string protocol;

    if (!parse_uri(uri, protocol, name_, tmp_port, path_)) {
	std::string error("can't parse URI: "); error += uri;
	throw Exception(error);
    }

    if (!protocol.empty() && std::strcmp(protocol.c_str(), const_local_service) == 0) {
	make_uds(name_.c_str(), addrs_);
	return;
    }

    if (!tmp_port && !protocol.empty()) tmp_port = resolve_service(protocol_.c_str());
    if (!tmp_port) tmp_port = default_port;

    port_ = tmp_port;
    protocol_ = protocol;
    resolve_hostname(name_.c_str(), port_, ipv6_, addrs_);
}
//####################################################################
void Netxx::Address::add_all_addresses (port_type port) {
    { // new scope just for safety
	SockAddr saddr(AF_INET, port);
	sockaddr_in *sai = reinterpret_cast<sockaddr_in*>(saddr.get_sa());
	sai->sin_addr.s_addr = htons(INADDR_ANY);
	addrs_.push_back(Peer("localhost", port, sai, saddr.get_sa_size()));
    }

# ifndef NETXX_NO_INET6
    if (ipv6_) {
	SockAddr saddr(AF_INET6, port);
	sockaddr_in6 *sai6 = reinterpret_cast<sockaddr_in6*>(saddr.get_sa());
	sai6->sin6_addr	= in6addr_any;
	addrs_.push_back(Peer("localhost", port, sai6, saddr.get_sa_size()));
    }
# endif
}
//####################################################################

namespace {

    //####################################################################
    bool parse_uri (const char *uri, std::string &protocol, std::string &name, Netxx::port_type &port, std::string &path) {
	const char *start_pos = uri, *stop_pos;

#	ifndef WIN32
	    // see if the URI is a Unix filepath
	    if (*uri == '/') {
		protocol = const_local_service;
		name = uri;
		return true;
	    }
#	endif

	// first look for the protocol seperator
	while (*uri != 0 && *uri != ':') ++uri;

	if (*uri == ':' && *(uri+1) == '/' && *(uri+2) == '/') {
	    // looks like we may have a protocol/service name
	    
	    if (uri != start_pos) protocol.assign(start_pos, uri - start_pos);
	    start_pos = uri + 3;

#	    ifndef WIN32
		// check to see if it is a local domain socket
		if (std::strcmp(protocol.c_str(), const_local_service) == 0) {
		    name = start_pos;
		    return true;
		}
#	    endif
	}

	uri = start_pos;

	// now look for any path info
	while (*uri != 0 && *uri != '/') ++uri;

	if (*uri == '/') {
	    // grab the path info
	    path = uri;
	}

	stop_pos = uri;
	uri = start_pos;

	// check for a port number in the hostname
	while (*uri != 0 && uri != stop_pos && *uri != ':') ++uri;

	if (*uri == ':') {
	    std::string tmp_port(uri+1, stop_pos - uri - 1);
	    if (*(uri+1) != 0) port = static_cast<Netxx::port_type>(std::atoi(tmp_port.c_str()));
	}

	stop_pos = uri;
	uri = start_pos;

	// all we should have left is the hostname
	if (uri == stop_pos) return false;
	name.assign(uri, stop_pos - uri);
	return true;
    }
    //####################################################################
    void make_uds (const char *filename, Netxx::Address::container_type &addrs) {
#   ifndef WIN32

	Netxx::SockAddr saddr(AF_LOCAL);
	sockaddr_un *sau = reinterpret_cast<sockaddr_un*>(saddr.get_sa());

	std::strncpy(sau->sun_path, filename, sizeof(sau->sun_path) - 1);
	addrs.push_back(Netxx::Peer(filename, 0, sau, saddr.get_sa_size()));

#   endif
    }
    //####################################################################
} // end anonymous namespace
