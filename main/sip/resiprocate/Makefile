# $Id: Makefile,v 1.12 2002/09/21 20:04:00 fluffy Exp $

# must have ARCH set
ARCH = i686

PROG = sipstack

SRC = Condition.cxx Lock.cxx Mutex.cxx Transport.cxx UdpTransport.cxx Log.cxx  Subsystem.cxx Data.cxx Preparse.cxx


OSRC =   *.hxx Makefile


#CXXFLAGS += -pg
#LDFLAGS  += -pg
#CXXFLAGS += -Weffc++
# optimize does not work with gcc 2.8 compiler
#CXXFLAGS += -O5 -DNDEBUG
CXXFLAGS += -O -g
#CXXFLAGS += -g
LDFLAGS  += 
CXXFLAGS += -I. -I../. -I../../. -MD -Wall $(CXXDEBUG)
LDLIBS   += 

ifeq ($(ARCH),i686)
CXXFLAGS += -mcpu=i686 -march=i686
endif

OBJ = obj.$(ARCH)
BIN = bin.$(ARCH)

OBJS = $(patsubst %.cxx,$(OBJ)/%.o,$(SRC))

# OK THE REST OF THE FILE IS NOT REALLY MEANT TO BE MODIFIED


#define the rules

all: $(OBJ) $(BIN) $(BIN)/libSipStack.a

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

html:
	- mkdir html

html/HIER.html: *hxx
	doc++ -B noBanner -p -d html *.hxx

html/sipStack.html: *.cxx *.hxx 
	cvs2html -a -k -o html

fsm.dot: Preparse.cxx dot.awk
	awk -f dot.awk Preparse.cxx > fsm.dot

%.pdf: %.ps
	ps2pdf13 $<

%.ps: %.dot
	dot -Tps -o$@ $<

$(OBJ)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ)/%.o: %.cxx
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.S: %.cxx
	$(CXX) $(CXXFLAGS)  -fverbose-asm -g -Wa,-ahln -c \
		-o /tmp/cjJunk.o $< > $@

$(BIN)/libSipStack.a: $(OBJS)
	ar $(ARFLAGS) $@ $^



testSubComponentList:  $(OBJS) $(OBJ)/testSubComponentList.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

testUdp:  $(OBJS) $(OBJ)/testUdp.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

testPreparse: $(OBJ)/Preparse.o $(OBJ)/testPreparse.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)


-include $(OBJ)/*.d
