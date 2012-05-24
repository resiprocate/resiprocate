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
 * This file contains the definition of the Netxx::DatagramServer class.
**/

#ifndef _Netxx_DatagramServer_h_
#define _Netxx_DatagramServer_h_

// Netxx includes
#include <Netxx/Types.h>
#include <Netxx/Timeout.h>
#include <Netxx/Address.h>
#include <Netxx/Peer.h>

// standard includes
#include <utility>

namespace Netxx {

    // forward declarations
    class ProbeInfo; 
    class ServerBase;

/**
 * The Netxx::DatagramServer class is used for datagram based servers.
**/
class DatagramServer {
public:
    /// type returned from receive_datagram
    typedef std::pair<signed_size_type, Peer> receive_type;

    //####################################################################
    /** 
     * Create a datagram server that will bind to all local addresses on the
     * given port number. The timeout will be used when you make a call to
     * receive_datagram.
     *
     * @param port The local port number to bind to.
     * @param timeout The timeout to use for server operations.
     * @author Peter Jones
    **/
    //####################################################################
    explicit DatagramServer (port_type port, const Timeout &timeout=Timeout());

    //####################################################################
    /** 
     * Create a datagram server that will bind to all addresses given in the
     * Address class.
     *
     * @param addr The list of addresses to bind to.
     * @param timeout The timeout to use for server operations.
     * @author Peter Jones
    **/
    //####################################################################
    explicit DatagramServer (const Address &addr, const Timeout &timeout=Timeout());

    //####################################################################
    /** 
     * Clean up after the DatagramServer.
     *
     * @author Peter Jones
    **/
    //####################################################################
    ~DatagramServer (void);

    //####################################################################
    /** 
     * Try to receive an incoming datagram. If you have set a timeout and no
     * datagram is pending within that timeout this memeber function will
     * return with -1 for the bytes read and an invalid Peer class. If no
     * timeout was given, this function will block until a datagram arives.
     * Once a datagram has been received it will be stored inside the given
     * buffer and the amount of stored data along with data about the sender
     * of the datagram will returned in a std::pair.
     *
     * @param buffer The buffer where the datagram data can be placed.
     * @param length The size of the buffer.
     * @return A std::pair where first is the bytes placed in the buffer and
     *         second is the Peer class for the sending peer.
     * @author Peter Jones
    **/
    //####################################################################
    receive_type receive (void *buffer, size_type length);

    //####################################################################
    /** 
     * Send a datagram to the given peer. The send will occur using the
     * datagram server socket. This may not be thread safe. Some protocols
     * will send a datagram to the client using the server socket with
     * information about another port to start talking on. It is up to the
     * developer to decide which approach is better, to respond using the
     * server datagram socket or create a new datagram server or client for
     * the communication.
     *
     * Using the same DatagramServer object in multiple threads of execution
     * and then calling the send() member function will probably cause you
     * problems and a debugging nightmare.
     *
     * @param peer The peer to send the datagram to.
     * @param buffer The buffer that contains the datagram to send.
     * @param length The size of the datagram to send.
     * @return The number of bytes sent or -1 in the case of a timeout.
    **/
    //####################################################################
    signed_size_type send (const Peer &peer, const void *buffer, size_type length);

    //####################################################################
    /** 
     * Get information about how this DatagramServer should be probed from
     * the Netxx::Probe class.
     *
     * @return A Netxx::ProbeInfo object.
     * @author Peter Jones
    **/
    //####################################################################
    const ProbeInfo* get_probe_info (void) const;

    friend bool operator== (const DatagramServer &ds, socket_type fd);
    friend bool operator== (socket_type fd, const DatagramServer &ds);
    friend bool operator!= (const DatagramServer &ds, socket_type fd);
    friend bool operator!= (socket_type fd, const DatagramServer &ds);
private:
    ServerBase *pimpl_;
    DatagramServer (const DatagramServer&);
    DatagramServer& operator= (const DatagramServer&);
}; // end Netxx::DatagramServer class

} // end Netxx namespace
#endif
