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
 * This file implements the TLS::tls_pimpl struct.
**/

// Netxx includes
#include "TLS_tls_pimpl.h"
#include "Netxx/Types.h"

// OpenSSL includes
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

// standard includes
#include <cstring>

//####################################################################
namespace {
    const unsigned int const_error_buffer_size = 240;
}
//####################################################################
Netxx::TLS::tls_pimpl::tls_pimpl(context_pimpl *ctx_pimpl)
    : ctx_pimpl_(ctx_pimpl), pi_(this)
{ }
//####################################################################
Netxx::TLS::tls_pimpl::tls_pimpl(context_pimpl *ctx_pimpl, int socketfd)
    : ctx_pimpl_(ctx_pimpl), socket_(socketfd), pi_(this)
{ }
//####################################################################
void Netxx::TLS::tls_pimpl::init (void) {
    server_ = false;

    if ( (openssl_ssl_ = SSL_new(ctx_pimpl_->openssl_ctx_)) == 0) {
	throw Exception("failed to create TLS handle (struct OpenSSL::SSL)");
    }

    BIO *tmp_bio;
    if ( (tmp_bio = BIO_new_socket(socket_.get_socketfd(), BIO_NOCLOSE)) == 0)
	throw Exception("failed to create TLS socket BIO");

    SSL_set_bio(openssl_ssl_, tmp_bio, tmp_bio);

    pi_.add_socket(socket_.get_socketfd());
}
//####################################################################
Netxx::TLS::tls_pimpl::~tls_pimpl (void) {
    SSL_free(openssl_ssl_);
}
//####################################################################
void Netxx::TLS::tls_pimpl::error2exception (const char *prefix) {
    char buffer[const_error_buffer_size];
    std::string error("OpenSSL: ");

    if (prefix) {
	error += prefix;
	error += ": ";
    }

    std::memset(buffer, 0, sizeof(buffer));
    ERR_error_string_n(ERR_get_error(), buffer, sizeof(buffer));
    error += buffer;

    while (ERR_get_error() != 0);
    throw Exception(error);
}
//####################################################################
Netxx::ProbeInfo::pending_type Netxx::TLS::TLS_ProbeInfo::check_pending (socket_type, pending_type pt) const {
    if ((pt == pending_none || pt & pending_read) && SSL_pending(pimpl_->openssl_ssl_)) return pending_read;
    return pending_none;
}
//####################################################################
