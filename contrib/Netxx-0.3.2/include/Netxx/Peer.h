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
 * This file contains the definition of the Netxx::Peer class.
**/

#ifndef _Netxx_Peer_h_
#define _Netxx_Peer_h_

// Netxx includes
#include <Netxx/Types.h>

// standard includes
#include <iosfwd>

namespace Netxx {

/**
 * The Netxx::Peer class holds information about the Peer you are talking
 * and/or connected to.
**/
class Peer {
public:
    //####################################################################
    /** 
     * Default constructor for an invalid peer.
     *
     * @author Peter Jones
    **/
    //####################################################################
    Peer (void);

    //####################################################################
    /** 
     * Constructor that takes the address of the peer.
     *
     * @param addr The string form of the peer's address
     * @param port The peer's port.
     * @param saddr The peer address struct.
     * @param saddr_size The size of saddr.
     * @author Peter Jones
    **/
    //####################################################################
    Peer (const char *addr, port_type port, void *saddr, size_type saddr_size);

    //####################################################################
    /** 
     * Create a new Peer where the socket file descriptor used to
     * communicate with the peer is given in socketfd and the sockaddr
     * struct for the peer is given in saddr.
     *
     * @param socketfd The socket file descriptor for the peer.
     * @param saddr The sockaddr struct for the peer.
     * @param saddr_size The size of the saddr struct.
     * @author Peter Jones
    **/
    //####################################################################
    Peer (socket_type socketfd, void *saddr, size_type saddr_size);

    //####################################################################
    /** 
     * Copy constructor.
     *
     * @param other The other Peer class to copy from.
     * @author Peter Jones
    **/
    //####################################################################
    Peer (const Peer &other);

    //####################################################################
    /** 
     * Assignment operator.
     *
     * @param other The other Peer class to assign from.
     * @author Peter Jones
    **/
    //####################################################################
    Peer& operator= (const Peer &other);

    //####################################################################
    /** 
     * Swap this Peer class with another one.
     *
     * @param other Peer class to swap with.
     * @author Peter Jones
    **/
    //####################################################################
    void swap (Peer &other);

    //####################################################################
    /** 
     * Class destructor.
     *
     * @author Peter Jones
    **/
    //####################################################################
    ~Peer (void);

    //####################################################################
    /** 
     * Get the string version of the address of the peer.
     *
     * @return The address of the peer.
     * @author Peter Jones
    **/
    //####################################################################
    const char* get_address (void) const;

    //####################################################################
    /** 
     * Get the port number for the peer.
     *
     * @return The port number of the peer.
     * @author Peter Jones
    **/
    //####################################################################
    port_type get_port (void) const;
    
    //####################################################################
    /** 
     * Get the local port number for the peer.
     *
     * @return The local port number of the peer.
     * @author Alex Mitrofanov
    **/
    //####################################################################
    port_type get_local_port (void) const;
    
    //####################################################################
    /** 
     * Get the socket file descriptor for this peer. This member function
     * will return -1 if there is no socket file descriptor for this peer.
     *
     * @return The socket file descriptor or -1.
     * @author Peter Jones
    **/
    //####################################################################
    socket_type get_socketfd (void) const;

    //####################################################################
    /** 
     * Test to see if a peer is valid.
     *
     * @return True if the peer is valid.
     * @return False if the peer is invalid.
     * @author Peter Jones
    **/
    //####################################################################
    operator bool (void) const;

    //####################################################################
    /** 
     * Get the sockaddr struct as a void pointer if this is a valid Peer
     * object.
     *
     * @return A void pointer to the sockaddr struct.
     * @return 0 If this Peer is invalid.
     * @author Peter Jones
    **/
    //####################################################################
    const void* get_sa (void) const;

    //####################################################################
    /** 
     * Get the size of the sockaddr struct if this is a valid Peer object.
     *
     * @return The size of the sockaddr struct.
     * @return 0 If this Peer is invalid.
     * @author Peter Jones
    **/
    //####################################################################
    size_type get_sa_size (void) const;
private:
    bool okay_;
    std::string addr_;
    port_type port_;
    socket_type socketfd_;
    void *sockaddr_;
    size_type sockaddr_size_;
}; // end Netxx::Peer class

//####################################################################
/** 
 * Insert the string form of the peer into the given stream.
 *
 * @param stream The stream to insert the peer into.
 * @param peer The peer to insert.
 * @return The stream from the first param.
 * @author Peter Jones
**/
//####################################################################
std::ostream& operator<< (std::ostream &stream, const Peer &peer);

} // end Netxx namespace
#endif
