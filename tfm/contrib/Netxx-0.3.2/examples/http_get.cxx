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
 * This is a simple example of how you can use the Netxx::Address class and
 * the Netxx::Stream class to do make a HTTP GET request to a web server.
 */

// Netxx Includes
#include <Netxx/Address.h>
#include <Netxx/Stream.h>
#include <Netxx/Timeout.h>
#include <Netxx/Types.h>

// Standard Includes
#include <iostream>
#include <exception>

int main (int argc, char *argv[]) {
    if (argc != 2 && argc != 3) {
	std::cout << "Usage: " << argv[0] << " URI [Host: header]\n";
	return 0;
    }

    try {
	Netxx::Address addr(argv[1], 80);
	Netxx::Stream client(addr, Netxx::Timeout(10));

	std::string request("GET ");

	if (addr.get_path()) {
	    request += addr.get_path();
	} else {
	    request += "/";
	}

	// setup the request line and headers
	request += " HTTP/1.0";
	request += 13; request += 10;

	// send the Host header
	request += "Host: ";
	if (argc == 3) request += argv[2];
	else request += addr.get_name();
	request += 13; request += 10;

	request += "Connection: close";
	request += 13; request += 10;
	request += 13; request += 10;

	// send the request
	client.write(request.c_str(), request.size());

	char buffer[1024];
	Netxx::signed_size_type length;

	// read back the result
	while ( (length = client.read(buffer, sizeof(buffer))) > 0)
	    std::cout.write(buffer, length);

    } catch (std::exception &e) {
	std::cerr << e.what() << std::endl;
	return 1;
    }

    return 0;
}
