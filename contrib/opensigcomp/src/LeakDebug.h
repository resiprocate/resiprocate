#ifndef __OSC__LEAK_DEBUG
#define __OSC__LEAK_DEBUG 1

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
  @file LeakDebug.h
  @brief Debugging macros used to detect memory leaks

  If compiled with LEAK_DEBUG defined these macros will
  keep track of the number of instances of each class are
  instantiated in the static variable Class::dInstances;
*/

#if defined(LEAK_DEBUG)

#define DEBUG_LEAK_HEADER_HOOK \
  public:\
    void * operator new(size_t s); \
    void operator delete(void *p); \
    static int dInstances;

#define DEBUG_LEAK_BODY_HOOK(c) \
  int osc::c::dInstances = 0; \
  void * osc::c::operator new(size_t s) { dInstances++; return malloc(s); } \
  void osc::c::operator delete(void *p) { dInstances--; free(p); } \

#else
#define DEBUG_LEAK_HEADER_HOOK
#define DEBUG_LEAK_BODY_HOOK(c)
#endif

#endif
