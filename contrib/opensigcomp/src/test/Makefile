TARGETS := unit-test speed-test sha1-speed-test object-size pcap-read massive-thread-test

all: $(TARGETS) run-test

BUILD_DIR  := ../../build
include    $(BUILD_DIR)/Makefile.common

CPPFLAGS += -Wno-format
CPPFLAGS += -I..

#CPPFLAGS += -DUSING_COLOR
#CPPFLAGS += -DSTACK_DEBUG
#CPPFLAGS += -DLEAK_DEBUG

LIBS += ../libopensigcomp.a

ifeq ($(POSIX_LOCKING),yes)
  LDFLAGS += -lpthread
endif

ifeq ($(OPENSSL_SHA1),yes)
  LDFLAGS += -lcrypto
endif

SRC := $(wildcard *.cpp)
DEPS := $(patsubst %.cpp, .%.d, $(SRC))
OBJS := $(filter-out massive_thread_test.o object_size.o speed_test.o\
          sha1_speed_test.o pcap_read.o,\
          $(patsubst %.cpp, %.o, $(SRC)))

unit-test: $(OBJS) $(LIBS)
	$(CXX) $(LDFLAGS) -o $@ $^

speed-test: speed_test.o $(LIBS)
	$(CXX) $(LDFLAGS) -o $@ $^

sha1-speed-test: sha1_speed_test.o $(LIBS)
	$(CXX) $(LDFLAGS) -o $@ $^

object-size: object_size.o $(LIBS)
	$(CXX) $(LDFLAGS) -o $@ $^

pcap-read: pcap_read.o $(LIBS)
	$(CXX) $(LDFLAGS) -o $@ $^

massive-thread-test: massive_thread_test.o $(LIBS)
	$(CXX) $(LDFLAGS) -o $@ $^


ifneq ($(MAKECMDGOALS),clean)
  -include $(DEPS)
endif

.%.d: %.cpp
	@echo Generating dependencies for $*.o
	@$(CPP) $(CPPFLAGS) -MM $< -MT $*.o -MT .$*.d > $@

.PHONY: clean run-test

clean:
	$(RM) $(TARGETS) *.o .*.d *.exe

run-test: unit-test
	@./unit-test

LIBFILES = $(wildcard ../*.cpp) $(wildcard ../*.h) $(wildcard ../*.asm)
../libopensigcomp.a: $(LIBFILES)
	$(MAKE) -C .. libopensigcomp.a

show.%:
	@echo $*=$($*)
