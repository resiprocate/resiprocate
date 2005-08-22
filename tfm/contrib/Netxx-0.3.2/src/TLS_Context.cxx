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
 * This file contains the implementation of the Netxx::TLS::Context class.
**/

// Netxx includes
#include "TLS_Context_pimpl.h"

// OpenSSL includes
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

// standard includes
#include <iostream>
#include <cstring>
#include <string>

//####################################################################

extern "C" int openssl_password_callback (char *buffer, int size, int encrypting, void *pimpl);

//####################################################################
Netxx::TLS::Context::Context (bool allow_init) {
    pimpl_ = new context_pimpl(this);

    if (allow_init) {
	SSL_library_init();
	SSL_load_error_strings();
    }

    if ( (pimpl_->openssl_ctx_ = SSL_CTX_new(SSLv23_method())) == 0) {
	throw Exception("can't create OpenSSL context object");
    }

    // Seed the pseudorandom number generator
    std::string seed; seed_prng(seed);
    if (!seed.empty()) RAND_seed(seed.c_str(), seed.size());

    // Setup the PEM password callback
    SSL_CTX_set_default_passwd_cb(pimpl_->openssl_ctx_, openssl_password_callback);
    SSL_CTX_set_default_passwd_cb_userdata(pimpl_->openssl_ctx_, pimpl_);
    SSL_CTX_set_options(pimpl_->openssl_ctx_, SSL_OP_ALL);

    // Work around a bug in older versions of OpenSSL
#   if (OPENSSL_VERSION_NUMBER < 0x00905100L)
	SSL_CTX_set_verify_depth(pimpl_->openssl_ctx_, 1);
#   endif
}
//####################################################################
Netxx::TLS::Context::~Context (void) {
    SSL_CTX_free(pimpl_->openssl_ctx_);
    delete pimpl_;
}
//####################################################################
void Netxx::TLS::Context::load_cert_chain (const char *filename) {
    if (SSL_CTX_use_certificate_chain_file(pimpl_->openssl_ctx_, filename) != 1) {
	std::string error("error loading certificate chain file: "); error += filename;
	throw Exception(error);
    }
}
//####################################################################
void Netxx::TLS::Context::load_private_key (const char *filename) {
    if (SSL_CTX_use_PrivateKey_file(pimpl_->openssl_ctx_, filename, SSL_FILETYPE_PEM) != 1) {
	std::string error("error loading private key file: "); error += filename;
	throw Exception(error);
    }
}
//####################################################################
void Netxx::TLS::Context::load_ca_file (const char *filename) {
    if (SSL_CTX_load_verify_locations(pimpl_->openssl_ctx_, filename, 0) != 1) {
	std::string error("error loading CA certificate file: "); error += filename;
	throw Exception(error);
    }
}
//####################################################################
void Netxx::TLS::Context::remove_protocol (Protocol which_protocol) {
    switch (which_protocol) {
	case PROTOCOL_SSLv2:
	    SSL_CTX_set_options(pimpl_->openssl_ctx_, SSL_OP_NO_SSLv2);
	    break;

	case PROTOCOL_SSLv3:
	    SSL_CTX_set_options(pimpl_->openssl_ctx_, SSL_OP_NO_SSLv3);
	    break;

	case PROTOCOL_TLSv1:
	    SSL_CTX_set_options(pimpl_->openssl_ctx_, SSL_OP_NO_TLSv1);
	    break;
    }
}
//####################################################################
bool Netxx::TLS::Context::get_password (std::string &password, bool encrypting) {
#ifdef WIN32
    /* can't use console for win32 */
    return false;
#else
    const char *p = EVP_get_pw_prompt();
    if (!p) p = "Enter passphrase: ";

    char buffer[PEM_BUFSIZE];

    for (;;) {
	if (EVP_read_pw_string(buffer, PEM_BUFSIZE, p, encrypting ? 1 : 0) != 0) return false;
	if (encrypting && std::strlen(buffer) < 4) std::cerr << "passphrase too short!\n";
	else break;
    }

    password = buffer;
    return true;
#endif
}
//####################################################################
void Netxx::TLS::Context::seed_prng (std::string&) {
    // FIXME Should we do anything here?
}
//####################################################################
int Netxx::TLS::context_pimpl::proxy_password_callback (char *buffer, int size, bool encrypting) {
    std::string passwd;

   if (!parent_->get_password(passwd, encrypting)) return -1;
   if (passwd.size() > static_cast<std::string::size_type>(size)) return -1;
    
   std::memcpy(buffer, passwd.c_str(), passwd.size());
   return static_cast<int>(passwd.size());
}
//####################################################################
extern "C" int openssl_password_callback (char *buffer, int size, int encrypting, void *pimpl) {
    return static_cast<Netxx::TLS::context_pimpl*>(pimpl)->proxy_password_callback(buffer, size, encrypting);
}
//####################################################################
