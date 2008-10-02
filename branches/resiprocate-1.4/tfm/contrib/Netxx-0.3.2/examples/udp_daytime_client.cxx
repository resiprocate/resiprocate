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
 * This example shows you how to use the UDP API. It sends a blank datagram
 * to a daytime server. The daytime server should then send back a datagram
 * with the current date and time.
 */

// Netxx Includes
#include <Netxx/Address.h>
#include <Netxx/Datagram.h>
#include <Netxx/Timeout.h>
#include <Netxx/Types.h>

// Standard Includes
#include <iostream>
#include <exception>
#include <string>

int main (int argc, char *argv[]) {
    if (argc != 2) {
	std::cerr << "Usage: " << argv[0] << " address\n";
	return 1;
    }

    try {
	Netxx::Address addr(argv[1], 13);
	Netxx::Datagram datagram(Netxx::Timeout(10));
	char buffer[1024];

	// send blank datagram to make server respond
	datagram.send(*addr.begin(), 0, 0);

	Netxx::Datagram::receive_type response = 
		datagram.receive(buffer, sizeof(buffer));

	if (response.first == -1) {
	    std::cerr << argv[0] << ": timeout waiting for server\n";
	    return 1;
	}

	std::cout.write(buffer, response.first);
    } catch (std::exception &e) {
	std::cerr << argv[0] << ": " << e.what() << "\n";
	return 1;
    }

    return 0;
}
