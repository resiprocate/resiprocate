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
 * This file contains the definition of the Netxx::TLS::Context class.
**/

#ifndef _Netxx_TLS_Context_h_
#define _Netxx_TLS_Context_h_

// Netxx includes
#include <Netxx/Types.h>

namespace Netxx { namespace TLS {

    // forward declarations
    class Stream;
    class Server;
    struct context_pimpl;

/**
 * The Netxx::TLS::Context class is used to hold common TLS information. It
 * is needed to create TLS stream objects. You can subclass the
 * Netxx::TLS::Context class to customize such things as the password
 * callback.
**/
class Context {
public:
    /**
     * Provides a list of protocols that you can choose to support or not
     * support. It is recomended that you don't support SSLv2.
     */
    enum Protocol {
	PROTOCOL_SSLv2,	///< SSL Version 2
	PROTOCOL_SSLv3, ///< SSL Version 3
	PROTOCOL_TLSv1, ///< TLS Version 1
    };

    //####################################################################
    /** 
     * Construct a new Netxx::TLS::Context object and optionally initilize
     * the backend TLS library. You don't want to initilize the backend TLS
     * library more than once so you can give the constructor false for the
     * allow_init flag.
     *
     * To make the Netxx::TLS::Context thread safe, you should create one
     * Context object and load certificate files before you start any
     * threads.
     *
     * @param allow_init If true, initilize the backend TLS library.
     * @author Peter Jones
    **/
    //####################################################################
    explicit Context (bool allow_init=true);

    //####################################################################
    /** 
     * Netxx::TLS::Context destructor.
     *
     * @author Peter Jones
    **/
    //####################################################################
    virtual ~Context (void);

    //####################################################################
    /** 
     * Load the a certificate chain from the given file. A certificate chain
     * file should contain PEM encoded certificate in chain order. Chain
     * order starts from the application certificate and ends with the root
     * CA certificate.
     *
     * @param filename The name of the file that contains the certificate chain.
     * @author Peter Jones
    **/
    //####################################################################
    void load_cert_chain (const char *filename);

    //####################################################################
    /** 
     * Load a possibly encrypted private key from the given file. If the
     * file is encrypted, the password callback will be called to get the
     * passphrase to decrypt the private key.
     *
     * @param filename The name of the file that contains the private key.
     * @author Peter Jones
    **/
    //####################################################################
    void load_private_key (const char *filename);

    //####################################################################
    /** 
     * Load a PEM encoded file that contains a list of trusted CA
     * certificates.
     *
     * @param filename The file tht contains the CA certificates.
     * @author Peter Jones
    **/
    //####################################################################
    void load_ca_file (const char *filename);

    //####################################################################
    /** 
     * Disable support for the given protocol.
     *
     * @param which_protocol The protocol to remove support for.
     * @author Peter Jones
    **/
    //####################################################################
    void remove_protocol (Protocol which_protocol);

protected:
    //####################################################################
    /** 
     * Override this function so that you can handle password requests in a
     * custom way. For example, you may want to open a GUI dialog and ask
     * for the password. You must use this function on Win32.
     *
     * @param password The string to store the password in.
     * @param encrypting True if the password is needed for encrypting; flase for decrypting
     * @return True for success
     * @return False for failure
     * @author Peter Jones
    **/
    //####################################################################
    virtual bool get_password (std::string &password, bool encrypting);

    //####################################################################
    /** 
     * Override this function to provide a custom way to seed the
     * pseudorandom number generator. On platfors that don't have a
     * /dev/random entropy source. This function will be called when the
     * context is created (and if the allow_init flag was true) and should
     * return some seed data in the passed in std::string.
     *
     * @param seed The place to return some seed data.
     * @author Peter Jones
    **/
    //####################################################################
    virtual void seed_prng (std::string &seed);

private:
    context_pimpl *pimpl_;
    friend struct context_pimpl;
    friend class Stream;
    friend class Server;

    Context (const Context&);
    Context& operator= (const Context&);
}; // end Netxx::TLS::Context class

}} // end Netxx and TLS namespace
#endif
