# VSM (Very Simplistic Makefile)

CC=g++
RESIPHOME=..
RESIPINC=$(RESIPHOME)
RESIPLIB=$(RESIPHOME)/build/resiprocate


CXXFLAGS = -Wall -fPIC -Wno-deprecated -mcpu=i686 -march=i686 -D_REENTRANT -DUSE_PTHREADS -ggdb -DUSE_SSL -O0 -I.  -I$(RESIPINC)/contrib/ares -I$(RESIPINC)

LDFLAGS = -L$(RESIPHOME)/contrib/ares -L$(RESIPLIB)

LDLIBS = -lresiprocate -lssl -lcrypto -lares -lpthread 

presSvr: presSvr.o PresConfig.o TuPresSvr.o ResourceMgr.o SubDialogMgr.o SubDialog.o Resource.o DialogSetMgr.o DialogState.o

clean:
	rm *.o
