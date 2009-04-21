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
 * This file contains the implementation of the Netxx::Datagram class.
**/

// common header
#include "common.h"

// Netxx includes
#include "Netxx/Datagram.h"
#include "Netxx/Address.h"
#include "Netxx/Peer.h"
#include "Socket.h"
#include "SockAddr.h"
#include "RecvFrom.h"
#include "OSError.h"

// system includes
#if defined (WIN32)
# include <winsock2.h>
#else
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/un.h>
# include <errno.h>
# include <stdio.h>
# include <unistd.h>
# include <sys/stat.h>
#endif

// standard includes
#include <string>
#include <memory>
#include <cstring>
#include <vector>
#include <cstdlib>

namespace {
    void call_connect(Netxx::Socket &socket, const Netxx::Peer &peer, std::vector<std::string> &files);
    void call_bind(Netxx::Socket &socket, std::vector<std::string> &files);
    void get_tmp_filename (std::string &tmp_name);
}

//####################################################################
struct Netxx::Datagram::pimpl {
    pimpl (const Timeout &timeout) : timeout_(timeout) {}
    pimpl (const Timeout &timeout, socket_type socketfd) : timeout_(timeout), socket_(socketfd) {}

    ~pimpl (void) {
#   ifndef WIN32
	if (!files_.empty()) {
	    socket_.close();

	    std::vector<std::string>::const_iterator i(files_.begin()), end(files_.end());
	    for (; i!=end; ++i) unlink(i->c_str());
	}
#   endif
    }

    Timeout timeout_;
    Socket socket_;
    ProbeInfo pi_;
    std::vector<std::string> files_;
};
//####################################################################
Netxx::Datagram::Datagram (const Address &connect_to, const Timeout &timeout) {
    if (!connect_to.size()) throw Exception("no addresses to call connect for");
    std::auto_ptr<pimpl> ap(pimpl_ = new pimpl(timeout));

    call_connect(pimpl_->socket_, *connect_to.begin(), pimpl_->files_);
    pimpl_->pi_.add_socket(pimpl_->socket_.get_socketfd());

    ap.release();
}
//####################################################################
Netxx::Datagram::Datagram (const char *peer_name, port_type default_port, const Timeout &timeout) {
    Address connect_to(peer_name, default_port);
    std::auto_ptr<pimpl> ap(pimpl_ = new pimpl(timeout));

    call_connect(pimpl_->socket_, *connect_to.begin(), pimpl_->files_);
    pimpl_->pi_.add_socket(pimpl_->socket_.get_socketfd());

    ap.release();
}
//####################################################################
Netxx::Datagram::Datagram (socket_type socketfd, const Timeout &timeout) {
    std::auto_ptr<pimpl> ap (pimpl_ = new pimpl(timeout, socketfd));

    pimpl_->pi_.add_socket(pimpl_->socket_.get_socketfd());

    ap.release();
}
//####################################################################
Netxx::Datagram::Datagram (const Timeout &timeout) {
    std::auto_ptr<pimpl> ap (pimpl_ = new pimpl(timeout));

    pimpl_->pi_.add_socket(pimpl_->socket_.get_socketfd());

    ap.release();
}
//####################################################################
Netxx::Datagram::~Datagram (void) {
    delete pimpl_;
}
//####################################################################
Netxx::signed_size_type Netxx::Datagram::send (const Peer &peer, const void *buffer, size_type length) {
#   if defined(WIN32)
	const char *buffer_ptr = static_cast<const char*>(buffer);
#   else
	const void *buffer_ptr = buffer;
#   endif

    const sockaddr *sa = reinterpret_cast<const sockaddr*>(peer.get_sa());
    size_type sa_size = peer.get_sa_size();
    bool correct_type = false;

    /*
     * make sure our socket is setup to handle this type of peer
     */
    if (peer.get_socketfd() != -1 && peer.get_socketfd() == pimpl_->socket_.get_socketfd()) {
	// we have to be ready!
	correct_type = true;
    } else if (pimpl_->socket_.get_socketfd() >= 0) {
	Socket::Type stype = pimpl_->socket_.get_type();

	switch (stype) {
#   ifndef WIN32
	    case Socket::LOCALDGRAM:
		correct_type = sa->sa_family == AF_LOCAL;
		break;
#   endif

	    case Socket::UDP:
		correct_type = sa->sa_family == AF_INET;
		break;

#   ifndef NETXX_NO_INET6
	    case Socket::UDP6:
		correct_type = sa->sa_family == AF_INET6;
		break;
#   endif

	    default:
		break;
	}
    }

    if (!correct_type) {
	Socket::Type stype;

	switch (sa->sa_family) {
	    case AF_INET:
		stype = Socket::UDP;
		break;

#   ifndef NETXX_NO_INET6
	    case AF_INET6:
		stype = Socket::UDP6;
		break;
#   endif

#   ifndef WIN32
	    case AF_LOCAL:
		stype = Socket::LOCALDGRAM;
		break;
#   endif
	    default:
		stype = Socket::UDP;
	}

	Socket tmp_socket(stype);
	pimpl_->socket_.swap(tmp_socket);
	call_bind(pimpl_->socket_, pimpl_->files_);
    }

    if (pimpl_->timeout_ && !pimpl_->socket_.writable(pimpl_->timeout_))
	return -1;

    signed_size_type rc;
    if ( (rc = sendto(pimpl_->socket_.get_socketfd(), buffer_ptr, length, 0, sa, sa_size)) < 0) {
	std::string error("sendto(2) failure: ");
	error += strerror(get_last_error());
	throw Exception(error);
    }

    return rc;
}
//####################################################################
Netxx::Datagram::receive_type Netxx::Datagram::receive (void *buffer, size_type length) {
    /*
     * if this is not a connected datagram, the socket might not be ready
     * yet. We can't receive a datagram because we don't know if it should
     * be a AF_INET, AF_INET6 or even a AF_LOCAL datagram.
     */
    if (pimpl_->socket_.get_socketfd() < 0)
	throw Exception("can't receive datagram unless one is sent first");

    if (pimpl_->timeout_ && !pimpl_->socket_.readable(pimpl_->timeout_))
	return std::make_pair(-1, Peer());

    return call_recvfrom(pimpl_->socket_, buffer, length);
}
//####################################################################
Netxx::signed_size_type Netxx::Datagram::write (const void *buffer, size_type length) {
    return pimpl_->socket_.write(buffer, length, pimpl_->timeout_);
}
//####################################################################
Netxx::signed_size_type Netxx::Datagram::read (void *buffer, size_type length) {
    return pimpl_->socket_.read(buffer, length, pimpl_->timeout_);
}
//####################################################################
void Netxx::Datagram::close (void) {
    pimpl_->socket_.close();
    pimpl_->pi_.clear();
}
//####################################################################
void Netxx::Datagram::release (void) {
    pimpl_->socket_.release();
}
//####################################################################
const Netxx::ProbeInfo* Netxx::Datagram::get_probe_info (void) const {
    return &(pimpl_->pi_);
}
//####################################################################

