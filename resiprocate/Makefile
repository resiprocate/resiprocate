# $Id: Makefile,v 1.6 2002/09/21 06:49:59 jason Exp $

# must have ARCH set
ARCH = i686

PROG = sipstack

SRC =	\
	FloatSubComponent.cxx \
	HeaderFieldValue.cxx \
	HeaderFieldValueList.cxx \
	HeaderTypes.cxx \
	IntSubComponent.cxx \
	SipMessage.cxx \
	StringSubComponent.cxx \
	SubComponent.cxx \
	SubComponentList.cxx \
	testMsg.cxx \
	testSubComponentList.cxx \
	UnknownSubComponent.cxx

OSRC =   *.hxx Makefile


#CXXFLAGS += -pg
#LDFLAGS  += -pg
#CXXFLAGS += -Weffc++
# optimize does not work with gcc 2.8 compiler
#CXXFLAGS += -O5 -DNDEBUG
CXXFLAGS += -O -g
#CXXFLAGS += -g
#CXXFLAGS += -DAPR11
#SRC += LeakTracer.cxx
LDFLAGS  += 
CXXFLAGS += -I. -MMD -Wall
LDLIBS   += 

# Add for static link on linux 
#LDFLAGS += -static
#LDLIBS  += -lSM -lICE

# comment out one of follwing lines and touch thread.hxx
#CXXFLAGS += -DNOTHREADS
#LDLIBS   += -lpthread

ifeq ($(ARCH),sun4)
CXXFLAGS +=
LDFLAGS  +=
LDLIBS   +=
endif


ifeq ($(ARCH),i686)
#CXXFLAGS += -mcpu=pentiumpro -march=pentiumpro
CXXFLAGS += -mcpu=i686 -march=i686
#CXXFLAGS += -ffast-math -fno-math-errno 
#CXXFLAGS += -mmx -mmx-only 
#CXXFLAGS += -I/opt/pgcc/include
#LDFLAGS  += -L/opt/pgcc/lib
CXXFLAGS +=
LDFLAGS  +=
LDLIBS   +=
endif

CXXFLAGS += -I../../.

OBJ = obj.$(ARCH)
BIN = bin.$(ARCH)

OBJS = $(patsubst %.cxx,$(OBJ)/%.o,$(SRC))

# OK THE REST OF THE FILE IS NOT REALLY MEANT TO BE MODIFIED


#define the rules

all: $(OBJ) $(BIN) $(BIN)/libSipStack.a

doc: html html/HIER.html html/sipStack.html

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


-include $(OBJ)/*.d
