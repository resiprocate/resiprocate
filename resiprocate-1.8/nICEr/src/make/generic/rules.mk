#
# defaults
#
OBJSUFFIX=o
ARSUFFIX=a
RANLIB=ranlib
CP=cp

COMPILE.c     = $(CC) $(CFLAGS) $(CPPFLAGS) $(CPPFLAGS.c) -c -o
COMPILE.cpp    = $(CXX) $(CFLAGS) $(CPPFLAGS) $(CPPFLAGS.cpp) -c -o
COMPILE.cxx    = $(CXX) $(CFLAGS) $(CPPFLAGS) $(CPPFLAGS.cxx) -c -o
LINK.c        = $(CC) $(LDFLAGS) $(LDFLAGS.c) -o
LINK.cpp        = $(CXX) $(LDFLAGS) $(LDFLAGS.cpp) -o
LINK.cxx        = $(CXX) $(LDFLAGS) $(LDFLAGS.cxx) -o
COMPILE.clic  = clic $< -- $(COMPILE.c)
