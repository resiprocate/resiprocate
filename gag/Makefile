LIBSIP = ../build/resiprocate/libresiprocate.a
LIBARES = ../contrib/ares/libares.a
GETOPT = ../contrib/getopt/getopt.o
CFLAGS = -I .. -g
LDFLAGS = -lssl -lcrypto -lpthread

all: gag

gag: gag.o GagMessage.o GagConduit.o $(LIBSIP) $(LIBARES) $(GETOPT)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cxx
	$(CXX) -c $(CFLAGS) -o $@ $<

show.%:
	@echo $*=$($*)

clean:
	$(RM) *.o gag .*.d

# Fast and sloppy dependencies
DEPS := $(patsubst %.cxx, .%.d, $(wildcard *.cxx))
-include $(DEPS)
.%.d: %.cxx
	@echo Generating dependencies for $*.o
	@$(CPP) $(CFLAGS) -MM $< -MT $*.o -MT .$*.d > $@