namespace {

    //####################################################################
    void call_connect (Netxx::Socket &socket, const Netxx::Peer &peer, std::vector<std::string> &files) {
	const sockaddr *sa = reinterpret_cast<const sockaddr*>(peer.get_sa());
	Netxx::size_type sa_size = peer.get_sa_size();
	Netxx::Socket::Type stype;

	switch (sa->sa_family) {
	    case AF_INET:
		stype = Netxx::Socket::UDP;
		break;


# ifndef NETXX_NO_INET6
	    case AF_INET6:
		stype = Netxx::Socket::UDP6;
		break;
# endif

# ifndef WIN32
	    case AF_LOCAL:
		stype = Netxx::Socket::LOCALDGRAM;
		break;
# endif

	    default:
		stype = Netxx::Socket::UDP;
	}

	Netxx::Socket tmp_socket(stype);
	socket.swap(tmp_socket);
	call_bind(socket, files);

	if (connect(socket.get_socketfd(), sa, sa_size) != 0) {
	    std::string error("connect(2) failed: ");
	    error += strerror(Netxx::get_last_error());
	    throw Netxx::Exception(error);
	}
    }
    //####################################################################
    void call_bind (Netxx::Socket &socket, std::vector<std::string> &files) {
#	ifndef WIN32
	    if (socket.get_type() == Netxx::Socket::LOCALDGRAM) {
		Netxx::SockAddr socket_addr(socket.get_type());
		sockaddr_un *saun = reinterpret_cast<sockaddr_un*>(socket_addr.get_sa());
		Netxx::size_type saun_size = socket_addr.get_sa_size();

		std::string tmp_name;
		get_tmp_filename(tmp_name);
		files.push_back(tmp_name);

		if (tmp_name.size() >= sizeof(saun->sun_path))
		    throw Netxx::Exception("temp name is too large for sockaddr_un");

		std::strcpy(saun->sun_path, tmp_name.c_str());
		
		if (bind(socket.get_socketfd(), reinterpret_cast<sockaddr*>(saun), saun_size) != 0) {
		    std::string error("bind(2) error: ");
		    error += strerror(Netxx::get_last_error());
		    throw Netxx::Exception(error);
		}

		/*
		 * This will make a local domain socket more like a real
		 * socket since anyone with local file system access can
		 * connect to it. This may be a bad thing in this case
		 * though. More to come.
		 */
		chmod(saun->sun_path, 0666);
	    }
#	endif
    }
    //####################################################################
    void get_tmp_filename (std::string &tmp_name) {
#	ifndef WIN32
	    char buffer[] = "/tmp/Netxx_Domain_Socket.XXXXXXXXXXXXXXXXX";
	    int fd;

	    if ( (fd = mkstemp(buffer)) < 0) {
		std::string error("can't create temporary file: ");
		error += strerror(Netxx::get_last_error());
		throw Netxx::Exception(error);
	    }

	    /*
	     * WARNING WARNING WARNING WARNING
	     *
	     * The following action could create a race condition.  It may
	     * not be that big of a deal in this case since when we call
	     * bind(2) and it will fail if the file in sun_path already
	     * exists. 
	     *
	     * So, if we close and unlink the file, and then before we call
	     * bind(2) someone creates a file with the same name, bind
	     * should fail.
	     *
	     * It might just be better to use tmpnam(3) but then you have to
	     * see the linker warning about tmpnam might be used in an
	     * unsafe manner. I think what we have here should be good
	     * enough.
	     *
	     * WARNING WARNING WARNING WARNING
	     */
	    close(fd);
	    unlink(buffer);

	    tmp_name = buffer;
#	endif
    }
    //####################################################################

} // end anonymous namespace
