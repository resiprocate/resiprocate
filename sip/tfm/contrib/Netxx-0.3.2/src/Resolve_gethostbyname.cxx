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
 * This file contains the implementation of the resolve_hostname function
 * using the gethostbyname system call.
**/

// common header
#include "common.h"

// Netxx includes
#include "Resolve.h"
#include "Netxx/Types.h"
#include "SockAddr.h"
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
# include <netdb.h>
#endif

// standard includes
#include <cstring>

namespace Netxx {

    //####################################################################
    void resolve_hostname (const char *hostname, port_type port, bool use_ipv6, std::vector<Peer> &addrs) {
	{ // first try to see if this address is already an IPv4 address
	    SockAddr saddr(AF_INET, port);
	    sockaddr_in *sai = reinterpret_cast<sockaddr_in*>(saddr.get_sa());;

	    if (inet_pton(AF_INET, hostname, &(sai->sin_addr)) == 1) {
		addrs.push_back(Peer(hostname, port, sai, saddr.get_sa_size()));
		return;
	    }
	}

#   ifndef NETXX_NO_INET6
	if (use_ipv6) { // now try to see if this address is already an IPv6 address
	    SockAddr saddr(AF_INET6, port);
	    sockaddr_in6 *sai6 = reinterpret_cast<sockaddr_in6*>(saddr.get_sa());

	    if (inet_pton(AF_INET6, hostname, &(sai6->sin6_addr)) == 1) {
		addrs.push_back(Peer(hostname, port, sai6, saddr.get_sa_size()));
		return;
	    }
	}
#   endif

	/*
	 * if we got this far then the address must really be a hostname so
	 * we are going to have to call gethostbyname and either pull it out
	 * of the hosts file or go to DNS. And don't forget NIS and WINS and
	 * ...
	 */
	hostent *he; // WARNING not MT safe
	if ( (he = gethostbyname(hostname)) == 0) {
	    std::string error("name resolution failure for "); error += hostname;
	    throw Exception(error);
	}

	for (char **ii = he->h_addr_list; *ii != 0; ++ii) {
	    switch (he->h_addrtype) {
		case AF_INET:
		{
		    SockAddr saddr(AF_INET, port);
		    sockaddr_in *sai = reinterpret_cast<sockaddr_in*>(saddr.get_sa());
		    std::memcpy(&(sai->sin_addr), *ii, sizeof(sai->sin_addr));
		    addrs.push_back(Peer(he->h_name, port, sai, saddr.get_sa_size()));
		}
		break;

#   ifndef NETXX_NO_INET6
		case AF_INET6:
		{
		    SockAddr saddr(AF_INET6, port);
		    sockaddr_in6 *sai6 = reinterpret_cast<sockaddr_in6*>(saddr.get_sa());
		    std::memcpy(&(sai6->sin6_addr), *ii, sizeof(sai6->sin6_addr));
		    addrs.push_back(Peer(he->h_name, port, sai6, saddr.get_sa_size()));
		}
		break;
#    endif
	    }
	}
    }
    //####################################################################

} // end Netxx namespace
