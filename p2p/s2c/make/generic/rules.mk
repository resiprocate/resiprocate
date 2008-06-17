#    
#    Copyright (C) 2006, Network Resonance, Inc.
#    All Rights Reserved
#    
#    Redistribution and use in source and binary forms, with or without
#    modification, are permitted provided that the following conditions
#    are met:
#    
#    1. Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#    2. Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#    3. Neither the name of Network Resonance, Inc. nor the name of any
#       contributors to this software may be used to endorse or promote 
#       products derived from this software without specific prior written
#       permission.
#    
#    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
#    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
#    ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
#    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
#    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
#    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
#    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
#    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
#    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#    POSSIBILITY OF SUCH DAMAGE.
#    
#
# defaults
#
OBJSUFFIX=o
ARSUFFIX=a
RANLIB=ranlib
CP=cp

COMPILE.c     = $(CC) $(CFLAGS) $(CPPFLAGS) -c -o
LINK.c        = $(CC) $(LDFLAGS) -o
COMPILE.cxx   = $(CXX) $(CFLAGS) $(CPPFLAGS) -c -o
LINK.cxx        = $(CXX) $(LDFLAGS) -o

COMPILE.clic  = ../../tools/clic/clic $< -- $(COMPILE.c)
COMPILE.y     = perl ../generic/compile_wrap "yacc -dv -o  $(<:.y=.c)" $< $(<:.y=.c) $(COMPILE.c)
COMPILE.l     = perl ../generic/compile_wrap "lex -o$(<:.l=.c)" $< $(<:.l=.c) $(COMPILE.c)
COMPILE.s2c   = perl ../generic/compile_wrap "./s2c " $< `basename $(<:.s2c=Gen.cxx)` $(COMPILE.c)