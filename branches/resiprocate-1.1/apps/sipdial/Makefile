
RESIPROCATE_PROJECT_HOME=/home/daniel/ws/resiprocate/
#RESIPROCATE_HOME=/home/daniel/ws/resiprocate/main/sip/
RESIPROCATE_HOME=${RESIPROCATE_PROJECT_HOME}

RESIPROCATE_INC=$(RESIPROCATE_HOME)

#RESIPROCATE_LIB=$(RESIPROCATE_HOME)/resip/stack/obj.debug.Linux.i686/
ARES_LIB=$(RESIPROCATE_PROJECT_HOME)/contrib/ares/
#RUTIL_LIB=$(RESIPROCATE_PROJECT_HOME)/rutil/obj.debug.Linux.i686/
#DUM_LIB=$(RESIPROCATE_PROJECT_HOME)/resip/dum/obj.debug.Linux.i686/
#REPRO_LIB=$(RESIPROCATE_PROJECT_HOME)/repro/obj.debug.Linux.i686/
NETXX_LIB=$(RESIPROCATE_PROJECT_HOME)/tfm/contrib/Netxx-0.3.2/src/
#TFM_LIB=$(RESIPROCATE_PROJECT_HOME)/tfm/obj.debug.Linux.i686/

CPP=g++
CPPOPTS=-march=i686 -D_REENTRANT -g  -Wall -c 
INC=-I$(RESIPROCATE_INC) -I/usr/include/cc++2 -I./include/sipdial

LD=g++
#LDOPTS=-static
LDOPTS=
#LIBPATH=-L/usr/local/lib -L$(RESIPROCATE_LIB) -L$(RUTIL_LIB) -L$(DUM_LIB) -L$(ARES_LIB) -L$(NETXX_LIB) -L$(TFM_LIB) -L$(REPRO_LIB)
LIBPATH=-L/usr/local/lib -L$(ARES_LIB) -L$(NETXX_LIB) -L.
LIB=-lsipdial -ldum -lresip -lrutil -lares  -lssl  -lcrypto  -lpthread  -lpopt -ldb_cxx -lpthread 

AR=ar
RANLIB=ranlib

MKSO=gcc -fPIC -shared

OBJ=DialerConfiguration.o DialInstance.o MyInviteSessionHandler.o 

DEMO_OBJ=sipdialer.o

TARGET=libsipdial.so

DEMO_TARGET=sipdialer

all:	$(TARGET) $(DEMO_TARGET)

%.o:	%.cpp
	$(CPP) $(CPPOPTS) $(INC) -o $@ $<

$(TARGET):	$(OBJ)
	$(MKSO) -o $(TARGET) $(OBJ)

$(DEMO_TARGET):	$(DEMO_OBJ)
	$(LD) -o $(DEMO_TARGET) $(DEMO_OBJ) $(LIBPATH) $(LIB) $(LDOPTS)

clean:
	-rm -r *.o $(TARGET)


