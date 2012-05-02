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

#ifndef _Netxx_TLS_cert_pimpl_h_
#define _Netxx_TLS_cert_pimpl_h_

// Netxx Includes
#include <Netxx/TLS/Certificate.h>

// OpenSSL Includes
#include <openssl/ssl.h>

namespace Netxx { namespace TLS {

class cert_impl : public Certificate {
public:
    cert_impl (void);  

    ~cert_impl (void);  

    operator void* (void) const;

    bool operator! (void) const;

    std::string get_fqdn (void) const;

    std::string get_country (void) const;
    
    std::string get_locality (void) const;
    
    std::string get_region (void) const;

    std::string get_organization (void) const;

    std::string get_organization_unit (void) const;

    void set (X509 *x, X509_NAME *xn);
private:
    std::string get_text (int nid) const;

    cert_impl (const cert_impl&);
    cert_impl& operator= (const cert_impl&);

    X509 *x509_;
    X509_NAME *x509_name_;
}; // end Netxx::TLS::cert_impl class

}} // namespace Netxx & namespace TLS
#endif
