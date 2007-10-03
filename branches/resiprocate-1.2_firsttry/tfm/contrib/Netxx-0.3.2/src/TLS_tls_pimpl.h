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
 * This file defines the TLS::tls_pimpl struct.
**/

#ifndef _Netxx_TLS_tls_pimpl_h_
#define _Netxx_TLS_tls_pimpl_h_

// Netxx includes
#include "TLS_Context_pimpl.h"
#include "Netxx/ProbeInfo.h"
#include "Socket.h"
#include "Netxx/TLS/Certificate.h"
#include "TLS_cert_pimpl.h"

// OpenSSL includes
#include <openssl/ssl.h>

namespace Netxx { namespace TLS {
    struct tls_pimpl;
    
    class TLS_ProbeInfo : public Netxx::ProbeInfo {
    public:
	TLS_ProbeInfo (tls_pimpl *p) : pimpl_(p) {}

	bool needs_pending_check (void) const
	{ return true; }

	pending_type check_pending (socket_type, pending_type pt) const;
    private:
	mutable tls_pimpl *pimpl_;
    };

    struct tls_pimpl {
	tls_pimpl (context_pimpl *ctx_pimpl, int socketfd);
	tls_pimpl (context_pimpl *ctx_pimpl);
	~tls_pimpl (void);

	void init (void);
	void error2exception (const char *prefix=0);

	bool server_;
	SSL *openssl_ssl_;
	context_pimpl *ctx_pimpl_;
	Socket socket_;
	TLS_ProbeInfo pi_;
	cert_impl peer_cert_;
	cert_impl issuer_cert_;
    };

}} // end Netxx and TLS namespaces
#endif
