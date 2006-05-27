# Simple Makefile

CC 	= 	gcc
CFLAGS	=	-g -O0 -I../openssl/include

libdtls_shim.a: dtls_shim.o
	ar rv libdtls_shim.a dtls_shim.o 

clean:
	rm -f *.o
