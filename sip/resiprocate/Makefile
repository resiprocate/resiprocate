# $Id: Makefile,v 1.57 2002/10/30 02:14:05 jason Exp $

# must have ARCH set
ARCH = i686

PROG = sipstack

SRC =	\
	DataParameter.cxx \
	Dialog.cxx \
	Executive.cxx \
	ExistsParameter.cxx \
	FloatParameter.cxx \
	HeaderFieldValue.cxx \
	HeaderFieldValueList.cxx \
	Headers.cxx \
	Helper.cxx \
	IntegerParameter.cxx \
	Message.cxx \
	MethodTypes.cxx \
	Parameter.cxx \
	ParameterTypes.cxx \
	ParserCategory.cxx \
	ParserCategories.cxx \
	Preparse.cxx \
	Resolver.cxx \
	SipMessage.cxx \
	SipStack.cxx \
	Symbols.cxx \
	TimerMessage.cxx \
	TimerQueue.cxx \
	TransactionMap.cxx \
	TransactionState.cxx \
	Transport.cxx \
	TransportSelector.cxx \
	UdpTransport.cxx \
	UnknownParameter.cxx \
	Uri.cxx \

# Make sure there is a blank line above this comment.

OSRC =   *.hxx Makefile


#CXXFLAGS += -pg
#LDFLAGS  += -pg
#CXXFLAGS += -Weffc++
# optimize does not work with gcc 2.8 compiler
#CXXFLAGS += -O5 -DNDEBUG
#CXXFLAGS += -O -g
#CXXFLAGS += -O2 -DNDEBUG -DNO_DEBUG
CXXFLAGS += -g
LDFLAGS  += -L$(LIB)
CXXFLAGS += -I. -I../. -MD -Wall $(CXXDEBUG)
LDLIBS   += -lutil2 -lpthread -lssl

ifeq ($(ARCH),i686)
CXXFLAGS += -mcpu=i686 -march=i686
endif

OBJ = obj.$(ARCH)
BIN = bin.$(ARCH)
LIB = ../lib.$(ARCH)

OBJS = $(patsubst %.cxx,$(OBJ)/%.o,$(SRC))

# OK THE REST OF THE FILE IS NOT REALLY MEANT TO BE MODIFIED


#define the rules

all:  $(OBJ) $(BIN) $(LIB) $(BIN)/libSipStack.a

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

$(BIN)/libSipStack.a: $(OBJS) 
	@$(MAKE) $(^:.o=.d)
	@$(MAKE) $(^)
	ar $(ARFLAGS) $@ $^
	@- ln -s $(shell pwd)/$@ $(LIB)

testUdp:  $(OBJS) $(OBJ)/testUdp.o
	@$(MAKE) $(^:.o=.d)
	@$(MAKE) $(^)
	$(CXX) $(LDFLAGS) -o $(BIN)/$@ $^ $(LDLIBS)

testPreparse: $(OBJ)/Preparse.o $(OBJ)/testPreparse.o
	@$(MAKE) $(^:.o=.d)
	@$(MAKE) $(^)
	$(CXX) $(LDFLAGS) -o $(BIN)/$@ $^ $(LDLIBS)

testSipStack1:  $(OBJS) $(OBJ)/testSipStack1.o
	@$(MAKE) $(^:.o=.d)
	@$(MAKE) $(^)
	$(CXX) $(LDFLAGS) -o $(BIN)/$@ $^ $(LDLIBS)

testSipMessage:  $(OBJS) $(OBJ)/testSipMessage.o
	@$(MAKE) $(^:.o=.d)
	@$(MAKE) $(^)
	$(CXX) $(LDFLAGS) -o $(BIN)/$@ $^ $(LDLIBS)

test%: $(OBJ)/test%.o $(OBJS)
	$(CXX) $(LDFLAGS) -o $(BIN)/$@ $^ $(LDLIBS)

headers-hash.c: headers.gperf Makefile
	gperf -L ANSI-C -t -k '3,$$' $< > $@

convertStringToInt:  $(OBJS) $(OBJ)/convertStringToInt.o
	@$(MAKE) $(^:.o=.d)
	@$(MAKE) $(^)
	$(CXX) $(LDFLAGS) -o $(BIN)/$@ $^ $(LDLIBS)

testMutable:  $(OBJS) $(OBJ)/testMutable.o
	$(CXX) $(LDFLAGS) -o $(BIN)/$@ $^ $(LDLIBS)

testParserCategories:  $(OBJS) $(OBJ)/testParserCategories.o
	$(CXX) $(LDFLAGS) -o $(BIN)/$@ $^ $(LDLIBS)

-include $(OBJ)/*.d
