#include <Netxx/StreamServer.h>
#include <Netxx/Stream.h>
#include <Netxx/Address.h>
#include <Netxx/Timeout.h>

#include <iostream>
#include <stdexcept>
#include <exception>

int main (int argc, char *argv[]) {
    if (argc != 3) {
	std::cerr << "Usage: " << argv[0] << " address size\n";
	return 1;
    }

    try {

	Netxx::Address addr;
	addr.add_address(argv[1], 0);

	Netxx::size_type buffer_size = std::atoi(argv[2]);
	char *buffer = new char[buffer_size];

	Netxx::StreamServer server(addr, Netxx::Timeout(30));
	Netxx::Peer peer = server.accept_connection();
	if (!peer) throw std::runtime_error("bad peer");

	Netxx::Stream client(peer.get_socketfd(), Netxx::Timeout(2));
	Netxx::signed_size_type bytes_aval=buffer_size, bytes_read;
	char *bufptr = buffer;

	while (bytes_aval) {
	    if ( (bytes_read = client.read(bufptr, bytes_aval)) <= 0)
		throw std::runtime_error("failed to read data from client");

	    bufptr += bytes_read;
	    bytes_aval -= bytes_read;
	}

	bytes_aval = buffer_size;

	if (client.write(buffer, buffer_size) != bytes_aval)
	    throw std::runtime_error("failed to write buffer to client");

    } catch (std::exception &e) {
	std::cerr << e.what() << std::endl;
	return 1;
    } catch ( ... ) {
	std::cerr << "caught unknown exception\n";
	return 1;
    }

    return 0;
}
