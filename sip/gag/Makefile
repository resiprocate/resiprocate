LIBSIP = ../build/resiprocate/libresiprocate.a
LIBARES = ../contrib/ares/libares.a
CFLAGS = -I ..
LDFLAGS = -lssl -lcrypto -lpthread

all: gag

gag: gag.o GagMessage.o $(LIBSIP) $(LIBARES)
	$(CXX) -o $@ $(LDFLAGS) $^

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

