# $Id: Makefile,v 1.2 2002/09/25 22:51:03 jason Exp $

# must have ARCH set
ARCH = i686

PROG = sipstack

SRC =	\
	Condition.cxx \
	Data.cxx \
	Lock.cxx \
	Log.cxx \
	Logger.cxx \
	Mutex.cxx \
	RandomHex.cxx \
	Subsystem.cxx \
	ThreadIf.cxx \
	Timer.cxx \

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

OBJS = $(patsubst %.cxx,$(OBJ)/%.o,$(SRC))

# OK THE REST OF THE FILE IS NOT REALLY MEANT TO BE MODIFIED


#define the rules

all: $(OBJ) $(BIN) $(BIN)/libutil2.a

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

$(OBJ)/%.o: %.cxx
	$(CXX) $(CXXFLAGS) -c -o $@ $<
	@[ -f $(@F:.o=.d) ] && mv $(@F:.o=.d) $(@:.o=_tmp.d) || \
		mv $(@:.o=.d) $(@:.o=_tmp.d)
	@sed -e "s/^$(@F)/$(@D)\/$(@F)/" < $(@:.o=_tmp.d) > $(@:.o=.d)
	@rm  $(@:.o=_tmp.d)


%.S: %.cxx
	$(CXX) $(CXXFLAGS)  -fverbose-asm -g -Wa,-ahln -c \
		-o /tmp/cjJunk.o $< > $@

$(BIN)/libutil2.a: $(OBJS)
	ar $(ARFLAGS) $@ $^

-include $(OBJ)/*.d

