TARGETS=stream_client stream_server

PERL=/usr/bin/perl
BASEDIR=../..
NETXX=${BASEDIR}/src/libNetxx.a
CXXFLAGS+=-I${BASEDIR}/include

all: ${TARGETS}

test: ${TARGETS}
	${PERL} test_stream.pl

stream_client: stream_client.o
	${CXX} ${CXXFLAGS} -o $@ stream_client.o ${NETXX}

stream_server: stream_server.o
	${CXX} ${CXXFLAGS} -o $@ stream_server.o ${NETXX}

clean:
	rm -f ${TARGETS} *.o

.SUFFIXES: .cxx

.cxx.o:
	$(CXX) $(CXXFLAGS) -c $<
