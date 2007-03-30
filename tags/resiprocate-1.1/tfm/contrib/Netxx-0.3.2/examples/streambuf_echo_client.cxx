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
 * This example shows you how you can use the Netxx::Netbuf template class
 * as a IOStreams streambuf. Doing so allows you to treat a TCP connection
 * as an IOStream.
 */

// Netxx Includes
#include <Netxx/Netbuf.h>
#include <Netxx/Stream.h>
#include <Netxx/Timeout.h>

// Standard Includes
#include <string>
#include <iostream>
#include <exception>

int main (int argc, char *argv[]) {
    if (argc != 2) {
	std::cout << "Usage: " << argv[0] << " <address>\n";
	return 1;
    }

    try {
	Netxx::Stream client(argv[1], 7, Netxx::Timeout(2));
	Netxx::Netbuf<1024> sbuf(client);
	std::iostream stream(&sbuf);
	std::string string_buffer;

	while (std::getline(std::cin, string_buffer)) {
	    stream << string_buffer << std::endl;

	    if (std::getline(stream, string_buffer)) {
		std::cout << string_buffer << std::endl;
	    } else {
		std::cerr << argv[0] << " error reading from server";
		std::cerr << std::endl;
		return 1;
	    }
	}
    } catch (std::exception &e) {
	std::cerr << e.what() << std::endl;
	return 1;
    }

    return 0;
}
