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
 * This file contains the definition of the Netxx::StreamBase class.
**/

#ifndef _Netxx_StreamBase_h_
#define _Netxx_StreamBase_h_

// Netxx includes
#include <Netxx/Types.h>
#include <Netxx/Address.h>
#include <Netxx/Timeout.h>

namespace Netxx {
    // forward declarations
    class Socket;
    class ProbeInfo;
	
/**
 * The Netxx::StreamBase class acts as a base class for the various stream
 * classes. You cannot directly instianciate an object of this class because
 * it has pure virtual funtions.
**/
class StreamBase {
public:
    //####################################################################
    /** 
     * Create a new StreamBase and set the timeout for stream operations.
     *
     * @param timeout The timeout to use for stream operations.
     * @author Peter Jones
    **/
    //####################################################################
    explicit StreamBase (const Timeout &timeout=Timeout());

    //####################################################################
    /** 
     * StreamBase virutal destructor. Cleanup the streambase.
     *
     * @author Peter Jones
    **/
    //####################################################################
    virtual ~StreamBase (void);

    //####################################################################
    /** 
     * Set the timeout that is used for various stream operations.
     *
     * @param timeout The timeout to use.
     * @author Peter Jones
    **/
    //####################################################################
    void set_timeout (const Timeout &timeout);

    //####################################################################
    /** 
     * Get the current timeout value that is used for stream operations.
     *
     * @return The current timeout value.
     * @author Peter Jones
    **/
    //####################################################################
    const Timeout& get_timeout (void) const;

    //####################################################################
    /** 
     * Read data from the stream into the given buffer. This operation will
     * place the read data into the buffer unless there is an error or
     * timeout. If a stream fatal error occures an exception is throw.
     *
     * @param buffer The buffer to place read data into.
     * @param length The avaliable size for the given buffer.
     * @return Greater than 0: For the bytes read into the buffer.
     * @return 0 If the peer closed the connection
     * @return -1: For a timeout
     * @author Peter Jones
    **/
    //####################################################################
    virtual signed_size_type read (void *buffer, size_type length) = 0;

    //####################################################################
    /** 
     * Write data from a buffer into the stream. If a stream fatal error
     * occures an exception is thrown.
     *
     * @param buffer The data to write to the stream.
     * @param length The amount of data to use from the buffer.
     * @return Greater than 0: The number of bytes written to the stream.
     * @return 0 If the peer closed the connection.
     * @return -1: for a timeout.
     * @author Peter Jones
    **/
    //####################################################################
    virtual signed_size_type write (const void *buffer, size_type length) = 0;

    //####################################################################
    /** 
     * Close the stream connection with the peer. After the connection is
     * closed you should not use any of the other stream member functions.
     *
     * @author Peter Jones
    **/
    //####################################################################
    virtual void close (void) = 0;

    //####################################################################
    /** 
     * Return the file descriptor that is being used for the stream
     * connection.
     *
     * @return The file descriptor for this connection.
     * @author Peter Jones
    **/
    //####################################################################
    virtual socket_type get_socketfd (void) const = 0;

    //####################################################################
    /** 
     * Get information about how this StreamBase should be probed from
     * the Netxx::Probe class.
     *
     * @return A Netxx::ProbeInfo object.
     * @author Peter Jones
    **/
    //####################################################################
    virtual const ProbeInfo* get_probe_info (void) const = 0;
protected:
    //####################################################################
    /** 
     * This protected member function is used to establish a connection with
     * one of the addresses given. This function will throw an exception if
     * it cannot make a connection with any of the given addresses.
     *
     * @param socket On successful return this will be the socket for the conection
     * @param address The list of addresses to try
     * @author Peter Jones
    **/
    //####################################################################
    void make_connection (Socket &socket, const Address &address);

    //####################################################################
    /** 
     * Used by the stream classes so they can implement a swap() member
     * function.
     *
     * @param other The other StreamBase to swap with.
     * @author Peter Jones
    **/
    //####################################################################
    void swap_base (StreamBase &other);
private:
    Timeout timeout_;
}; // end Netxx::StreamBase class

} // end Netxx namespace
#endif
