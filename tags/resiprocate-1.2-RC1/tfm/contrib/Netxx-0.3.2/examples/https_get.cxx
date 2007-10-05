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

/*
 * This example shows you how to make a TLS request and examin the the
 * server's certificate.
 */

// Netxx Includes
#include <Netxx/Address.h>
#include <Netxx/Timeout.h>
#include <Netxx/TLS/Stream.h>
#include <Netxx/TLS/Certificate.h>

// Standard Includes
#include <iostream>
#include <exception>

int main (int argc, char *argv[]) {
    if (argc != 2) {
	std::cout << "Usage: " << argv[0] << " URI\n";
	return 0;
    }

    try {
	Netxx::TLS::Context context;
	Netxx::Address addr(argv[1], 443);
	Netxx::TLS::Stream client(context, addr, Netxx::Timeout(10));

	const Netxx::TLS::Certificate &peer_cert = client.get_peer_cert();
	const Netxx::TLS::Certificate &issuer_cert = client.get_issuer_cert();

	if (peer_cert) {
	    std::cout << "Peer Certificate:\n";
	    std::cout << "     C: " << peer_cert.get_country() << "\n";
	    std::cout << "    ST: " << peer_cert.get_region() << "\n";
	    std::cout << "     O: " << peer_cert.get_organization() << "\n";
	    std::cout << "    OU: " << peer_cert.get_organization_unit() << "\n";
	    std::cout << "    CN: " << peer_cert.get_fqdn() << "\n\n";
	} else {
	    std::cout << "Missing Peer Certificate\n\n";
	}

	if (issuer_cert) {
	    std::cout << "Issuer Certificate:\n";
	    std::cout << "     C: " << issuer_cert.get_country() << "\n";
	    std::cout << "    ST: " << issuer_cert.get_region() << "\n";
	    std::cout << "     O: " << issuer_cert.get_organization() << "\n";
	    std::cout << "    OU: " << issuer_cert.get_organization_unit() << "\n";
	    std::cout << "    CN: " << issuer_cert.get_fqdn() << "\n\n";
	} else {
	    std::cout << "Missing Issuer Certificate\n\n";
	}

	std::string request("GET ");
	if (addr.get_path()) {
	    request += addr.get_path();
	} else {
	    request += "/";
	}

	request += " HTTP/1.0\r\n";
	request += "Host: ";
	request += addr.get_name();
	request += "\r\n";
	request += "Connection: close\r\n\r\n";

	client.write(request.c_str(), request.size());

	char buffer[1024];
	Netxx::signed_size_type length;

	while ( (length = client.read(buffer, sizeof(buffer))) > 0)
	    std::cout.write(buffer, length);

    } catch (std::exception &e) {
	std::cerr << argv[0] << ": " << e.what() << std::endl;
	return 1;
    }

    return 0;
}
