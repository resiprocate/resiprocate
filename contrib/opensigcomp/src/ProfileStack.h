#ifndef __OSC__STACK_PROFILE
#define __OSC__STACK_PROFILE 1

/* ***********************************************************************
   Open SigComp -- Implementation of RFC 3320 Signaling Compression

   Copyright 2005 Estacado Systems, LLC

   Your use of this code is governed by the license under which it
   has been provided to you. Unless you have a written and signed
   document from Estacado Systems, LLC stating otherwise, your license
   is as provided by the GNU General Public License version 2, a copy
   of which is available in this project in the file named "LICENSE."
   Alternately, a copy of the licence is available by writing to
   the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
   Boston, MA 02110-1307 USA

   Unless your use of this code is goverened by a written and signed
   contract containing provisions to the contrary, this program is
   distributed WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY of FITNESS FOR A PARTICULAR PURPOSE. See the
   license for additional details.

   To discuss alternate licensing terms, contact info@estacado.net
 *********************************************************************** */

/**
  @file ProfileStack.h
  @brief Debugging macros used to profile stack memory usage.

  If compiled with STACK_DEBUG defined for an x86 processor
  using the g++ compiler, these macros will generate output
  indicating current stack usage whenever they are called.
  For propre use, the DEBUG_STACK_FRAME macro needs to be
  included at the beginning of every method and function
  in the library.
*/

#if defined(STACK_DEBUG) && defined(__i386__) && defined (__GNUC__)

#include <stdio.h>

static void *g_ebp;
static int g_localStackSize;

/*
  We use printf here because it is less disruptive to the stack
  pointer than using std::cout.
*/
#define DEBUG_STACK_FRAME \
  do \
  { \
    asm("movl %%ebp,%0\n\t" \
        "movl %%ebp,%1\n\t" \
        "subl %%esp,%1" : "=g" (g_ebp), "=g" (g_localStackSize) ); \
    printf ("@0x%8.8x+%d %s:%d: %s\n", \
                  (int)(g_ebp),g_localStackSize,__FILE__,__LINE__, \
            __PRETTY_FUNCTION__); \
    do \
    { \
      g_ebp = *(void **)g_ebp; \
      printf("  previous frame = 0x%8.8x\n",(int)(g_ebp));\
    } \
    while (g_ebp); \
  } \
  while(0)

#else
#define DEBUG_STACK_FRAME ((void)0)
#endif

#endif
