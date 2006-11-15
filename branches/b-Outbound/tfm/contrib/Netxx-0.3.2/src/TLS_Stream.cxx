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
 * This file contains the implementation of the Netxx::TLS::Stream class.
**/

// Netxx includes
#include "Netxx/TLS/Stream.h"
#include "TLS_tls_pimpl.h"
#include "Netxx/TLS/Context.h"
#include "Socket.h"
#include "Netxx/SockOpt.h"

// OpenSSL includes
#include <openssl/ssl.h>
#include <openssl/x509.h>

// standard includes
#include <memory>

//####################################################################
namespace {
    void handle_shutdown (Netxx::TLS::tls_pimpl *pimpl, bool  dont_throw_exceptions);
    void handle_connect  (Netxx::TLS::tls_pimpl *pimpl, const Netxx::Timeout &timeout);
    void handle_accept   (Netxx::TLS::tls_pimpl *pimpl, const Netxx::Timeout &timeout);
}
//####################################################################
Netxx::TLS::Stream::Stream (Context &context, const Address &address, const Timeout &timeout)
    : StreamBase(timeout)
{
    std::auto_ptr<tls_pimpl> auto_pimpl(pimpl_ = new tls_pimpl(context.pimpl_));
    make_connection(pimpl_->socket_, address);

    pimpl_->init();
    handle_connect(pimpl_, get_timeout());

    auto_pimpl.release();
}
//####################################################################
Netxx::TLS::Stream::Stream (Context &context, const char *addr, port_type default_port, const Timeout &timeout)
    : StreamBase(timeout)
{
    Address address(addr, default_port);

    std::auto_ptr<tls_pimpl> auto_pimpl(pimpl_ = new tls_pimpl(context.pimpl_));
    make_connection(pimpl_->socket_, address);

    pimpl_->init();
    handle_connect(pimpl_, get_timeout());

    auto_pimpl.release();
}
//####################################################################
Netxx::TLS::Stream::Stream (Context &context, int socketfd, Mode mode, const Timeout &timeout) 
    : StreamBase(timeout)
{
    std::auto_ptr<tls_pimpl> auto_pimpl(pimpl_ = new tls_pimpl(context.pimpl_, socketfd));
    pimpl_->init();

    switch (mode) {
	case mode_client:
	    handle_connect(pimpl_, get_timeout());
	    break;

	case mode_server:
	    pimpl_->server_ = true;
	    handle_accept(pimpl_, get_timeout());
	    break;
    }

    auto_pimpl.release();
}
//####################################################################
Netxx::TLS::Stream::~Stream (void) {
    handle_shutdown(pimpl_, true);
    delete pimpl_;
}
//####################################################################
void Netxx::TLS::Stream::close (void) {
    handle_shutdown(pimpl_, false);
}
//####################################################################
int Netxx::TLS::Stream::get_socketfd (void) const {
    return pimpl_->socket_.get_socketfd();
}
//####################################################################
Netxx::signed_size_type Netxx::TLS::Stream::write (const void *buffer, size_type length) {
    if (!length) return 0;

    // older versions of OpenSSL used char* instead of void*
    const char *cbuffer = static_cast<const char*>(buffer);
    signed_size_type rc;

    if (get_timeout() && !pimpl_->socket_.writable(get_timeout())) return -1;

    for (;;) {
	rc = SSL_write(pimpl_->openssl_ssl_, cbuffer, length);
	if (rc >= 0) return rc;

	switch (SSL_get_error(pimpl_->openssl_ssl_, rc)) {
	    case SSL_ERROR_ZERO_RETURN:
		close();
		return 0;

	    case SSL_ERROR_WANT_WRITE:
		if (!pimpl_->socket_.writable(get_timeout())) return -1;
		break;

	    case SSL_ERROR_WANT_READ:
		if (!pimpl_->socket_.readable(get_timeout())) return -1;
		break;

	    default:
		pimpl_->error2exception("failed to write");
		break;
	}
    }
}
//####################################################################
Netxx::signed_size_type Netxx::TLS::Stream::read (void *buffer, size_type length) {

    // older versions of OpenSSL used char* instead of void*
    char *cbuffer = static_cast<char*>(buffer);
    signed_size_type rc;

    if (get_timeout() && !SSL_pending(pimpl_->openssl_ssl_) && !pimpl_->socket_.readable(get_timeout())) return -1;

    for (;;) {
	rc = SSL_read(pimpl_->openssl_ssl_, cbuffer, length);
	if (rc >= 0) return rc;

	switch (SSL_get_error(pimpl_->openssl_ssl_, rc)) {
	    case SSL_ERROR_ZERO_RETURN:
		close();
		return 0;

	    case SSL_ERROR_WANT_WRITE:
		if (!pimpl_->socket_.writable(get_timeout())) return -1;
		break;

	    case SSL_ERROR_WANT_READ:
		if (!pimpl_->socket_.readable(get_timeout())) return -1;
		break;

	    default:
		pimpl_->error2exception("failed to read");
		break;
	}
    }
}
//####################################################################
const Netxx::TLS::Certificate& Netxx::TLS::Stream::get_peer_cert (void) const {
    X509 *cert;
    X509_NAME *subject;

    if ( (cert = SSL_get_peer_certificate(pimpl_->openssl_ssl_)) == 0 || ( subject = X509_get_subject_name(cert)) == 0) {
	cert = 0; subject = 0;
    }

    pimpl_->peer_cert_.set(cert, subject);
    return pimpl_->peer_cert_;
}
//####################################################################
const Netxx::TLS::Certificate& Netxx::TLS::Stream::get_issuer_cert (void) const {
    X509 *cert;
    X509_NAME *issuer;

    if ( (cert = SSL_get_peer_certificate(pimpl_->openssl_ssl_)) == 0 || (issuer = X509_get_issuer_name(cert)) == 0) {
	cert = 0; issuer = 0;
    }

    pimpl_->issuer_cert_.set(cert, issuer);
    return pimpl_->issuer_cert_;
}
//####################################################################
const Netxx::ProbeInfo* Netxx::TLS::Stream::get_probe_info (void) const {
    return &(pimpl_->pi_);
}
//####################################################################
namespace {

