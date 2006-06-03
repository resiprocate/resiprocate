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
 * This file contains the implementation of the Netxx::StreamServer class.
**/

// common header
#include "common.h"

// definition include
#include "Netxx/StreamServer.h"

// Netxx includes
#include "Netxx/Address.h"
#include "Netxx/Types.h"
#include "Netxx/ProbeInfo.h"
#include "ServerBase.h"
#include "Socket.h"
#include "Accept.h"
#include "OSError.h"

// standard includes
#include <memory>

// system includes
#if defined(WIN32)
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

//####################################################################
namespace {
    void call_listen (Netxx::Socket *sockets, Netxx::size_type sockets_size, int backlog);
}
//####################################################################
Netxx::StreamServer::StreamServer (port_type port, const Timeout &timeout, int listen_backlog) {
    std::auto_ptr<ServerBase> ap(pimpl_ = new ServerBase(timeout));

    Address addr;
    addr.add_all_addresses(port);
    init(addr, listen_backlog);

    ap.release();
}
//####################################################################
Netxx::StreamServer::StreamServer (const Address &addr, const Timeout &timeout, int listen_backlog) {
    std::auto_ptr<ServerBase> ap(pimpl_ = new ServerBase(timeout));

    init(addr, listen_backlog);

    ap.release();
}
//####################################################################
Netxx::StreamServer::~StreamServer (void) {
    delete pimpl_;
}
//####################################################################
Netxx::Peer Netxx::StreamServer::accept_connection (void) {
    Socket *ready_socket = pimpl_->get_readable_socket();
    if (!ready_socket) return Peer();
    return call_accept(*ready_socket, pimpl_->get_timeout());
}
//####################################################################
const Netxx::ProbeInfo* Netxx::StreamServer::get_probe_info (void) const {
    return pimpl_->get_probe_info();
}
//####################################################################
void Netxx::StreamServer::init (const Address &addr, int backlog) {
    pimpl_->bind_to(addr, true);

    Socket *sockets;
    size_type sockets_size;

    pimpl_->get_socket_list(sockets, sockets_size);
    call_listen(sockets, sockets_size, backlog);
}
//####################################################################

namespace {

    //####################################################################
    void call_listen (Netxx::Socket *sockets, Netxx::size_type sockets_size, int backlog) {
	for (Netxx::size_type i=0; i<sockets_size; ++i) {
	    if (listen(sockets[i].get_socketfd(), backlog) != 0) {
		std::string error("listen(2) error: ");
		error += strerror(Netxx::get_last_error());
		throw Netxx::Exception(error);
	    }
	}
    }
    //####################################################################
    
} // end anonymous namespace

namespace Netxx {

    //####################################################################
    bool operator== (const StreamServer &ss, socket_type fd)
    { return ss.pimpl_->has_socket(fd); }
    //####################################################################
    bool operator== (socket_type fd, const StreamServer &ss)
    { return ss == fd; }
    //####################################################################
    bool operator!= (const StreamServer &ss, socket_type fd)
    { return !(ss == fd); }
    //####################################################################
    bool operator!= (socket_type fd, const StreamServer &ss)
    { return !(ss == fd); }
    //####################################################################

}
