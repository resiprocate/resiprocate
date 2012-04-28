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
 * This file contains the definition of the Netxx::StreamServer class.
**/

#ifndef _Netxx_StreamServer_h_
#define _Netxx_StreamServer_h_

// Netxx includes
#include <Netxx/Types.h>
#include <Netxx/Timeout.h>
#include <Netxx/Address.h>
#include <Netxx/Peer.h>

namespace Netxx {

    // forward declaration
    class ProbeInfo;
    class ServerBase;

/**
 * The Netxx::StreamServer class is used for stream based servers like TCP
 * through IPv4, TCP through IPv6 and TCP through Local Domain Sockets.
**/
class StreamServer {
public:
    //####################################################################
    /** 
     * Create a StreamServer that will bind to all local addresses using the
     * given port. You can also set a timeout value and a listen(2) backlog
     * value.
     *
     * @param port The port to use on all local interfaces.
     * @param timeout The timeout to use. Default is to block.
     * @param listen_backlog The listen(2) backlog parameter.
     * @author Peter Jones
    **/
    //####################################################################
    StreamServer (port_type port, const Timeout &timeout=Timeout(), int listen_backlog=128);

    //####################################################################
    /** 
     * Create a StreamServer that will bind to each address given in addr.
     * You can also set the timeout and listen(2) backlog value.
     *
     * @param addr The addresses to bind to.
     * @param timeout The timeout to use. Default is to block.
     * @param listen_backlog The listen(2) backlog parameter.
     * @author Peter Jones
    **/
    //####################################################################
    StreamServer (const Address &addr, const Timeout &timeout=Timeout(), int listen_backlog=128);

    //####################################################################
    /** 
     * Clean up after the StreamServer. Any sockets in use will be closed.
     *
     * @author Peter Jones
    **/
    //####################################################################
    ~StreamServer (void);

    //####################################################################
    /** 
     * Try to accept a connection from a Peer. The returned value will
     * indicate whether or not a connection was established or a timeout
     * occured. This member function will throw an exception if there is an
     * error.
     *
     * @return A Peer that contains info about the connected peer.
     * @return An invalid Peer to signal a timeout.
     * @see Netxx::Peer
     * @author Peter Jones
    **/
    //####################################################################
    Peer accept_connection (void);

    //####################################################################
    /** 
     * Get information about how this StreamServer should be probed from the
     * Netxx::Probe class.
     *
     * @return A Netxx::ProbeInfo object.
     * @author Peter Jones
    **/
    //####################################################################
    const ProbeInfo* get_probe_info (void) const;

    friend bool operator== (const StreamServer &ss, socket_type fd);
    friend bool operator== (socket_type fd, const StreamServer &ss);
    friend bool operator!= (const StreamServer &ss, socket_type fd);
    friend bool operator!= (socket_type fd, const StreamServer &ss);
private:
    ServerBase *pimpl_;
    StreamServer (const StreamServer&);
    StreamServer& operator= (const StreamServer&);
    void init (const Address &addr, int backlog);
}; // end Netxx::StreamServer class

} // end Netxx namespace
#endif
