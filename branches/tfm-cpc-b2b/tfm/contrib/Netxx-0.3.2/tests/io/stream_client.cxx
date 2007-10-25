#include <Netxx/Stream.h>
#include <Netxx/Timeout.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <exception>
#include <cstring>
#include <cstdlib>

int main (int argc, char *argv[]) {
    if (argc != 4) {
	std::cerr << "Usage: " << argv[0] << " address file size\n";
	return 1;
    }

    try {

	Netxx::Stream client(argv[1], 0, Netxx::Timeout(2));
	std::ifstream file(argv[2]);
	if (!file) throw std::runtime_error("failed to open input file");

	Netxx::size_type buffer_size = std::atoi(argv[3]);
	char *buffer = new char[buffer_size];
	char *return_buffer = new char[buffer_size];

	file.read(buffer, buffer_size);

	if (!file || file.gcount() != buffer_size) {
	    std::ostringstream error;

	    error << "read from file did not give the correct byte count, ";
	    error << "gcount said: " << file.gcount();
	    error << " but I expected: " << buffer_size;

	    throw std::runtime_error(error.str());
	}

	if (client.write(buffer, buffer_size) != buffer_size)
	    throw std::runtime_error("write to server did not send the correct byte count");

	Netxx::signed_size_type bytes_read=0, buffer_aval=buffer_size;
	char *bufptr = return_buffer;

	while (buffer_aval) {
	    if ( (bytes_read = client.read(bufptr, buffer_aval)) <= 0)
		throw std::runtime_error("read from server was less than or equal to 0");

	    bufptr += bytes_read;
	    buffer_aval -= bytes_read;
	}

	if (std::memcmp(buffer, return_buffer, buffer_size) != 0)
	    throw std::runtime_error("buffer mismatch");

    } catch (std::exception &e) {
	std::cerr << e.what() << std::endl;
	return 1;
    } catch ( ... ) {
	std::cerr << "an unknown exception was caught" << std::endl;
	return 1;
    }

    return 0;
}
