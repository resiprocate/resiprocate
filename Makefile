# $Id: Makefile,v 1.3 2002/09/21 00:56:56 jason Exp $

# must have ARCH set
ARCH = i686

PROG = sipstack

SRC = \
	Parameter.cxx \
	ParameterList.cxx \
	StringParameter.cxx \
	UnknownParameter.cxx 

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
LDLIBS   += -lXm -lXt -lX11 -ltiff -lXp
# -lXxf86dga

# Add for static link on linux 
#LDFLAGS += -static
#LDLIBS  += -lSM -lICE

# comment out one of follwing lines and touch thread.hxx
#CXXFLAGS += -DNOTHREADS
#LDLIBS   += -lpthread

ifeq ($(ARCH),sun4)
CXXFLAGS += -I/usr/dt/include -I/usr/openwin/include/X11
LDFLAGS  += -L/usr/dt/lib -L/usr/openwin/lib
LDLIBS   += -lXext
endif


ifeq ($(ARCH),i686)
#CXXFLAGS += -mcpu=pentiumpro -march=pentiumpro
CXXFLAGS += -mcpu=i686 -march=i686
#CXXFLAGS += -ffast-math -fno-math-errno 
#CXXFLAGS += -mmx -mmx-only 
#CXXFLAGS += -I/opt/pgcc/include
#LDFLAGS  += -L/opt/pgcc/lib
CXXFLAGS += -I/usr/X11R6/include
LDFLAGS  += -L/usr/X11R6/lib
LDLIBS   += -lXpm -lXext 
endif

CXXFLAGS += -I../../.

# OK THE REST OF THE FILE IS NOT REALLY MEANT TO BE MODIFIED
OBJ = obj.$(ARCH)
BIN = bin.$(ARCH)

OBJS = $(patsubst %.cxx,$(OBJ)/%.o,$(SRC))

#define the rules

all: $(OBJ) $(BIN) $(BIN)/libSipStack.a

doc: html html/HIER.html html/gesture.html

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

html/gesture.html: *.cxx *.hxx 
	cvs2html -a -k -o html

$(OBJ)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
	sed -e "s/^$(@F)/$(@D)\/$(@F)/" < $(@F:.o=.d) > $(@:.o=.d)
	rm $(@F:.o=.d)

$(OBJ)/%.o: %.cxx
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	sed -e "s/^$(@F)/$(@D)\/$(@F)/" < $(@F:.o=.d) > $(@:.o=.d)
	rm $(@F:.o=.d)

%.S: %.cxx
	$(CXX) $(CXXFLAGS)  -fverbose-asm -g -Wa,-ahln -c \
		-o /tmp/cjJunk.o $< > $@

$(BIN)/$(PROG): $(OBJ)/Gesture.o $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BIN)/$(PROG).pure: $(OBJ)/Gesture.o $(OBJS)
	purify $(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BIN)/libSipStack.a: $(OBJS)
	ar $(ARFLAGS) $@ $^


testParameterList:  $(OBJS) $(OBJ)/testParameterList.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)




-include $(OBJ)/*.d
