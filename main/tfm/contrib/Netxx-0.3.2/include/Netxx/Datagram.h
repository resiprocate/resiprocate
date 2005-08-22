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
 * This file contains the definition of the Netxx::Datagram class.
**/

#ifndef _Netxx_Datagram_h_
#define _Netxx_Datagram_h_

// Netxx includes
#include <Netxx/Types.h>
#include <Netxx/Address.h>
#include <Netxx/Timeout.h>

// standard includes
#include <string>
#include <utility>

namespace Netxx {

    // forward declarations
    class ProbeInfo;

/**
 * The Netxx::Datagram class is a simple wrapper class for sending
 * datagrams. It supports both connected and unconnected datagram sockets.
**/
class Datagram {
public:
    /// the type returned from Datagram::receive()
    typedef std::pair<signed_size_type, Peer> receive_type;

    //####################################################################
    /** 
     * Construct a new Netxx::Datagram object. If you use this constructor
     * you must use the send() and receive() member functions instead of the
     * read() and write() member functions.
     *
     * @param timeout An optional timeout for datagram operations.
     * @author Peter Jones
    **/
    //####################################################################
    explicit Datagram (const Timeout &timeout=Timeout());

    //####################################################################
    /** 
     * Construct a new Netxx::Datagram object and connect it to the given
     * peer. Connected Datagram objects can use the read() and write()
     * member functions. Any data send or received will be to/from the
     * connected peer.
     *
     * @param connect_to The peer to connect to.
     * @param timeout An optional timeout for datagram operations.
     * @author Peter Jones
    **/
    //####################################################################
    explicit Datagram (const Address &connect_to, const Timeout &timeout=Timeout());

    //####################################################################
    /** 
     * Construct a new Netxx::Datagram object and connect it to the given
     * peer name and port number. The peer name and port number are given to
     * the Netxx::Address so please see that documentation for more info.
     * 
     * Connected Datagram objects can use the read() and write() member
     * functions. Any data send or received will be to/from the connected
     * peer.
     *
     * @param peer_name The name of the peer to connect to with optional port number.
     * @param default_port The port to use if none is given in peer_name.
     * @param timeout An optional timeout to use for datagram operations.
     * @see Netxx::Address
     * @author Peter Jones
    **/
    //####################################################################
    Datagram (const char *peer_name, port_type default_port, const Timeout &timeout=Timeout());

    //####################################################################
    /** 
     * Create a datagram object using an already existing socket file
     * descriptor. The responsiblity for the socket will be transfered to
     * the new datagram object. You can remove responsibility by calling the
     * release() member function.
     *
     * @param socketfd The file descriptor for the socket to take ownership of.
     * @param timeout An optional timeout for datagram operations.
     * @author Peter Jones
    **/
    //####################################################################
    explicit Datagram (socket_type socketfd, const Timeout &timeout=Timeout());

    //####################################################################
    /** 
     * Class destructor. Clean up after the datagram by closing the socket
     * if necessary.
     *
     * @author Peter Jones
    **/
    //####################################################################
    ~Datagram (void);

    //####################################################################
    /** 
     * Send data to a specific peer address. You cannot use this function if
     * you created the datagram object with the connecting constructor. This
     * function will throw an exception if there are any errors.
     *
     * @param peer The peer to send the data to.
     * @param buffer The buffer to send to the peer.
     * @param length The size of the buffer.
     * @return Greater than or equal to 0: The number of bytes sent to the peer.
     * @return -1: A timeout occured.
     * @author Peter Jones
    **/
    //####################################################################
    signed_size_type send (const Peer &peer, const void *buffer, size_type length);

    //####################################################################
    /** 
     * Receive data from any peer. The peer that sent that data will be
     * returned in a std::pair. This function will throw an exception if
     * an error occures. You can only use this function if you did not use
     * the connecting constructor.
     *
     * @param buffer A place to store the incoming datagram message.
     * @param length The size of the buffer.
     * @return receive_type.first greater than or equal to 0: The number of bytes that were placed in the buffer.
     * @return receive_type.first == -1: A timeout occured.
     * @author Peter Jones
    **/
    //####################################################################
    receive_type receive (void *buffer, size_type length);

    //####################################################################
    /** 
     * Write data to the connected peer. You can only use this function if
     * you used the connecting constructor. An exception will be thrown if
     * an error occures.
     *
     * @param buffer The buffer that contains the messeage to send to the peer.
     * @param length The size of the buffer.
     * @return Greater than or equal to 0: The number of bytes sent to the peer.
     * @return -1 A timeout occured.
     * @author Peter Jones
    **/
    //####################################################################
    signed_size_type write (const void *buffer, size_type length);

    //####################################################################
    /** 
     * Read data from a connected peer into a buffer. You can only call this
     * function if you used the connecting constructor. An exception will be
     * thrown if an error occures.
     *
     * @param buffer The place to store the received message from the peer.
     * @param length The size of the buffer.
     * @return Greater than or equal to 0:The number of bytes placed in your buffer.
     * @return -1 A timeout occured.
     * @author Peter Jones
    **/
    //####################################################################
    signed_size_type read  (void *buffer, size_type length);

    //####################################################################
    /** 
     * Get the file descriptor for the socket this Datagram object is using.
     *
     * @return The current socket file descriptor.
     * @author Peter Jones
    **/
    //####################################################################
    socket_type get_socketfd (void) const;

    //####################################################################
    /** 
     * Close the socket that the datagram object is using if it is open.
     * This is normally done by the datagram class destructor.
     *
     * @author Peter Jones
    **/
    //####################################################################
    void close (void);

    //####################################################################
    /** 
     * Release control of the socket file descriptor that is being used.
     * This will prevent the destructor from closing the socket. Once you
     * call release you must make sure that some other object takes control
     * of the file descriptor or it could be leaked.
     *
     * @author Peter Jones
    **/
    //####################################################################
    void release (void);

    //####################################################################
    /** 
     * Get information about how Datagram objects should be used with the
     * Netxx::Probe class.
     *
     * @return A Netxx::ProbeInfo object.
     * @author Peter Jones
    **/
    //####################################################################
    const ProbeInfo* get_probe_info (void) const;
private:
    struct pimpl; pimpl *pimpl_;
    Datagram (const Datagram&);
    Datagram& operator= (const Datagram&);
}; // end Netxx::Datagram class

} // end Netxx namespace
#endif
