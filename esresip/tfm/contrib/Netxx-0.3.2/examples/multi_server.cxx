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
 * This example shows you how to use the Netxx::Probe class. In this case,
 * we have a group of server classes and we use the Probe to know when we
 * can accept a connection on one of the servers.
 */

// Netxx Includes
#include <Netxx/Timeout.h>
#include <Netxx/Probe.h>
#include <Netxx/StreamServer.h>
#include <Netxx/DatagramServer.h>
#include <Netxx/Stream.h>
#include <Netxx/Datagram.h>

// Standard Includes
#include <iostream>
#include <exception>
#include <ctime>

namespace {
    const Netxx::port_type const_port_echo	= 4000;
    const Netxx::port_type const_port_daytime   = 4001;
    const Netxx::size_type const_buffer_size	= 4096;
}

int main (int argc, char *argv[]) {
    if (argc > 1) {
	std::cerr << argv[0] << ": this program does not take any options\n";
	return 1;
    }

    try {
	Netxx::Timeout		timeout(5);
	Netxx::Probe		probe;

	Netxx::StreamServer	tcp_echo_server(const_port_echo, timeout);
	Netxx::DatagramServer	udp_echo_server(const_port_echo, timeout);
	Netxx::StreamServer	tcp_daytime_server(const_port_daytime, timeout);
	Netxx::DatagramServer	udp_daytime_server(const_port_daytime, timeout);

	std::cout << argv[0] << ":   tcp echo server listening on port ";
	std::cout << const_port_echo << std::endl;

	std::cout << argv[0] << ":   udp echo server listening on port ";
	std::cout << const_port_echo << std::endl;

	std::cout << argv[0] << ": tcp daytime server listening on port ";
	std::cout << const_port_daytime << std::endl;

	std::cout << argv[0] << ": udp daytime server listening on port ";
	std::cout << const_port_daytime << std::endl;


	/*
	 * add servers to the Probe for readable testing since we only care
	 * when the sockets are readable not writable.
	 */
	probe.add(tcp_echo_server, Netxx::Probe::ready_read);
	probe.add(udp_echo_server, Netxx::Probe::ready_read);
	probe.add(tcp_daytime_server, Netxx::Probe::ready_read);
	probe.add(udp_daytime_server, Netxx::Probe::ready_read);

	char buffer[const_buffer_size];
	Netxx::Probe::result_type rt;

	for (;;) {
	    rt = probe.ready(timeout);

	    if (rt.first == -1 || !(rt.second & Netxx::Probe::ready_read)) {
		std::cout << argv[0] << ": timeout waiting for connection" << std::endl;
		continue;
	    }

	    std::cout << argv[0] << ": one of the servers is ready, testing ..." << std::endl;

	    if (rt.first == tcp_echo_server)
	    {
		Netxx::Peer peer = tcp_echo_server.accept_connection();

		if (!peer) { 
		    std::cout << argv[0] << ": timeout in accept" << std::endl;
		    continue;
		}

		std::cout << argv[0] << ": incoming TCP-ECHO connection from: " << peer << std::endl;

		Netxx::Stream client(peer.get_socketfd(), timeout);
		Netxx::signed_size_type bytes_read;

		while ( (bytes_read = client.read(buffer, sizeof(buffer))) > 0)
		    client.write(buffer, bytes_read);

		std::cout << argv[0] << ": connection with " << peer << " closed" << std::endl;
	    }
	    else if (rt.first == udp_echo_server)
	    {
		Netxx::DatagramServer::receive_type info =
		    udp_echo_server.receive(buffer, sizeof(buffer));

		if (!info.second) {
		    std::cout << argv[0] << ": timeout waiting for datagram";
		    std::cout << std::endl;
		    continue;
		}

		std::cout << argv[0] << ": incoming UDP-ECHO datagram from ";
		std::cout << info.second << std::endl;

		udp_echo_server.send(info.second, buffer, info.first);
		std::cout << argv[0] << ": sent datagram to " << info.second << std::endl;
	    }
	    else if (rt.first == tcp_daytime_server)
	    {
		Netxx::Peer peer = tcp_daytime_server.accept_connection();

		if (!peer) { 
		    std::cout << argv[0] << ": timeout in accept" << std::endl;
		    continue;
		}

		std::cout << argv[0] << ": incoming TCP-DAYTIME connection from: " << peer << std::endl;

		Netxx::Stream client(peer.get_socketfd(), timeout);
		std::time_t now = std::time(0);
		char *time_str = std::ctime(&now);
		client.write(time_str, std::strlen(time_str));

		std::cout << argv[0] << ": connection with " << peer << " closed" << std::endl;
	    }
	    else if (rt.first == udp_daytime_server)
	    {
		Netxx::DatagramServer::receive_type info =
		    udp_daytime_server.receive(buffer, sizeof(buffer));

		if (!info.second) {
		    std::cout << argv[0] << ": timeout waiting for datagram";
		    std::cout << std::endl;
		    continue;
		}

		std::cout << argv[0] << ": incoming UDP-DAYTIME datagram from ";
		std::cout << info.second << std::endl;

		std::time_t now = std::time(0);
		char *time_str = std::ctime(&now);
		udp_daytime_server.send(info.second, time_str, std::strlen(time_str));
		std::cout << argv[0] << ": sent datagram to " << info.second << std::endl;
	    }
	}
    } catch (std::exception &e) {
	std::cerr << argv[0] << ": " << e.what() << std::endl;
	return 1;
    }

    return 0;
}
