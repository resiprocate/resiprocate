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
 * Similar to the tcp_echo_server.cxx example, this example shows you how to
 * write a TLS server. It will echo back everything that a TLS echo client
 * sends it.
 */

// Netxx Includes
#include <Netxx/Address.h>
#include <Netxx/Peer.h>
#include <Netxx/Stream.h>
#include <Netxx/StreamServer.h>
#include <Netxx/Timeout.h>
#include <Netxx/Types.h>
#include <Netxx/TLS/Stream.h>

// Standard Includes
#include <iostream>
#include <exception>

int main (int argc, char *argv[]) {
    try {
	Netxx::Address address;
	Netxx::Timeout timeout(60);
	Netxx::TLS::Context  context;

	context.load_private_key("example.pem");
	context.load_cert_chain("example.pem");

	if (argc > 1) {
	    for (int i=1; i<argc; ++i) address.add_address(argv[i], 7000);
	} else {
	    address.add_all_addresses(7000);
	}

	Netxx::StreamServer server(address, timeout);
	char buffer[1024];

	for (;;) {
	    Netxx::Peer client = server.accept_connection();

	    if (!client) {
		std::cout << argv[0] << ": timeout waiting for connection" << std::endl;
		continue;
	    }

	    Netxx::TLS::Stream client_stream(context, client.get_socketfd(),
		    Netxx::TLS::Stream::mode_server, timeout);
	    Netxx::signed_size_type bytes_read;

	    std::cout << argv[0] << ": connection from " << client << std::endl;

	    while ( (bytes_read = client_stream.read(buffer, sizeof(buffer))) > 0)
		client_stream.write(buffer, bytes_read);

	    std::cout << argv[0] << ": client disconnected from server" << std::endl;
	}
    } catch (std::exception &e) {
	std::cerr << argv[0] << ": " << e.what() << std::endl;
	return 1;
    }

    return 0;
}
