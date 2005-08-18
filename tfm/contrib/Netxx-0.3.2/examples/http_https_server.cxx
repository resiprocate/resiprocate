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
 * This is a very simple web server using a lot of different classes from
 * Netxx. It will accept incoming HTTP and HTTPS requests. This example is a
 * little big, but is does do some basic HTTP/1.0 web server stuff!
 *
 * Remember, this is just an example! This web server does not have any
 * security checking. In fact, using some ../../ directory references,
 * someone could get your password file!
 */

// Netxx includes
#include <Netxx/Address.h>
#include <Netxx/Peer.h>
#include <Netxx/Stream.h>
#include <Netxx/StreamServer.h>
#include <Netxx/Timeout.h>
#include <Netxx/Types.h>
#include <Netxx/TLS/Stream.h>
#include <Netxx/Netbuf.h>

// standard includes
#include <iostream>
#include <fstream>
#include <exception>
#include <memory>

int main (int argc, char *argv[]) {
    std::string document_root;

    if (argc < 2) document_root = "../src";
    else document_root = argv[1];

    if (document_root[document_root.size()-1] != '/')
	document_root += '/';

    try {
	Netxx::TLS::Context context;
	context.load_private_key("example.pem");
	context.load_cert_chain("example.pem");

	// bind to all addresses for the following ports
	Netxx::Address address;
	address.add_all_addresses(8080);
	address.add_all_addresses(8443);

	Netxx::Timeout accept_timeout(600);
	Netxx::Timeout read_timeout(60);

	Netxx::StreamServer server(address, accept_timeout);

	std::cout << "waiting for HTTP  requests on port 8080\n";
	std::cout << "waiting for HTTPS requests on port 8443\n";

	for (;;) {
	    Netxx::Peer client = server.accept_connection();

	    if (!client) {
		std::cout << argv[0] << ": timeout waiting for connection";
		std::cout << std::endl;
		continue;
	    }

	    std::cout << argv[0] << ": connection from " << client;
	    std::cout << std::endl;

	    std::auto_ptr<Netxx::StreamBase> client_stream;

	    if (client.get_local_port() == 8080) {

		client_stream.reset(new Netxx::Stream(
			    client.get_socketfd(), read_timeout));
	    } else {

		client_stream.reset(new Netxx::TLS::Stream(
			    context, client.get_socketfd(),
			    Netxx::TLS::Stream::mode_server, read_timeout));
	    }

	    Netxx::Netbuf<1024> sbuf(*client_stream);
	    std::iostream stream(&sbuf);

	    std::string token;
	    stream >> token;

	    if (token != "GET") continue;

	    stream >> token;
	    std::cout << argv[0] << ": " << token << std::endl;

	    if (token == "/") {
		std::string message = "<html><body><h1>"
		    "Hello from Netxx</h1></body></html>\n";

		stream << "HTTP/1.0 200 OK\r\n";
		stream << "Content-Type: text/html\r\n";
		stream << "Content-Length: " << message.size() << "\r\n";
		stream << "Connection: closed\r\n\r\n";
		stream << message << std::flush;
		continue;
	    }

	    std::string document = document_root + token;
	    std::ifstream file(document.c_str());

	    if (!file) {
		stream << "HTTP/1.0 404 Not found\r\n";
		stream << "Content-Type: text/html\r\n";
		stream << "Connection: closed\r\n\r\n";
		stream << "<html><body><h1>Not Found</h1></body></html>\n";
		stream << std::flush;
		continue;
	    }

	    stream << "HTTP/1.0 200 OK\r\n";
	    std::string::size_type dotpos = token.rfind(".");

	    if (dotpos != std::string::npos) {
		std::string ext = token.substr(dotpos+1);

		if (ext == "html" || ext == "htm") {
		    stream << "Content-Type: text/html\r\n";
		} else if (ext == "jpeg" || ext == "jpg") {
		    stream << "Content-Type: image/jpeg\r\n";
		} else if (ext == "gif") {
		    stream << "Content-Type: image/gif\r\n";
		} else if (ext == "png") {
		    stream << "Content-Type: image/png\r\n";
		} else {
		    stream << "Content-Type: text/plain\r\n";
		}
	    } else {
		stream << "Content-Type: text/plain\r\n";
	    }

	    stream << "Connection: closed\r\n\r\n";
	    stream << std::flush;

	    char buffer[4096];
	    while (file.read(buffer, sizeof(buffer)) || file.gcount())
		stream.write(buffer, file.gcount());
	}
    } catch (std::exception &e) {
	std::cerr << argv[0] << ": " << e.what() << std::endl;
	return 1;
    }

    return 0;
}
