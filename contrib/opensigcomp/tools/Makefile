all: asm_syntax.gif udvm_dasm test

BUILD_DIR   := ../build
include $(BUILD_DIR)/Makefile.common

CPPFLAGS += -I../src

ifeq ($(POSIX_LOCKING),yes)
  LDFLAGS += -lpthread
endif

ifeq ($(OPENSSL_SHA1),yes)
  LDFLAGS += -lcrypto
endif

SRC := $(wildcard *.cpp)
DEPS := $(patsubst %.cpp, .%.d, $(SRC))

ifneq ($(MAKECMDGOALS),clean)
  -include $(DEPS)
endif

%.svg: %.dot
	-dot -Tsvg $< -o $@

%.gif: %.dot
	-dot -Tgif $< -o $@

udvm_dasm: udvm_dasm.o Disassembler.o ../src/libopensigcomp.a
	$(CXX) -o $@ $^ $(LDFLAGS)

LIBFILES = $(wildcard ../src/*.cpp) $(wildcard ../src/*.h)
../src/libopensigcomp.a: $(LIBFILES)
	$(MAKE) -C ../src libopensigcomp.a

.%.d: %.cpp
	@echo Generating dependencies for $*.o
	@$(CPP) $(CPPFLAGS) -MM $< -MT $*.o -MT .$*.d > $@

.PHONY: clean test

test:
	$(MAKE) -C test

show.%:
	@echo $*=$($*)

clean:
	$(RM) asm_syntax.gif udvm_dasm *.o .*.d *.exe
	$(MAKE) -C test clean

