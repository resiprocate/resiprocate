BUILD_DIR  := ../../build
include    $(BUILD_DIR)/Makefile.common

SRC := $(wildcard *.asm)
UDO := $(patsubst %.asm, %.udo, $(SRC))
TESTOUTPUT := $(patsubst %.asm, .testoutput/%.2.asm, $(SRC))

all: $(UDO) $(TESTOUTPUT)

%.udo: %.asm ../udvm_asm.pl
	$(PERL) ../udvm_asm.pl $< $@

../udvm_dasm: $(wildcard ../*.h) $(wildcard ../*.cpp) \
              $(wildcard ../../src/*.h) $(wildcard ../../src/*.cpp)
	$(MAKE) -C .. udvm_dasm

show.%:
	@echo $*=$($*)

clean:
	$(RM) $(UDO) .testoutput/*

# The assembler and disassembler are tested by first
# Disassembling the assembled output for each .asm file,
# assembling the disassembled output, and disassembling
# this second assembly. The output of this second
# disassembly step is then compared against the output
# of the first disassembly step, as are the corresponding
# .udo files. If anything doesn't match, then something
# is amiss.

.PRECIOUS: .testoutput/epic.1.asm

.testoutput/%.1.asm: %.udo ../udvm_dasm
	../udvm_dasm $< > $@

.testoutput/%.2.asm: .testoutput/%.1.udo ../udvm_dasm
	../udvm_dasm $< > $@
	@diff $@ .testoutput/$*.1.asm
	@diff $*.udo .testoutput/$*.1.udo
	@rm -f .testoutput/*
