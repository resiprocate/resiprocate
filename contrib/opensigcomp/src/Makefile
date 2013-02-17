all: libopensigcomp.a test

BUILD_DIR   := ../build
include $(BUILD_DIR)/Makefile.common

#CPPFLAGS += -DLEAK_DEBUG

GENERATED_HEADERS := DeflateBytecodes.h
GENERATED_SOURCES := DeflateBytecodes.cpp

SRC := $(wildcard *.cpp) $(GENERATED_SOURCES)
DEPS := $(patsubst %.cpp, .%.d, $(SRC))
OBJS := $(patsubst %.cpp, %.o, $(SRC))
ASMS := $(patsubst %.cpp, %.s, $(SRC))

libopensigcomp.a: $(OBJS)
	$(AR) r $@ $^

ifneq ($(MAKECMDGOALS),clean)
  -include $(DEPS)
endif

.%.d: %.cpp
	@echo Generating dependencies for $*.o
	@$(CPP) $(CPPFLAGS) -MM $< -MT $*.o -MT .$*.d > $@

%.s: %.cpp
	$(CXX) $(CPPFLAGS) -fverbose-asm -S $<

%.udo: %.asm ../tools/udvm_asm.pl
	@$(PERL) ../tools/udvm_asm.pl $< $@ $*.sym

%Bytecodes.h %Bytecodes.cpp: %.udo ../tools/bytecodes2class.pl
	@$(PERL) ../tools/bytecodes2class.pl $< $*Bytecodes $*.sym
	

.PHONY: clean test size symsize externals asm

clean:
	$(RM) *.a *.o .*.d *.udo *.sym *.s \
		$(GENERATED_SOURCES) $(GENERATED_HEADERS)
	$(MAKE) -C test clean

test: libopensigcomp.a
	$(MAKE) BUILD=$(BUILD) -C test

size: $(OBJS)
	@mkdir tmpsize
	@cp $^ tmpsize
	@cd tmpsize; strip *; du -b * . | sort -n
	@rm -Rf tmpsize

symsize: $(OBJS)
	nm --size-sort -B -td *.o | sort -un | c++filt

externals: libopensigcomp.a
	nm $< | c++filt | grep ' U ' | grep -v 'osc::' | sort -u

asm: $(ASMS)

show.%:
	@echo $*=$($*)

###########################################################################
# Library and header installation

EXTERNAL_HEADERS := \
        BitBuffer.h \
        Buffer.h \
        CompartmentMap.h \
        Compressor.h \
        DeflateCompressor.h \
        Libc.h \
        NackCodes.h \
        ReadWriteLockable.h \
        SigcompMessage.h \
        Stack.h \
        StateChanges.h \
        StateHandler.h \
        StateMap.h \
        TcpStream.h \
        Types.h \
        Udvm.h 

install: libopensigcomp.a $(EXTERNAL_HEADERS)
	$(MKDIR) $(INSTALL_LIB)
	$(COPY) libopensigcomp.a $(INSTALL_LIB)
	$(MKDIR) $(INSTALL_INC)
	$(COPY) $(EXTERNAL_HEADERS) $(INSTALL_INC)
