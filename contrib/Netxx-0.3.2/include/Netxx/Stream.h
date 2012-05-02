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
 * This file contains the definition of the Netxx::Stream class.
**/

#ifndef _Netxx_Stream_h_
#define _Netxx_Stream_h_

// Netxx includes
#include <Netxx/Types.h>
#include <Netxx/StreamBase.h>

namespace Netxx {
    
    // forward declarations
    class ProbeInfo;
	
/**
 * The Netxx::Stream class is used to establish a connection with a peer.
 * The peer to connect to is listed in a Netxx::Address class. The peer may
 * have more than one address in the Netxx::Address and the Netxx::Stream
 * class will connect to the first one possible.
 *
 * This class is mainly for a 'client' application since it makes the
 * connection to a given peer. To make is useful in a server application,
 * this class also has a constructor that you can use to communicate with a
 * client that was returned from a call to accept_connection().
**/
class Stream : public StreamBase {
public:
    //####################################################################
    /** 
     * Construct a new Netxx::Stream and connect to the given peer. The
     * first address in the Netxx::Address class that works will be the one
     * the Stream object is connected to. If the Stream object cannot
     * connect to any of the address an exception is trown.
     *
     * If a timeout is given, it will be used for all stream operations,
     * including the initial connection to the peer.
     *
     * @param address The list of addresses for the peer.
     * @param timeout An optional timeout to use for all operations.
     * @author Peter Jones
    **/
    //####################################################################
    explicit Stream (const Address &address, const Timeout &timeout=Timeout());

    //####################################################################
    /** 
     * Construct a new Netxx::Stream without a Netxx::Address. Instead given
     * a char* that holds the name of the peer. This name will be passed to
     * the Netxx::Address constructor along with the given default port
     * number.
     *
     * @param uri The name of the peer to connect to in a possible URI format.
     * @param default_port The port to use if the given URI does not have one.
     * @param timeout An optional timeout to use for all operations.
     * @see Netxx::Address
     * @author Peter Jones
    **/
    //####################################################################
    explicit Stream (const char *uri, port_type default_port=0, const Timeout &timeout=Timeout());

    //####################################################################
    /** 
     * Construct a new Netxx::Stream and take over an already established
     * connection. This Stream object will then own the connection socket
     * and will close it in its destructor.
     *
     * This is useful for server applications so that they can use a
     * Netxx::Stream object to communicate with a new client that was
     * returned from Netxx::StreamServer::accept_connection().
     *
     * @param socketfd The file descriptor for the socket to own.
     * @param timeout An optional timeout to use for all operations.
     * @author Peter Jones
    **/
    //####################################################################
    explicit Stream (socket_type socketfd, const Timeout &timeout=Timeout());

    //####################################################################
    /** 
     * Netxx::Stream copy constructor. I can't see how this would be very
     * useful but it is provided for safety and completeness. The copy
     * constructor will cause a copy of the socket to be made.
     *
     * @param other The other Netxx::Stream to copy from.
     * @author Peter Jones
    **/
    //####################################################################
    Stream (const Stream &other);

    //####################################################################
    /** 
     * Assignement operator. I can't see how this would be very useful but
     * it is provided for safety and completeness. This function will
     * preform a copy of the give Netxx::Stream object via the Netxx::Stream
     * copy constructor.
     *
     * @param other The other Netxx::Stream to copy from.
     * @author Peter Jones
    **/
    //####################################################################
    Stream& operator= (const Stream &other);

    //####################################################################
    /** 
     * Exchange connection information with another Netxx::Stream. This is
     * similar to std::swap().
     *
     * @param other The other Netxx::Stream object to swap with.
     * @author Peter Jones
    **/
    //####################################################################
    void swap (Stream &other);

    //####################################################################
    /** 
     * Netxx::Stream destructor. Close the connection with the peer if there
     * is one.
     *
     * @author Peter Jones
    **/
    //####################################################################
    ~Stream (void);

    //####################################################################
    /** 
     * Read data from the connected peer and place then in the given buffer.
     * If an error occures this function will throw an exception. If a
     * timeout occures (only if you set a timeout) then -1 is returned.
     *
     * It is possible for this function to return -1 if you did not set a
     * timeout. This would happen if you had set the socket for this
     * connection to non-blocking via the Netxx::SockOpt class.
     *
     * @param buffer The buffer to store the read bytes into.
     * @param length The size of the given buffer.
     * @return Greater than 0: The number of bytes read from the peer and placed into your buffer.
     * @return 0: The peer closed the connection.
     * @return -1: A timeout occured.
     * @see Netxx::SockOpt
     * @author Peter Jones
    **/
    //####################################################################
    signed_size_type read  (void *buffer, size_type length);

    //####################################################################
    /** 
     * Write data from the given buffer to the connected peer. If an error
     * occures this function will throw an exception. If a timeout occures
     * (only if you set a timeout) then -1 is returned.
     *
     * It is possible for this function to return -1 if you did not set a
     * timeout. This would happen if you had set the socket for this
     * connection to non-blocking via the Netxx::SockOpt class.
     *
     * @param buffer The buffer to take the data from and send to the peer.
     * @param length The amount of data to use from the buffer.
     * @return Greater than 0: The number of bytes that were sent to the peer.
     * @return 0: The peer closed the connection.
     * @return -1: A timeout occured.
     * @see Netxx::SockOpt
     * @author Peter Jones
    **/
    //####################################################################
    signed_size_type write (const void *buffer, size_type length);

    //####################################################################
    /** 
     * Close the connection to the peer. Once the connection is closed you
     * should not preform any other operations on this object. This function
     * is normally called from the destructor.
     *
     * @author Peter Jones
    **/
    //####################################################################
    void close (void);

    //####################################################################
    /** 
     * Return the socket file descriptor used for the peer connection. This
     * is useful for calls to the Netxx::SockOpt class, as well as others.
     *
     * @return The socket file descriptor being used by this object.
     * @author Peter Jones
    **/
    //####################################################################
    socket_type get_socketfd (void) const;

    //####################################################################
    /** 
     * Get information about how this Stream should be probed from the
     * Netxx::Probe class.
     *
     * @return A Netxx::ProbeInfo object.
     * @author Peter Jones
    **/
    //####################################################################
    const ProbeInfo* get_probe_info (void) const;
private:
    struct pimpl; pimpl *pimpl_;
}; // end Netxx::Stream class

} // end Netxx namespace
#endif
