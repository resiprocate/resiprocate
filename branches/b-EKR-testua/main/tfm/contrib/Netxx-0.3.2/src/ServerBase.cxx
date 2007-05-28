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
 * This file contains the implementation of the Netxx::ServerBase class.
**/

// common header
#include "common.h"

// Netxx includes
#include "ServerBase.h"
#include "Netxx/Address.h"
#include "Netxx/SockOpt.h"
#include "Netxx/ProbeInfo.h"
#include "OSError.h"
#include "Socket.h"

// system includes
#if defined(WIN32)
# include <winsock2.h>
#else
# include <unistd.h>
# include <errno.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <sys/param.h>
# include <sys/un.h>
# include <sys/stat.h>
#endif

// standard includes
#include <map>
#include <vector>
#include <algorithm>
#include <functional>

//####################################################################
namespace {
#   ifndef WIN32
	struct unlink_functor : public std::unary_function<std::string, void> {
	    void operator() (const std::string &file)
	    { unlink(file.c_str()); }
	};
#   endif
}
//####################################################################
Netxx::ServerBase::ServerBase (const Timeout &timeout)
    : timeout_(timeout), sockets_(0), sockets_size_(0)
{ /* must not call bind_to from here */ }
//####################################################################
Netxx::ServerBase::~ServerBase (void) {
#   ifndef WIN32
	if (sockets_ && !files_.empty()) {
	    std::for_each(sockets_, sockets_ + sockets_size_, std::mem_fun_ref(&Socket::close));
	    std::for_each(files_.begin(), files_.end(), unlink_functor());
	}
#   endif

    if (sockets_) delete [] sockets_;
}
//####################################################################
void Netxx::ServerBase::bind_to(const Address &addr, bool stream_server) {
    if (sockets_) throw Exception("bug in Netxx, ServerBase::sockets_ != 0");

    /*
     * it is safe to allocate this block of memory because the destructor
     * will free it even if this function dies with an exception.
     */
    sockets_size_ = addr.size();
    sockets_ = new Socket[sockets_size_];

    /*
     * now that we have an array of sockets we need to walk through each one
     * and set it up. We will use a temporay socket and make it call
     * socket() for us. Then we can set the reuse address socket option and
     * call bind.
     */
    Address::const_iterator addr_it=addr.begin(), addr_end=addr.end();
    for (size_type current_socket=0; addr_it != addr_end; ++addr_it, ++current_socket) {
	const sockaddr *sa = static_cast<const sockaddr*>(addr_it->get_sa());
	size_type sa_size = addr_it->get_sa_size();

	Socket::Type stype;
	switch (sa->sa_family) {
	    case AF_INET:
		stype = stream_server ? Socket::TCP : Socket::UDP;
		break;

#	ifndef NETXX_NO_INET6
	    case AF_INET6:
		stype = stream_server ? Socket::TCP6 : Socket::UDP6;
		break;
#	endif

#	ifndef WIN32
	    case AF_LOCAL:
		stype = stream_server ? Socket::LOCALSTREAM : Socket::LOCALDGRAM;
		break;
#	endif

	    default:
		stype = stream_server ? Socket::TCP : Socket::UDP;
		break;
	}

	Socket tmp_socket(stype);
	socket_type socketfd = tmp_socket.get_socketfd();
	tmp_socket.swap(sockets_[current_socket]);

	SockOpt socket_opt(socketfd);
	socket_opt.set_reuse_address();

	if (bind(socketfd, sa, sa_size) != 0) {
	    std::string error("bind(2) error: ");
	    error += strerror(Netxx::get_last_error());
	    throw Exception(error);
	}

	sockets_map_[socketfd] = &(sockets_[current_socket]);
	pi_.add_socket(socketfd);

#	ifndef WIN32
	    /*
	     * check to see if we need to record a filename. we would need
	     * to record a filename for local domain sockets in order to
	     * remove the file when the server is done with it.
	     *
	     * also take this time to set the mode for the socket file to
	     * something sane. this may be a bad assumption here but I think
	     * this is a good value for a domain socket.
	     */
	    if (sa->sa_family == AF_LOCAL) {
		const sockaddr_un *saun = reinterpret_cast<const sockaddr_un*>(sa);

		/*
		 * This will make a local domain socket more like a real
		 * socket since anyone with local file system access can
		 * connect to it.
		 */
		chmod(saun->sun_path, 0666);

		if (saun->sun_path[0] == '/') {
		    files_.push_back(saun->sun_path);
		} else {
		    char buffer[MAXPATHLEN];

		    if (getcwd(buffer, sizeof(buffer))) {
 			std::string fullpath = buffer; fullpath += '/'; fullpath += saun->sun_path;
			files_.push_back(fullpath);
		    } else {
			files_.push_back(saun->sun_path);
		    }
		}
	    }
#	endif
    }

    probe_.add(*this);
}
//####################################################################
void Netxx::ServerBase::get_socket_list (Socket *&sockets, size_type &size) {
    sockets = sockets_;
    size = sockets_size_;
}
//####################################################################
Netxx::Socket* Netxx::ServerBase::get_readable_socket (void) {
    Probe::result_type rt = probe_.ready(timeout_, Probe::ready_read);
    if (rt.first == -1) return 0;
    return sockets_map_[rt.first];
}
//####################################################################
void Netxx::ServerBase::set_timeout (const Timeout &timeout) {
    timeout_ = timeout;
}
//####################################################################
const Netxx::Timeout& Netxx::ServerBase::get_timeout (void) const {
    return timeout_;
}
//####################################################################
const Netxx::ProbeInfo* Netxx::ServerBase::get_probe_info (void) const {
    return &pi_;
}
//####################################################################
bool Netxx::ServerBase::has_socket (socket_type socketfd) const {
    return sockets_map_.find(socketfd) != sockets_map_.end();
}
//####################################################################
