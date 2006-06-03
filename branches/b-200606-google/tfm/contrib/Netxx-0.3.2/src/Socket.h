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
 * This file contains the definition of the Netxx::Socket class.
**/

#ifndef _Netxx_Socket_h_
#define _Netxx_Socket_h_

// Netxx includes
#include <Netxx/Types.h>
#include <Netxx/Address.h>
#include "Probe_impl.h"

namespace Netxx {
    class Timeout;
	
/**
 * The Socket class is a wrapper around the operating system socket
 * functions. It is the low level socket interface for the Netxx code, due
 * to the fact that there are at least two higher level classes (Server and
 * Client).
**/
class Socket {
public:
    enum Type {
	TCP,		///< TCP IPv4 Address
	TCP6,		///< TCP IPv6 Address
	UDP,		///< UDP IPv4 Address
	UDP6,		///< UDP IPv6 Address
	LOCALSTREAM,	///< Unix Domain TCP
	LOCALDGRAM	///< Unix Domain UDP
    };

    Socket (void);

    explicit Socket (Type type);

    //####################################################################
    /** 
     * Socket class ctor that creates a socket from a already connected
     * socket file descriptor.
     *
     * @param sockedfd The socket file descriptor to use
     * @author Peter Jones
    **/
    //####################################################################
    explicit Socket (socket_type socketfd);

    //####################################################################
    /** 
     * Socket class copy ctor. The ctor will copy the other socket, so
     * you will have two sockets with the same file desc.
     *
     * @param other The other Socket
     * @author Peter Jones
    **/
    //####################################################################
    Socket (const Socket &other);

    //####################################################################
    /** 
     * Assignment operator
     *
     * @param other The other Socket
     * @author Peter Jones
    **/
    //####################################################################
    Socket& operator= (const Socket& other);

    //####################################################################
    /** 
     * Swap this socket for another one.
     *
     * @param other The other Socket to swap with
     * @author Peter Jones
    **/
    //####################################################################
    void swap (Socket &other);

    //####################################################################
    /** 
     * Socket class dtor
     *
     * @author Peter Jones
    **/
    //####################################################################
    ~Socket (void);

    //####################################################################
    /** 
     * Read data from the socket and place it into the user buffer. If the given
     * timeout is not a null timeout then this method will only wait for data
     * until the timeout expires, at which time it will return -1. If you give a
     * null timeout, this method will block until data arives.
     *
     * @param buffer The user buffer to write data into
     * @param length The max bytes to place into buffer
     * @param timeout How long to wait for data to arive
     * @return The number of bytes read from the socket, or -1 for timeout
     * @author Peter Jones
    **/
    //####################################################################
    signed_size_type read  (void *buffer, size_type length, const Timeout &timeout);

    //####################################################################
    /** 
     * Write data from a user buffer to the socket. If the given timeout
     * is not a null timeout then this method will only wait for the
     * socket to become writable until the timeout expires, at which
     * point it will return -1. If you give a null timeout then this
     * method will block until all data has been written to the socket.
     *
     * @param buffer The user buffer to read from
     * @param length The number of bytes to read from the user buffer
     * @return The number of bytes written, -1 for timeout
     * @author Peter Jones
    **/
    //####################################################################
    signed_size_type write (const void *buffer, size_type length, const Timeout &timeout);

    //####################################################################
    /** 
     * Check to see if there is data to read from a socket. If you give a valid
     * timeout then this method will only block for that timeout.
     *
     * @param timeout The amount of time to wait for the socket to become
     * 	readable
     * @return True if there is data to read, false otherwise
     * @author Peter Jones
    **/
    //####################################################################
    bool readable (const Timeout &timeout);

    //####################################################################
    /** 
     * Check to see if a socket can be written to. If you give a timeout
     * value then this method will not block longer then the timeout.
     *
     * @param timeout How long to wait for the socket to become writable.
     * @return True if you can write data to the socket, false otherwise
     * @author Peter Jones
    **/
    //####################################################################
    bool writable (const Timeout &timeout);

    //####################################################################
    /** 
     * Check to see if a socket can be read from or written to. If you give
     * a timeout value then this method will not block longer then the
     * timeout.
     *
     * @param timeout How long to wait for the socket to become writable.
     * @return True if you can read from or write to the socket.
     * @reutrn False if there is a timeout.
     * @author Peter Jones
    **/
    //####################################################################
    bool readable_or_writable (const Timeout &timeout);

    //####################################################################
    /** 
     * Close the socket.
     *
     * @author Peter Jones
    **/
    //####################################################################
    void close (void);

    //####################################################################
    /** 
     * Release the socket fd so that it won't be closed by the close
     * member function or the dtor.
     *
     * @author Peter Jones
    **/
    //####################################################################
    void release (void);

    //####################################################################
    /** 
     * Get the socket type. This value is only valid when a socket is
     * constructed with a type or with another socket that was constructed
     * with a type.
     *
     * @return The socket type.
     * @author Peter Jones
    **/
    //####################################################################
    Type get_type (void) const;

    //####################################################################
    /** 
     * Get the file descriptor for this socket.
     *
     * @return The fd for this socket
     * @author Peter Jones
    **/
    //####################################################################
    socket_type get_socketfd (void) const;

    //####################################################################
    /** 
     * Set the socket file descriptor. This will close the current
     * descriptor if needed and reset this Socket to control the new socket
     * file descriptor. That means that the Socket destructor will close the
     * given file descriptor unless you call the release function.
     *
     * @param socketfd The socket file descriptor to take over
     * @author Peter Jones
    **/
    //####################################################################
    void set_socketfd (socket_type socketfd);

    //####################################################################
    /** 
     * Find out if the socket is open and ready
     *
     * @return True if socket is NOT ready
     * @author Peter Jones
    **/
    //####################################################################
    bool operator! (void) const;

    bool operator< (const Socket &other) const;
    bool operator< (socket_type socketfd) const;
private:
    socket_type socketfd_;
    bool probe_ready_;
    bool type_ready_;
    Probe_impl probe_;
    Type type_;
}; // end Netxx::Socket

bool operator== (const Socket &first, const Socket &second);
bool operator!= (const Socket &first, const Socket &second);

} // end Netxx namespace
#endif
