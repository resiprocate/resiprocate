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
 * This file contains the definition of the Netxx::TLS::Stream class.
**/

#ifndef _Netxx_TLS_Stream_h_
#define _Netxx_TLS_Stream_h_

// Netxx includes
#include <Netxx/Types.h>
#include <Netxx/StreamBase.h>
#include <Netxx/TLS/Context.h>

namespace Netxx {
    // forward declarations
    class ProbeInfo;
    
namespace TLS {
    // forward declaration
    class Certificate;
    struct tls_pimpl;
	
/**
 * The Netxx::TLS::Stream class is used to send and receive data on a
 * connected TLS socket. It has two constructors, one that connects to the
 * given address and another that uses an already connected socket.
 *
 * When using the constructor that can take a socket descriptor you must
 * specifiy whether the TLS::Stream should operate in server or client mode.
 * This is due to the TLS protocol being a little different for a client
 * than it is for a server.
**/
class Stream : public Netxx::StreamBase {
public:
    /**
     * The connection mode for the Netxx::TLS::Stream
     */
    enum Mode {
	mode_client, ///< This Netxx::TLS::Stream should act as a TLS client.
	mode_server  ///< This Netxx::TLS::Stream shoudl act as a TLS server.
    };

    //####################################################################
    /** 
     * Construct a new Netxx::TLS::Stream object. This constructor will
     * connect to the peer given in the Netxx::Address class.
     *
     * @param context The context to use for this stream.
     * @param address The address to connect to.
     * @param timeout The timeout to use for the connect and stream operations.
     * @author Peter Jones
    **/
    //####################################################################
    explicit Stream (Context &context, const Address &address, const Timeout &timeout=Timeout());

    //####################################################################
    /** 
     * Construct a new Netxx::TLS::Stream object. This constructor will
     * connect to the given address. The given address string is passed to a
     * Netxx::Address class.
     *
     * @param context The contect to use for this stream.
     * @param addr The address to connect to.
     * @param default_port The port to connect to if one is not given in addr.
     * @param timeout The timeout to use for the connect and stream operations.
     * @author Peter Jones
    **/
    //####################################################################
    Stream (Context &context, const char *addr, port_type default_port, const Timeout &timeout=Timeout());

    //####################################################################
    /** 
     * Construct a new Netxx::TLS::Stream object and start TLS on the given
     * socket file descriptor. This object will then own the socket file
     * descriptor and will close it when it is done with it.
     *
     * A mode is needed so that the Netxx::TLS::Stream will know what parts
     * of the TLS protocol to use, client or server. Default is client.
     *
     * @param context The TLS::Context to use for this stream.
     * @param socketfd The socket file descriptor to use.
     * @param mode Is this a TLS server or client?
     * @param timeout The Timeout to use for stream operations.
     * @author Peter Jones
    **/
    //####################################################################
    Stream (Context &context, socket_type socketfd, Mode mode=mode_client, const Timeout &timeout=Timeout());

    //####################################################################
    /** 
     * Netxx::TLS::Stream destructor. Shutdown the TLS connection if
     * necessary and clean things up.
     *
     * @author Peter Jones
    **/
    //####################################################################
    ~Stream (void);

    //####################################################################
    /** 
     * Read data from the TLS connection and place it into the given buffer.
     * If an error occures this function will throw an exception.
     *
     * @param buffer The buffer to store the read data into.
     * @param length The size of the given buffer.
     * @return Greater than 0: The number of bytes stored in the buffer.
     * @return 0: The connected peer closed the connection.
     * @return -1: A timeout occured.
     * @author Peter Jones
    **/
    //####################################################################
    signed_size_type read  (void *buffer, size_type length);

    //####################################################################
    /** 
     * Write data from the given buffer to the TLS conection. If an error
     * occures this function will throw an exception.
     *
     * @param buffer The buffer to write to the connection.
     * @param length The number of bytes to use from the buffer.
     * @return Greater than 0: The number of bytes written to the connection.
     * @return 0: The connected peer closed the connection.
     * @return -1: A timeout occured.
     * @author Peter Jones
    **/
    //####################################################################
    signed_size_type write (const void *buffer, size_type length);

    //####################################################################
    /** 
     * Close the connection. Once you do this you can't call read or write
     * anymore. This is normally done by the destructor.
     *
     * @author Peter Jones
    **/
    //####################################################################
    void close (void);

    //####################################################################
    /** 
     * Get the connected peer's certificate.
     *
     * @return A valid certificate if the peer gave one to you
     * @return An invalid certificate if the peer does not have one
     * @author Alex Mitrofanov
    **/
    //####################################################################
    const Certificate& get_peer_cert (void) const;
    
    //####################################################################
    /** 
     * Get the certificate for the issuer of the connected peer's
     * certificate.
     *
     * @return A valid certificate if the peer gave one and it was signed by an issuer
     * @return An invalid certificate if there was no peer cert or no issuer
     * @author Alex Mitrofanov
    **/
    //####################################################################
    const Certificate& get_issuer_cert (void) const;

    //####################################################################
    /** 
     * Return the current socket file descriptor in use.
     *
     * @author Alex Mitrofanov
    **/
    //####################################################################
    socket_type get_socketfd (void) const;

    //####################################################################
    /** 
     * Get information about how this TLS::Stream should be probed from the
     * Netxx::Probe class.
     *
     * @return A Netxx::ProbeInfo object.
     * @author Peter Jones
    **/
    //####################################################################
    const ProbeInfo* get_probe_info (void) const;

private:
    tls_pimpl *pimpl_;
    Stream (const Stream&);
    Stream& operator= (const Stream&);
}; // end Netxx::TLS::Stream class

}} // end Netxx and TLS namespaces
#endif
