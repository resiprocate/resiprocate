#ifndef __OSC__LIBC
#define __OSC__LIBC 1

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
  @file Libc.h
  @brief Preprocessor macros for portability to non-standard libc
         implementations.

  This file contains macros that provide for ease of porting to
  environments that don't necessarily have the standard C library
  available.
*/


#include <string.h>

#define OSC_MEMMOVE(dst,src,len)  ::memmove((dst),(src),(len))
#define OSC_MEMSET(b,c,len)       ::memset((b),(c),(len))

#ifdef HAVE_REALLOC
#include <stdlib.h>
#define OSC_FREE(ptr)             ::free((ptr))
#define OSC_MALLOC(size)          ::malloc((size))
#define OSC_REALLOC(ptr,size)     ::realloc((ptr),(size))
#endif

#endif
