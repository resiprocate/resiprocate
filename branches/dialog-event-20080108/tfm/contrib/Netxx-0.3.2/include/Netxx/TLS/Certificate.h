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
 * This file contains the definition of the TLS::Certificate class.
**/

#ifndef _Netxx_TLS_Certificate_h_
#define _Netxx_TLS_Certificate_h_

// standard includes
#include <string>

namespace Netxx { namespace TLS {

/**
 * The Netxx::TLS::Certificate class allows you to access information about
 * an X509 certificate. You should use this so that you can verify that the
 * peer you are communicating with is the one you think it is.
 */
class Certificate {
public:

    //####################################################################
    /** 
     * Create a new Netxx::TLS::Certificate. This is pretty useless for now
     * because this is a pure-virtual class.
     *
     * @author Alex Mitrofanov
    **/
    //####################################################################
    Certificate (void);  

    //####################################################################
    /** 
     * Cleanup a Netxx::TLS::Certificate object
     *
     * @author Alex Mitrofanov
    **/
    //####################################################################
    virtual ~Certificate (void);

    //####################################################################
    /** 
     * Check to see if this certificate object is valid.
     *
     * @return True (1) if the certificate is valid.
     * @return False (0/NULL) if the certificate is NOT valid.
     * @author Peter Jones
    **/
    //####################################################################
    virtual operator void* (void) const = 0;

    //####################################################################
    /** 
     * Another way to check if this certificate object is valid.
     *
     * @return True if the certificate is NOT valid.
     * @return False if the certificate is valid.
     * @author Peter Jones
    **/
    //####################################################################
    virtual bool operator! (void) const = 0;

    //####################################################################
    /** 
     * Get the fully qualified domain name, or the common name from the
     * certificate. The FQDN is checked first, if there is no FQND,
     * commonName is returned.
     *
     * @return The FQDN or commonName
     * @return A blank string to signal error.
     * @author Alex Mitrofanov
    **/
    //####################################################################
    virtual std::string get_fqdn (void) const = 0;

    //####################################################################
    /** 
     * Get the country name from the certificate.
     *
     * @return The country name.
     * @return A blank string to signal error.
     * @author Alex Mitrofanov
    **/
    //####################################################################
    virtual std::string get_country (void) const = 0;

    //####################################################################
    /** 
     * Get the locality name (e.g, the state name) from the certifcate.
     *
     * @return The locality name.
     * @return A blank string to signal error.
     * @author Alex Mitrofanov
    **/
    //####################################################################
    virtual std::string get_locality (void) const = 0;

    //####################################################################
    /** 
     * Get the region name (e.g., city name) from the certificate.
     *
     * @return The region name.
     * @return A blank string to signal error.
     * @author Alex Mitrofanov
    **/
    //####################################################################
    virtual std::string get_region (void) const = 0;

    //####################################################################
    /** 
     * Get the organization name from the certificate.
     *
     * @return The organization name.
     * @return A blank string to signal error.
     * @author Alex Mitrofanov
    **/
    //####################################################################
    virtual std::string get_organization (void) const = 0;

    //####################################################################
    /** 
     * Get the organizational unit name from the certificate.
     *
     * @return The organizational unit name
     * @return A blank string to signal error.
     * @author Alex Mitrofanov
    **/
    //####################################################################
    virtual std::string get_organization_unit (void) const = 0;

private:
    Certificate (const Certificate&);
    Certificate& operator= (const Certificate&);
}; // end Netxx::TLS::Certificate class

}} // namespace Netxx & namespace TLS

#endif
