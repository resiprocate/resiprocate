# $Id: Makefile,v 1.10 2002/10/13 06:02:36 jason Exp $

# must have ARCH set
ARCH = i686

PROG = sipstack

SRC =	\
	Condition.cxx \
	Data.cxx \
	Fifo.cxx \
	Lock.cxx \
	Log.cxx \
	Logger.cxx \
	Mutex.cxx \
	ParseBuffer.cxx \
	RandomHex.cxx \
	Socket.cxx \
	Subsystem.cxx \
	ThreadIf.cxx \
	Timer.cxx \
	VException.cxx \

OSRC =   *.hxx Makefile


CXXFLAGS += -O -g 
LDFLAGS  += 
CXXFLAGS += -I. -I../. -MD -Wall $(CXXDEBUG)
LDLIBS   += -lssl -lpthread

ifeq ($(ARCH),i686)
CXXFLAGS += -mcpu=i686 -march=i686
endif

OBJ = obj.$(ARCH)
BIN = bin.$(ARCH)
LIB = ../lib.$(ARCH)

OBJS = $(patsubst %.cxx,$(OBJ)/%.o,$(SRC))

# OK THE REST OF THE FILE IS NOT REALLY MEANT TO BE MODIFIED


#define the rules

all: $(OBJ) $(BIN) $(LIB) $(BIN)/libutil2.a

doc: html html/HIER.html html/sipStack.html fsm.pdf

clean:
	-rm -f *.rpo core *~ \#* .make* *.a *.d
	-rm -rf html
	-rm obj.*/*.[od] bin.*/$(PROG) 

backup:
	tar cvf - $(SRC) $(OSRC) | gzip > `date +"backup_$(PROG)%b%d.tgz"`

$(BIN):
	- mkdir $(BIN)

$(OBJ):
	- mkdir $(OBJ)

$(LIB):
	- mkdir $(LIB)

html:
	- mkdir html

html/HIER.html: *hxx
	doc++ -B noBanner -p -d html *.hxx

html/sipStack.html: *.cxx *.hxx 
	cvs2html -a -k -o html

$(OBJ)/%.o: %.cxx
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	@[ -f $(@F:.o=.d) ] && mv $(@F:.o=.d) $(@:.o=_tmp.d) || \
		mv $(@:.o=.d) $(@:.o=_tmp.d)
	@sed -e "s/^$(@F)/$(@D)\/$(@F)/" < $(@:.o=_tmp.d) > $(@:.o=.d)
	@rm  $(@:.o=_tmp.d)


$(OBJ)/%.d: %.cxx
	$(CXX) $(CXXFLAGS) -c -o $(@:.d=.o) $<
	@[ -f $(@F) ] && mv $(@F) $(@:.d=_tmp.d) || \
		mv $(@) $(@:.d=_tmp.d)
	@sed -e "s/^$(@F:.d=.o)/$(@D)\/$(@F:.d=.o)/" < $(@:.d=_tmp.d) > $(@)
	@rm  $(@:.d=_tmp.d)

%.S: %.cxx
	$(CXX) $(CXXFLAGS)  -fverbose-asm -g -Wa,-ahln -c \
		-o /tmp/cjJunk.o $< > $@

$(BIN)/libutil2.a: $(OBJS)
	@$(MAKE) $(^:.o=.d)
	@$(MAKE) $(^)
	ar $(ARFLAGS) $@ $^
	-@ ln -s $(shell pwd)/$@ $(LIB)


-include $(OBJ)/*.d

# OK THE REST OF THE FILE IS OK TO MODIFY

testParseBuffer:  $(OBJS) $(OBJ)/testParseBuffer.o 
	@$(MAKE) $(^:.o=.d)
	@$(MAKE) $(^)
	$(CXX) $(LDFLAGS) -o $(BIN)/$@ $^ $(LDLIBS)

testData:  $(OBJS) $(OBJ)/testData.o 
	@$(MAKE) $(^:.o=.d)
	@$(MAKE) $(^)
	$(CXX) $(LDFLAGS) -o $(BIN)/$@ $^ $(LDLIBS)
