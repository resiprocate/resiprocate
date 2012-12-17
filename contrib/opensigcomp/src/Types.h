#ifndef __OSC__TYPES
#define __OSC__TYPES 1

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
  @file Types.h

  @brief Definitions of several types used throughout the Open SigComp stack.

  This file's primary purpose is to allow trivial portability to platforms
  that may have varying lengths and/or signs for built-in integer types.

  @todo Relocate the non-portability related cruft from the bottom of
        this file
*/

#include <stdlib.h>

#ifdef DEBUG
#include <iostream>
#include <iomanip>
#endif

namespace osc
{
  /** Used to represent an unsigned 8-bit integer. Equivalent to the ISO
      C99 type uint8_t. */
  typedef unsigned char  u8;

  /** Used to represent an unsigned 16-bit integer. Equivalent to the ISO
      C99 type uint16_t. */
  typedef unsigned short u16;

  /** Used to represent an unsigned 32-bit integer. Equivalent to the ISO
      C99 type uint32_t. */
  typedef unsigned int   u32;

  /** Used to represent a byte, typically for buffer purposes. 
      This is likely to be aliased to type unsigned char on most platforms. */
  typedef unsigned char  byte_t;

  /** Used to designate the number of objects in a collection. This 
      will be an unsigned integer of undetermined length */
  typedef unsigned int   count_t;
}

#ifdef __i386__
#define UNALIGNED_U16_OKAY
#define UNALIGNED_U32_OKAY
#endif

////////////////////////////////////////////////////////////
// The following stuff really doesn't go here.
// This file is intended to be used to adjust variable
// sizes for porting purposes.

namespace osc
{
  class Buffer;

  /**
    Convienience structure used to hold a 20-byte SHA-1 digest value
  */
  typedef struct
  { 
    byte_t digest[20];
  }  sha1_t;

  bool operator <(osc::sha1_t const &digest1,osc::sha1_t const &digest2);
  bool operator ==(osc::sha1_t const &digest1,osc::sha1_t const &digest2);


  /** Buffer type for compartment identification. Can be constructed as
      osc::compartment_id_t(osc::byte_t *bufferPointer, size_t bufferLength) */

  typedef osc::Buffer compartment_id_t;
  
  typedef osc::Buffer state_id_t;

  typedef u32 compressor_id_t;

  /**
    Convenience structure that groups together the parameters defined
    in RFC 3320 to control the behavior of a UDVM.
   */
  typedef struct
  {
    osc::u32 decompression_memory_size;
    osc::u32 state_memory_size;
    osc::u8 cycles_per_bit;
    osc::u16 SigComp_version;
  } sigcomp_parameters_t;

}

#if defined(_MSC_VER) && (_MSC_VER < 1300)
bool osc::operator < (osc::sha1_t const &digest1, osc::sha1_t const &digest2);
bool osc::operator == (osc::sha1_t const &digest1, osc::sha1_t const &digest2);
#else
inline bool osc::operator < (osc::sha1_t const &digest1,
                             osc::sha1_t const &digest2)
{
  osc::count_t pos = 0;
  while(pos<20)
  {
    pos++;
    if(digest1.digest[pos] != digest2.digest[pos])
    {
      return digest1.digest[pos] < digest2.digest[pos];
    }

  }
  return false;
}

inline bool osc::operator == (osc::sha1_t const &digest1,
                             osc::sha1_t const &digest2)
{
  osc::count_t pos = 0;
  while(pos<20)
  {
    if(digest1.digest[pos] != digest2.digest[pos])
    {
      return false;
    }
    pos++;
  }
  return true;
}
#endif

#endif
