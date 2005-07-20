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
 * This is another example that shows how easy the Netxx API is. This
 * program will connect to a TCP echo server which will echo everything
 * going into this program's standard input back to it.
 */

// Netxx Includes
#include <Netxx/Stream.h>
#include <Netxx/Types.h>

// Standard Includes
#include <string>
#include <iostream>
#include <exception>

int main (int argc, char *argv[]) {
    if (argc != 2) {
	std::cout << "Usage: " << argv[0] << " <address:port>\n";
	return 1;
    }

    try {
	Netxx::Stream client(argv[1], 7);
	Netxx::signed_size_type byte_count;
	char read_buffer[4096];
	std::string write_buffer;

	while (std::getline(std::cin, write_buffer)) {
	    if (write_buffer.size() > sizeof(read_buffer)) {
		std::cerr << "can't send a line longer than ";
		std::cerr << sizeof(read_buffer) << " bytes\n";
		return 1;
	    }

	    write_buffer += '\n';

	    if ( (byte_count = client.write(write_buffer.c_str(), write_buffer.size())) <= 0)
		throw std::runtime_error("write: peer closed connection");

	    if ( (byte_count = client.read(read_buffer, sizeof(read_buffer))) <= 0)
		throw std::runtime_error("read: peer closed connection");

	    std::cout.write(read_buffer, byte_count);
	}
    } catch (std::exception &e) {
	std::cerr << argv[0] << ": " << e.what() << std::endl;
	return 1;
    }

    return 0;
}