    //####################################################################
    void handle_shutdown (Netxx::TLS::tls_pimpl *pimpl, bool dont_throw_exceptions) {
	if (!pimpl->socket_) return;

	/*
	 * FIXME
	 *
	 * this code does not account for a non-blocking socket
	 *
	 * FIXME
	 */
	if (!pimpl->server_ || (SSL_get_shutdown(pimpl->openssl_ssl_) & SSL_RECEIVED_SHUTDOWN)) {
	    if (SSL_shutdown(pimpl->openssl_ssl_) != 1 && !dont_throw_exceptions) {
		pimpl->error2exception("failed TLS shutdown");
	    }
	} else {
	    SSL_clear(pimpl->openssl_ssl_);
	    pimpl->socket_.close();

	    if (!dont_throw_exceptions) {
		throw Netxx::Exception("TLS connection shutdown failure");
	    }
	}

	pimpl->socket_.close();
    }
    //####################################################################
    void handle_connect (Netxx::TLS::tls_pimpl *pimpl, const Netxx::Timeout &timeout) {
	Netxx::SockOpt sockop(pimpl->socket_.get_socketfd());
	if (timeout) sockop.set_non_blocking();
	int rc;

	SSL_set_connect_state(pimpl->openssl_ssl_);

	for (;;) {
	    rc = SSL_connect(pimpl->openssl_ssl_);
	    if (rc > 0) return;

	    switch (SSL_get_error(pimpl->openssl_ssl_, rc)) {
		case SSL_ERROR_WANT_READ:
		    if (!pimpl->socket_.readable(timeout)) {
			throw Netxx::Exception("timeout during TLS connection handshake");
		    }
		    continue;

		case SSL_ERROR_WANT_WRITE:
		    if (!pimpl->socket_.writable(timeout)) {
			throw Netxx::Exception("timeout during TLS connection handshake");
		    }
		    continue;

		default:
		    pimpl->error2exception("failed TLS connection handshake");
		    break;
	    }
	}
    }
    //####################################################################
    void handle_accept (Netxx::TLS::tls_pimpl *pimpl, const Netxx::Timeout &timeout) {
	Netxx::SockOpt sockop(pimpl->socket_.get_socketfd());
	if (timeout) sockop.set_non_blocking();
	int rc;

	SSL_set_accept_state(pimpl->openssl_ssl_);

	for (;;) {
	    rc = SSL_accept(pimpl->openssl_ssl_);
	    if (rc == 1) return;

	    switch (SSL_get_error(pimpl->openssl_ssl_, rc)) {
		case SSL_ERROR_WANT_READ:
		    if (!pimpl->socket_.readable(timeout)) {
			throw Netxx::Exception("timeout during TLS accept handshake");
		    }
		    continue;

		case SSL_ERROR_WANT_WRITE:
		    if (!pimpl->socket_.writable(timeout)) {
			throw Netxx::Exception("timeout during TLS accept handshake");
		    }
		    continue;

		default:
		    pimpl->error2exception("failed TLS accept handshake");
		    break;
	    }
	}
    }
    //####################################################################

} // end anonymous namespace
