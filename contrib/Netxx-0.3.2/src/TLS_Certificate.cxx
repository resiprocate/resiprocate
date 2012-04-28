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

// Netxx Includes
#include "TLS_cert_pimpl.h"

// OpenSSL includes
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

// standard includes
#include <string>

//####################################################################
namespace {
    const char const_subject_alt_name[]	    = "subjectAltName";
    const char const_field_dns[]	    = "DNS";
} // end anonymous namespace
//####################################################################
Netxx::TLS::Certificate::Certificate (void) {
}
//####################################################################
Netxx::TLS::Certificate::~Certificate (void) {
}
//####################################################################
Netxx::TLS::cert_impl::cert_impl() : x509_(0), x509_name_(0) {
}
//####################################################################
Netxx::TLS::cert_impl::~cert_impl (void) {
}
//####################################################################
Netxx::TLS::cert_impl::operator void* (void) const {
    return x509_name_;
}
//####################################################################
bool Netxx::TLS::cert_impl::operator! (void) const {
    return !x509_name_;
}
//####################################################################
std::string Netxx::TLS::cert_impl::get_fqdn (void) const {
    int extcount;

    if (x509_ && ( extcount = X509_get_ext_count(x509_)) > 0) {
	X509_EXTENSION *ext;
	const char *extstr;

	for (int i=0; i<extcount; ++i) {
	    ext = X509_get_ext(x509_, i);
	    extstr = OBJ_nid2sn(OBJ_obj2nid(X509_EXTENSION_get_object(ext)));

	    if (std::strcmp(extstr, const_subject_alt_name) == 0) {
		X509V3_EXT_METHOD *meth;
		STACK_OF(CONF_VALUE) *val;
		CONF_VALUE *nval;

		if ( (meth = X509V3_EXT_get(ext)) == 0) break;
		val = meth->i2v(meth, meth->d2i(0, &(ext->value->data), ext->value->length), 0);

		for (int j=0; j<sk_CONF_VALUE_num(val); ++j) {
		    nval = sk_CONF_VALUE_value(val, j);
		    if (std::strcmp(nval->name, const_field_dns) == 0) return std::string(nval->value);
		}
	    }
	}
    }

    return get_text(NID_commonName);
}
//####################################################################
std::string Netxx::TLS::cert_impl::get_country (void) const {
    return get_text(NID_countryName);
}
//####################################################################
std::string Netxx::TLS::cert_impl::get_locality (void) const {
    return get_text(NID_localityName);
}
//####################################################################
std::string Netxx::TLS::cert_impl::get_region (void) const {
    return get_text(NID_stateOrProvinceName);
}
//####################################################################
std::string Netxx::TLS::cert_impl::get_organization (void) const {
    return get_text(NID_organizationName);
}
//####################################################################
std::string Netxx::TLS::cert_impl::get_organization_unit (void) const {
    return get_text(NID_organizationalUnitName);
}
//####################################################################
std::string Netxx::TLS::cert_impl::get_text (int nid) const {
    std::string rc;
    char name[128];

    if (x509_name_ && X509_NAME_get_text_by_NID(x509_name_, nid, name, sizeof(name)) > 0) {
	rc = name;
    }

    return rc;
}
//####################################################################
void Netxx::TLS::cert_impl::set(X509 *x, X509_NAME *xn) {
    x509_ = x;
    x509_name_ = xn;
}
//####################################################################
