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
 * This file contains the implementation of the Netxx::DatagramServer class.
**/

// common header
#include "common.h"

// Netxx includes
#include "Netxx/DatagramServer.h"
#include "Netxx/Datagram.h"
#include "Netxx/Address.h"
#include "Netxx/ProbeInfo.h"
#include "ServerBase.h"
#include "Socket.h"
#include "RecvFrom.h"

// standard includes
#include <memory>
#include <utility>

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

//####################################################################
Netxx::DatagramServer::DatagramServer (port_type port, const Timeout &timeout) {
    std::auto_ptr<ServerBase> ap(pimpl_ = new ServerBase(timeout));

    Address addr;
    addr.add_all_addresses(port);
    pimpl_->bind_to(addr, false);

    ap.release();
}
//####################################################################
Netxx::DatagramServer::DatagramServer (const Address &addr, const Timeout &timeout) {
    std::auto_ptr<ServerBase> ap(pimpl_ = new ServerBase(timeout));

    pimpl_->bind_to(addr, false);

    ap.release();
}
//####################################################################
Netxx::DatagramServer::~DatagramServer (void) {
    delete pimpl_;
}
//####################################################################
Netxx::DatagramServer::receive_type Netxx::DatagramServer::receive (void *buffer, size_type length) {
    Socket *ready_socket = pimpl_->get_readable_socket();
    if (!ready_socket) return std::make_pair(static_cast<signed_size_type>(-1), Peer());
    return call_recvfrom(*ready_socket, buffer, length);
} 
//####################################################################
Netxx::signed_size_type Netxx::DatagramServer::send (const Peer &peer, const void *buffer, size_type length) {
    /*
     * We will create a Netxx::Datagram to do the dirty work for us. We take
     * care that the Datagram class does not close our server socket! We
     * give the Datagram peer.get_socketfd() because that is the server
     * socket that recevived the datagram.
     */
    Datagram client(peer.get_socketfd(), pimpl_->get_timeout());
    client.release();
    return client.send(peer, buffer, length);
}
//####################################################################
const Netxx::ProbeInfo* Netxx::DatagramServer::get_probe_info (void) const {
    return pimpl_->get_probe_info();
}

namespace Netxx {

    //####################################################################
    bool operator== (const DatagramServer &ds, socket_type fd)
    { return ds.pimpl_->has_socket(fd); }
    //####################################################################
    bool operator== (socket_type fd, const DatagramServer &ds)
    { return ds == fd; }
    //####################################################################
    bool operator!= (const DatagramServer &ds, socket_type fd)
    { return !(ds == fd); }
    //####################################################################
    bool operator!= (socket_type fd, const DatagramServer &ds)
    { return !(ds == fd); }
    //####################################################################

} // end Netxx namespace
