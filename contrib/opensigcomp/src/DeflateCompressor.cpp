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
  @file DeflateCompressor.cpp
  @brief Implementation of osc::DeflateCompressor class.
*/



#include "ProfileStack.h"
#include <assert.h>
#include "DeflateCompressor.h"
#include "DeflateDictionary.h"
#include "BitBuffer.h"
#include "SigcompMessage.h"
#include "DeflateData.h"
#include "Compartment.h"
#include "DeflateBytecodes.h"
#include "StateHandler.h"
#include "StateHandler.h"
#include "Sha1Hasher.h"
#include "State.h"

/* These lookup table pairs are in the order {bits, value} */

#ifndef OPTIMIZE_SIZE
osc::DeflateCompressor::lookup_table_t
osc::DeflateCompressor::c_literalTable[256] =
{
  {8, 0x30}, {8, 0x31}, {8, 0x32}, {8, 0x33}, {8, 0x34}, {8, 0x35}, 
  {8, 0x36}, {8, 0x37}, {8, 0x38}, {8, 0x39}, {8, 0x3A}, {8, 0x3B}, 
  {8, 0x3C}, {8, 0x3D}, {8, 0x3E}, {8, 0x3F}, {8, 0x40}, {8, 0x41}, 
  {8, 0x42}, {8, 0x43}, {8, 0x44}, {8, 0x45}, {8, 0x46}, {8, 0x47}, 
  {8, 0x48}, {8, 0x49}, {8, 0x4A}, {8, 0x4B}, {8, 0x4C}, {8, 0x4D}, 
  {8, 0x4E}, {8, 0x4F}, {8, 0x50}, {8, 0x51}, {8, 0x52}, {8, 0x53}, 
  {8, 0x54}, {8, 0x55}, {8, 0x56}, {8, 0x57}, {8, 0x58}, {8, 0x59}, 
  {8, 0x5A}, {8, 0x5B}, {8, 0x5C}, {8, 0x5D}, {8, 0x5E}, {8, 0x5F}, 
  {8, 0x60}, {8, 0x61}, {8, 0x62}, {8, 0x63}, {8, 0x64}, {8, 0x65}, 
  {8, 0x66}, {8, 0x67}, {8, 0x68}, {8, 0x69}, {8, 0x6A}, {8, 0x6B}, 
  {8, 0x6C}, {8, 0x6D}, {8, 0x6E}, {8, 0x6F}, {8, 0x70}, {8, 0x71}, 
  {8, 0x72}, {8, 0x73}, {8, 0x74}, {8, 0x75}, {8, 0x76}, {8, 0x77}, 
  {8, 0x78}, {8, 0x79}, {8, 0x7A}, {8, 0x7B}, {8, 0x7C}, {8, 0x7D}, 
  {8, 0x7E}, {8, 0x7F}, {8, 0x80}, {8, 0x81}, {8, 0x82}, {8, 0x83}, 
  {8, 0x84}, {8, 0x85}, {8, 0x86}, {8, 0x87}, {8, 0x88}, {8, 0x89}, 
  {8, 0x8A}, {8, 0x8B}, {8, 0x8C}, {8, 0x8D}, {8, 0x8E}, {8, 0x8F}, 
  {8, 0x90}, {8, 0x91}, {8, 0x92}, {8, 0x93}, {8, 0x94}, {8, 0x95}, 
  {8, 0x96}, {8, 0x97}, {8, 0x98}, {8, 0x99}, {8, 0x9A}, {8, 0x9B}, 
  {8, 0x9C}, {8, 0x9D}, {8, 0x9E}, {8, 0x9F}, {8, 0xA0}, {8, 0xA1}, 
  {8, 0xA2}, {8, 0xA3}, {8, 0xA4}, {8, 0xA5}, {8, 0xA6}, {8, 0xA7}, 
  {8, 0xA8}, {8, 0xA9}, {8, 0xAA}, {8, 0xAB}, {8, 0xAC}, {8, 0xAD}, 
  {8, 0xAE}, {8, 0xAF}, {8, 0xB0}, {8, 0xB1}, {8, 0xB2}, {8, 0xB3}, 
  {8, 0xB4}, {8, 0xB5}, {8, 0xB6}, {8, 0xB7}, {8, 0xB8}, {8, 0xB9}, 
  {8, 0xBA}, {8, 0xBB}, {8, 0xBC}, {8, 0xBD}, {8, 0xBE}, {8, 0xBF}, 
  {9, 0x190}, {9, 0x191}, {9, 0x192}, {9, 0x193}, {9, 0x194}, {9, 0x195}, 
  {9, 0x196}, {9, 0x197}, {9, 0x198}, {9, 0x199}, {9, 0x19A}, {9, 0x19B}, 
  {9, 0x19C}, {9, 0x19D}, {9, 0x19E}, {9, 0x19F}, {9, 0x1A0}, {9, 0x1A1}, 
  {9, 0x1A2}, {9, 0x1A3}, {9, 0x1A4}, {9, 0x1A5}, {9, 0x1A6}, {9, 0x1A7}, 
  {9, 0x1A8}, {9, 0x1A9}, {9, 0x1AA}, {9, 0x1AB}, {9, 0x1AC}, {9, 0x1AD}, 
  {9, 0x1AE}, {9, 0x1AF}, {9, 0x1B0}, {9, 0x1B1}, {9, 0x1B2}, {9, 0x1B3}, 
  {9, 0x1B4}, {9, 0x1B5}, {9, 0x1B6}, {9, 0x1B7}, {9, 0x1B8}, {9, 0x1B9}, 
  {9, 0x1BA}, {9, 0x1BB}, {9, 0x1BC}, {9, 0x1BD}, {9, 0x1BE}, {9, 0x1BF}, 
  {9, 0x1C0}, {9, 0x1C1}, {9, 0x1C2}, {9, 0x1C3}, {9, 0x1C4}, {9, 0x1C5}, 
  {9, 0x1C6}, {9, 0x1C7}, {9, 0x1C8}, {9, 0x1C9}, {9, 0x1CA}, {9, 0x1CB}, 
  {9, 0x1CC}, {9, 0x1CD}, {9, 0x1CE}, {9, 0x1CF}, {9, 0x1D0}, {9, 0x1D1}, 
  {9, 0x1D2}, {9, 0x1D3}, {9, 0x1D4}, {9, 0x1D5}, {9, 0x1D6}, {9, 0x1D7}, 
  {9, 0x1D8}, {9, 0x1D9}, {9, 0x1DA}, {9, 0x1DB}, {9, 0x1DC}, {9, 0x1DD}, 
  {9, 0x1DE}, {9, 0x1DF}, {9, 0x1E0}, {9, 0x1E1}, {9, 0x1E2}, {9, 0x1E3}, 
  {9, 0x1E4}, {9, 0x1E5}, {9, 0x1E6}, {9, 0x1E7}, {9, 0x1E8}, {9, 0x1E9}, 
  {9, 0x1EA}, {9, 0x1EB}, {9, 0x1EC}, {9, 0x1ED}, {9, 0x1EE}, {9, 0x1EF}, 
  {9, 0x1F0}, {9, 0x1F1}, {9, 0x1F2}, {9, 0x1F3}, {9, 0x1F4}, {9, 0x1F5}, 
  {9, 0x1F6}, {9, 0x1F7}, {9, 0x1F8}, {9, 0x1F9}, {9, 0x1FA}, {9, 0x1FB}, 
  {9, 0x1FC}, {9, 0x1FD}, {9, 0x1FE}, {9, 0x1FF}
};

osc::DeflateCompressor::lookup_table_t
osc::DeflateCompressor::c_lengthTable[259] =
{
  /* Lengths 0 through 2 cannot be encoded */
  {0,0}, {0,0}, {0,0},

  /* Code = 257, Lengths = 3 - 3 */
  {7, 0x0001}, 

  /* Code = 258, Lengths = 4 - 4 */
  {7, 0x0002}, 

  /* Code = 259, Lengths = 5 - 5 */
  {7, 0x0003}, 

  /* Code = 260, Lengths = 6 - 6 */
  {7, 0x0004}, 

  /* Code = 261, Lengths = 7 - 7 */
  {7, 0x0005}, 

  /* Code = 262, Lengths = 8 - 8 */
  {7, 0x0006}, 

  /* Code = 263, Lengths = 9 - 9 */
  {7, 0x0007}, 

  /* Code = 264, Lengths = 10 - 10 */
  {7, 0x0008}, 

  /* Code = 265, Lengths = 11 - 12 */
  {8, 0x0012}, {8, 0x0013}, 

  /* Code = 266, Lengths = 13 - 14 */
  {8, 0x0014}, {8, 0x0015}, 

  /* Code = 267, Lengths = 15 - 16 */
  {8, 0x0016}, {8, 0x0017}, 

  /* Code = 268, Lengths = 17 - 18 */
  {8, 0x0018}, {8, 0x0019}, 

  /* Code = 269, Lengths = 19 - 22 */
  {9, 0x0034}, {9, 0x0035}, {9, 0x0036}, {9, 0x0037}, 

  /* Code = 270, Lengths = 23 - 26 */
  {9, 0x0038}, {9, 0x0039}, {9, 0x003A}, {9, 0x003B}, 

  /* Code = 271, Lengths = 27 - 30 */
  {9, 0x003C}, {9, 0x003D}, {9, 0x003E}, {9, 0x003F}, 

  /* Code = 272, Lengths = 31 - 34 */
  {9, 0x0040}, {9, 0x0041}, {9, 0x0042}, {9, 0x0043}, 

  /* Code = 273, Lengths = 35 - 42 */
  {10, 0x0088}, {10, 0x0089}, {10, 0x008A}, {10, 0x008B}, {10, 0x008C}, 
  {10, 0x008D}, {10, 0x008E}, {10, 0x008F}, 

  /* Code = 274, Lengths = 43 - 50 */
  {10, 0x0090}, {10, 0x0091}, {10, 0x0092}, {10, 0x0093}, {10, 0x0094}, 
  {10, 0x0095}, {10, 0x0096}, {10, 0x0097}, 

  /* Code = 275, Lengths = 51 - 58 */
  {10, 0x0098}, {10, 0x0099}, {10, 0x009A}, {10, 0x009B}, {10, 0x009C}, 
  {10, 0x009D}, {10, 0x009E}, {10, 0x009F}, 

  /* Code = 276, Lengths = 59 - 66 */
  {10, 0x00A0}, {10, 0x00A1}, {10, 0x00A2}, {10, 0x00A3}, {10, 0x00A4}, 
  {10, 0x00A5}, {10, 0x00A6}, {10, 0x00A7}, 

  /* Code = 277, Lengths = 67 - 82 */
  {11, 0x0150}, {11, 0x0151}, {11, 0x0152}, {11, 0x0153}, {11, 0x0154}, 
  {11, 0x0155}, {11, 0x0156}, {11, 0x0157}, {11, 0x0158}, {11, 0x0159}, 
  {11, 0x015A}, {11, 0x015B}, {11, 0x015C}, {11, 0x015D}, {11, 0x015E}, 
  {11, 0x015F}, 

  /* Code = 278, Lengths = 83 - 98 */
  {11, 0x0160}, {11, 0x0161}, {11, 0x0162}, {11, 0x0163}, {11, 0x0164}, 
  {11, 0x0165}, {11, 0x0166}, {11, 0x0167}, {11, 0x0168}, {11, 0x0169}, 
  {11, 0x016A}, {11, 0x016B}, {11, 0x016C}, {11, 0x016D}, {11, 0x016E}, 
  {11, 0x016F}, 

  /* Code = 279, Lengths = 99 - 114 */
  {11, 0x0170}, {11, 0x0171}, {11, 0x0172}, {11, 0x0173}, {11, 0x0174}, 
  {11, 0x0175}, {11, 0x0176}, {11, 0x0177}, {11, 0x0178}, {11, 0x0179}, 
  {11, 0x017A}, {11, 0x017B}, {11, 0x017C}, {11, 0x017D}, {11, 0x017E}, 
  {11, 0x017F}, 

  /* Code = 280, Lengths = 115 - 130 */
  {12, 0x0C00}, {12, 0x0C01}, {12, 0x0C02}, {12, 0x0C03}, {12, 0x0C04}, 
  {12, 0x0C05}, {12, 0x0C06}, {12, 0x0C07}, {12, 0x0C08}, {12, 0x0C09}, 
  {12, 0x0C0A}, {12, 0x0C0B}, {12, 0x0C0C}, {12, 0x0C0D}, {12, 0x0C0E}, 
  {12, 0x0C0F}, 

  /* Code = 281, Lengths = 131 - 162 */
  {13, 0x1820}, {13, 0x1821}, {13, 0x1822}, {13, 0x1823}, {13, 0x1824}, 
  {13, 0x1825}, {13, 0x1826}, {13, 0x1827}, {13, 0x1828}, {13, 0x1829}, 
  {13, 0x182A}, {13, 0x182B}, {13, 0x182C}, {13, 0x182D}, {13, 0x182E}, 
  {13, 0x182F}, {13, 0x1830}, {13, 0x1831}, {13, 0x1832}, {13, 0x1833}, 
  {13, 0x1834}, {13, 0x1835}, {13, 0x1836}, {13, 0x1837}, {13, 0x1838}, 
  {13, 0x1839}, {13, 0x183A}, {13, 0x183B}, {13, 0x183C}, {13, 0x183D}, 
  {13, 0x183E}, {13, 0x183F}, 

  /* Code = 282, Lengths = 163 - 194 */
  {13, 0x1840}, {13, 0x1841}, {13, 0x1842}, {13, 0x1843}, {13, 0x1844}, 
  {13, 0x1845}, {13, 0x1846}, {13, 0x1847}, {13, 0x1848}, {13, 0x1849}, 
  {13, 0x184A}, {13, 0x184B}, {13, 0x184C}, {13, 0x184D}, {13, 0x184E}, 
  {13, 0x184F}, {13, 0x1850}, {13, 0x1851}, {13, 0x1852}, {13, 0x1853}, 
  {13, 0x1854}, {13, 0x1855}, {13, 0x1856}, {13, 0x1857}, {13, 0x1858}, 
  {13, 0x1859}, {13, 0x185A}, {13, 0x185B}, {13, 0x185C}, {13, 0x185D}, 
  {13, 0x185E}, {13, 0x185F}, 

  /* Code = 283, Lengths = 195 - 226 */
  {13, 0x1860}, {13, 0x1861}, {13, 0x1862}, {13, 0x1863}, {13, 0x1864}, 
  {13, 0x1865}, {13, 0x1866}, {13, 0x1867}, {13, 0x1868}, {13, 0x1869}, 
  {13, 0x186A}, {13, 0x186B}, {13, 0x186C}, {13, 0x186D}, {13, 0x186E}, 
  {13, 0x186F}, {13, 0x1870}, {13, 0x1871}, {13, 0x1872}, {13, 0x1873}, 
  {13, 0x1874}, {13, 0x1875}, {13, 0x1876}, {13, 0x1877}, {13, 0x1878}, 
  {13, 0x1879}, {13, 0x187A}, {13, 0x187B}, {13, 0x187C}, {13, 0x187D}, 
  {13, 0x187E}, {13, 0x187F}, 

  /* Code = 284, Lengths = 227 - 257 */
  {13, 0x1880}, {13, 0x1881}, {13, 0x1882}, {13, 0x1883}, {13, 0x1884}, 
  {13, 0x1885}, {13, 0x1886}, {13, 0x1887}, {13, 0x1888}, {13, 0x1889}, 
  {13, 0x188A}, {13, 0x188B}, {13, 0x188C}, {13, 0x188D}, {13, 0x188E}, 
  {13, 0x188F}, {13, 0x1890}, {13, 0x1891}, {13, 0x1892}, {13, 0x1893}, 
  {13, 0x1894}, {13, 0x1895}, {13, 0x1896}, {13, 0x1897}, {13, 0x1898}, 
  {13, 0x1899}, {13, 0x189A}, {13, 0x189B}, {13, 0x189C}, {13, 0x189D}, 
  {13, 0x189E}, 

  // This code is not used.
  // {13, 0x189F}, 

  /* Code = 285, Lengths = 258 - 258 */
  {8, 0x00C5}

};
#endif

/**
  Constructor for osc::DeflateCompressor.

  @param stateHandler  Reference to state handler that this compressor
                       uses to store and retrieve compression-related
                       state. This must be the same StateHandler that
                       is used by the stack to which the compressor is
                       to be added.
 */
osc::DeflateCompressor::DeflateCompressor(osc::StateHandler &stateHandler)
  : Compressor(stateHandler)
{
  DEBUG_STACK_FRAME;
  assert(stateHandler.getSigcompVersion() == 2);
}

// Using "this" in a constructor list is not always kosher, so MSVC++
// complains about it. In practice, compressors can't be copied, so
// this code should never be invoked anyway.
#ifdef _MSC_VER
#pragma warning ( disable : 4355 )
#endif

/**
  Copy constructor for osc::DeflateCompressor.
 */
osc::DeflateCompressor::DeflateCompressor(DeflateCompressor const &r)
  : Compressor(*this)
{
  DEBUG_STACK_FRAME;
  /* Assign attributes */
  assert(0);
}

/**
  Destructor for osc::DeflateCompressor.
 */
osc::DeflateCompressor::~DeflateCompressor()
{
  DEBUG_STACK_FRAME;
}

/**
  Assignment operator for osc::DeflateCompressor.
 */
osc::DeflateCompressor &
osc::DeflateCompressor::operator=(DeflateCompressor const &r)
{
  DEBUG_STACK_FRAME;
  if (&r == this)
  {
    return *this;
  }
  /* Assign attributes */
  assert(0);
  return *this;
}

bool
osc::DeflateCompressor::encodeLiteral(BitBuffer &buffer, byte_t symbol)
{
  DEBUG_STACK_FRAME;
#ifdef OPTIMIZE_SIZE
  // The following code is equivalent to using the lookup table;
  // however, encoding bits into an LSB buffer in MSB order is much
  // slower than encoding them in LSB order. As usual, this is a
  // space/time tradeoff.

  if (symbol < 0x90)
  {
    return buffer.appendMsbFirst(8, symbol + 0x30);
  }
  else
  {
    return buffer.appendMsbFirst(9, symbol + 0x100);
  }
#else
  return buffer.appendMsbFirst(c_literalTable[symbol].bits,
                               c_literalTable[symbol].value);
#endif

  return false;
}

bool
osc::DeflateCompressor::encodeLength(BitBuffer &buffer, int length)
{
  DEBUG_STACK_FRAME;
#ifdef OPTIMIZE_SIZE
  if (length <= 10)
  {
    return buffer.appendMsbFirst(7, length - 2);
  }

  if (length == 258)
  {
    return buffer.appendMsbFirst(8, 197);
  }

  int base = 10;
  int code = 265;
  int limit;

  for (int extra = 1; extra <= 5; extra++)
  {
    for (int i = 0; i < 4; i++)
    {
      limit = base + (1 << extra);
      if (length <= limit)
      {
        if (code <= 279)
        {
          buffer.appendMsbFirst(7, code - 256);
        }
        else
        {
          buffer.appendMsbFirst(8, code - 280 + 192);
        }
        return buffer.appendMsbFirst(extra, length - (base + 1));
      }
      code++;
      base = limit;
    }
  }
  return false;
#else
  return buffer.appendMsbFirst(c_lengthTable[length].bits,
                               c_lengthTable[length].value);
#endif
}


bool
osc::DeflateCompressor::encodeDistance(BitBuffer &buffer, int distance)
{
  DEBUG_STACK_FRAME;
  if (distance <= 4)
  {
    return buffer.appendMsbFirst(5, distance - 1);
  }

  int base = 4;
  int code = 4;
  int limit;

  for (int extra = 1; extra <= 13; extra++)
  {
    // First value at this bit length
    limit = base + (1 << extra);
    if (distance <= limit)
    {
      buffer.appendMsbFirst(5, code);
      return buffer.appendMsbFirst(extra, distance - (base + 1));
    }
    code++;
    base = limit;

    // Second value at this bit length
    limit = base + (1 << extra);
    if (distance <= limit)
    {
      buffer.appendMsbFirst(5, code);
      return buffer.appendMsbFirst(extra, distance - (base + 1));
    }
    code++;
    base = limit;
  }

  return false;
}


/**
  Helper function for osc::DeflateCompressor::generateNewState.
*/
inline void appendWord(osc::byte_t *&curr, osc::u16 word)
{
  DEBUG_STACK_FRAME;
  *(curr++) = (word >> 8) & 0xFF;
  *(curr++) = (word) & 0xFF;
}

/**
  Helper function for osc::DeflateCompressor::generateNewState.
*/
inline void appendBytes(osc::byte_t *&curr, 
                        const osc::byte_t *src,
                        size_t count)
{
  DEBUG_STACK_FRAME;
  OSC_MEMMOVE(curr, src, count);
  curr += count;
}

/**
  Creates a state object that precisely matches the one
  that we expect to be generated as a result of applying the
  specified input (and other factors) to the indicated
  previously sent state.

  @param oldState The state (if any) that we expect the
                  remote endpoint to fetch.

  @param input    The input buffer that was passed in to be
                  compressed

  @param inputSize The size of the input buffer

  @param serial   The serial number that we will send as
                  part of this compressed message

  @param sentCaps The capabilites that we advertize in this message

  @todo This is a bit inefficient, especially in the way that we
        handle adding on the length/value table. We should
        tighten this up some when we get an opportunity.

  @retval 0 Out of memory error
*/

osc::State *
osc::DeflateCompressor::generateNewState(osc::State *oldState,
                                         const osc::byte_t *input,
                                         size_t inputSize,
                                         size_t bufferSize,
                                         osc::byte_t serial,
                                         osc::byte_t sentCaps)
{
  DEBUG_STACK_FRAME;
  //////////////////////////////////////////////////////////////////
  // Store history and other state info.

  // Compute the history buffer as seen by the UDVM
  osc::Buffer history;
  osc::byte_t *historyPtr = history.getMutableBuffer(bufferSize);
  if (!historyPtr)
  {
    return 0;
  }


  // Overlay the new message into the buffer (and update the bufferOffset)
  osc::u16 bufferOffset;

  if (oldState)
  {
    bufferOffset = oldState->getWord(DeflateBytecodes::DECOMPRESSED_POINTER)
                   - DeflateBytecodes::CIRCULAR_BUFFER;
  }
  else
  {
    bufferOffset = 0;
  }

  size_t i;

  // If the buffer consists exclusively of the input, then we
  // don't need to bother with retrieving the history.
  if (inputSize > bufferSize)
  {
    bufferOffset = (bufferOffset + (inputSize - bufferSize) ) % bufferSize;
    for (i = inputSize - bufferSize; i < inputSize; i++)
    {
      historyPtr[bufferOffset] = input[i];
      bufferOffset = (bufferOffset + 1) % bufferSize;
    }
  }

  // Otherwise, we need to fill in the history from the
  // previous state and fill in the rest with the input.
  else
  {
    // Zero the new buffer out
    OSC_MEMSET(historyPtr, 0, bufferSize);

    // Copy the old history into the buffer

    if (oldState)
    {
      const osc::byte_t *oldStateCircBuf = oldState->getStateDataRawBuffer() 
                                           + DeflateBytecodes::CIRCULAR_BUFFER 
                                           - oldState->getAddress();

      size_t copiedBytes = oldState->getStateSize()
                           - DeflateBytecodes::CIRCULAR_BUFFER 
                           + oldState->getAddress();

      if (copiedBytes > bufferSize)
      {
        copiedBytes = bufferSize;
      }
      memmove(historyPtr, oldStateCircBuf, copiedBytes);
    }

    for (i = 0; i < inputSize; i++)
    {
      historyPtr[bufferOffset] = input[i];
      bufferOffset = (bufferOffset + 1) % bufferSize;
    }
  }

  bufferOffset += DeflateBytecodes::CIRCULAR_BUFFER;

  // Generate the new state
  osc::State *state =
    new osc::State(DeflateBytecodes::STATE_ADDRESS,
                   DeflateBytecodes::DECOMPRESS_SIGCOMP_MESSAGE, 6);

  if (!state)
  {
    return 0;
  }

  osc::byte_t *curr = 
    state->getMutableBuffer(DeflateBytecodes::CIRCULAR_BUFFER + bufferSize 
                            - DeflateBytecodes::STATE_ADDRESS);

  if (!curr)
  {
    delete state;
    return 0;
  }

  // byte_copy_left
  appendWord(curr,DeflateBytecodes::CIRCULAR_BUFFER);

  // byte_copy_right
  appendWord(curr,DeflateBytecodes::CIRCULAR_BUFFER + bufferSize);

  // input_bit_order
  appendWord(curr,0);

  // decompressed_pointer
  appendWord(curr,bufferOffset);

  /* Length Table */
  appendWord(curr,0); appendWord(curr,3); appendWord(curr,0);
  appendWord(curr,4); appendWord(curr,0); appendWord(curr,5);
  appendWord(curr,0); appendWord(curr,6); appendWord(curr,0);
  appendWord(curr,7); appendWord(curr,0); appendWord(curr,8);
  appendWord(curr,0); appendWord(curr,9); appendWord(curr,0);
  appendWord(curr,10); appendWord(curr,1); appendWord(curr,11);
  appendWord(curr,1); appendWord(curr,13); appendWord(curr,1);
  appendWord(curr,15); appendWord(curr,1); appendWord(curr,17);
  appendWord(curr,2); appendWord(curr,19); appendWord(curr,2);
  appendWord(curr,23); appendWord(curr,2); appendWord(curr,27);
  appendWord(curr,2); appendWord(curr,31); appendWord(curr,3);
  appendWord(curr,35); appendWord(curr,3); appendWord(curr,43);
  appendWord(curr,3); appendWord(curr,51); appendWord(curr,3);
  appendWord(curr,59); appendWord(curr,4); appendWord(curr,67);
  appendWord(curr,4); appendWord(curr,83); appendWord(curr,4);
  appendWord(curr,99); appendWord(curr,4); appendWord(curr,115);
  appendWord(curr,5); appendWord(curr,131); appendWord(curr,5);
  appendWord(curr,163); appendWord(curr,5); appendWord(curr,195);
  appendWord(curr,5); appendWord(curr,227); appendWord(curr,0);
  appendWord(curr,258);

  /* Distance Table */
  appendWord(curr,0); appendWord(curr,1); appendWord(curr,0);
  appendWord(curr,2); appendWord(curr,0); appendWord(curr,3);
  appendWord(curr,0); appendWord(curr,4); appendWord(curr,1);
  appendWord(curr,5); appendWord(curr,1); appendWord(curr,7);
  appendWord(curr,2); appendWord(curr,9); appendWord(curr,2);
  appendWord(curr,13); appendWord(curr,3); appendWord(curr,17);
  appendWord(curr,3); appendWord(curr,25); appendWord(curr,4);
  appendWord(curr,33); appendWord(curr,4); appendWord(curr,49);
  appendWord(curr,5); appendWord(curr,65); appendWord(curr,5);
  appendWord(curr,97); appendWord(curr,6); appendWord(curr,129);
  appendWord(curr,6); appendWord(curr,193); appendWord(curr,7);
  appendWord(curr,257); appendWord(curr,7); appendWord(curr,385);
  appendWord(curr,8); appendWord(curr,513); appendWord(curr,8);
  appendWord(curr,769); appendWord(curr,9); appendWord(curr,1025);
  appendWord(curr,9); appendWord(curr,1537); appendWord(curr,10);
  appendWord(curr,2049); appendWord(curr,10); appendWord(curr,3073);
  appendWord(curr,11); appendWord(curr,4097); appendWord(curr,11);
  appendWord(curr,6145); appendWord(curr,12); appendWord(curr,8193);
  appendWord(curr,12); appendWord(curr,12289); appendWord(curr,13);
  appendWord(curr,16385); appendWord(curr,13); appendWord(curr,24577);

  appendWord(curr,0x0400 | serial);
  appendWord(curr, (sentCaps << 8) | 0x02 );
  if (m_stateHandler.hasSipDictionary())
  {
    osc::byte_t sipDictId[8] = {0x06, 0xFB, 0xE5, 0x07, 
                                0xDF, 0xE5, 0xE6, 0x00};
    appendBytes(curr,sipDictId, 8);
  }
  else
  {
    osc::byte_t zeros[8] = {0x00, 0x00, 0x00, 0x00, 
                            0x00, 0x00, 0x00, 0x00};
    appendBytes(curr,zeros, 8);
  }
  appendBytes(curr,DeflateBytecodes::getBytecodes(),
                   DeflateBytecodes::getSize());
  osc::byte_t zero = 0;
  appendBytes(curr,&zero, 1);
  appendBytes(curr, historyPtr, bufferSize);

  state->finalizeMutableBuffer();

  return state;
}

/**
  @todo Make output allowance more realistic. This should
        be 9/8ths of the input size, plus any overhead
        bits (and an additional byte for padding).

  @retval 0 Out of memory error
*/

osc::SigcompMessage *
osc::DeflateCompressor::compress (Compartment &compartment,
                                  const byte_t *input, size_t inputSize,
                                  bool reliableTransport)
{
  DEBUG_STACK_FRAME;
  size_t outputAllowance = inputSize + (inputSize/2);

  compartment.writeLock();

  int bufferSizeCode = (compartment.getRemoteCpbDmsSms() >> 3) & 0x07;

  //////////////////////////////////////////////////////////////////////
  // Determine the size of the Deflate buffer we'll be using
  //////////////////////////////////////////////////////////////////////

  // Reliable transport effectively halves how much stuff we can send.
  // Also, for remote DMS sizes of 2048, we chop the buffer down
  // to 512 bytes so that we have room for the actual SigComp message.
  if (reliableTransport || (bufferSizeCode == 1))
  {
    bufferSizeCode -= 1;
  }
 
  // Adjust for advertised state memory, if necessary. 
  if (bufferSizeCode >= (compartment.getRemoteCpbDmsSms() & 0x07))
  {
    bufferSizeCode = (compartment.getRemoteCpbDmsSms() & 0x07);
    if (bufferSizeCode == 0)
    {
      bufferSizeCode = 1;
    }
  }

  size_t bufferSize = (512 << bufferSizeCode);

  // Compute the largest message we can send on the wire.
  size_t maxSigcompMessageSize;
  if (reliableTransport)
  {
    maxSigcompMessageSize = compartment.getRemoteDecompressionMemorySize()/2;
  }
  else
  {
    maxSigcompMessageSize =
      compartment.getRemoteDecompressionMemorySize() - bufferSize
      - DeflateBytecodes::CIRCULAR_BUFFER;
  }


  //////////////////////////////////////////////////////////////////////
  // Initialize Deflate Dictionary
  //////////////////////////////////////////////////////////////////////
  osc::DeflateDictionary dictionary(bufferSize + inputSize);

  if (!dictionary.isValid())
  {
    compartment.unlock();
    return 0;
  }

  dictionary.setMaxDistance(bufferSize);

  osc::State *oldState = compartment.getMostRecentAckedState(0);

  // If we need to create a large buffer of nulls for the
  // dictionary to account for a change in our compression buffer
  // size, we'll store it in this variable.
  osc::byte_t *nulls = 0;

  if (oldState)
  {
    osc::u16 start = DeflateBytecodes::CIRCULAR_BUFFER - oldState->getAddress();
    osc::u16 curr  = oldState->getWord(DeflateBytecodes::DECOMPRESSED_POINTER) 
                     - oldState->getAddress();
    osc::u16 end   = oldState->getStateSize();
    const osc::byte_t *buffer = oldState->getStateDataRawBuffer();
    if (static_cast<size_t>(end - start) <= bufferSize)
    {
      // Add from curr to end of buffer
      if (!dictionary.addHistory(buffer + curr, end - curr))
      {
        oldState->release();
        compartment.unlock();
        return 0;
      }

      // If the buffer has grown, then we need to throw in enough
      // nulls to compensate for the larger buffer.
      if (static_cast<size_t>(end - start) < bufferSize)
      {
        size_t nullPadding = bufferSize - static_cast<size_t>(end - start);
        nulls = new osc::byte_t[nullPadding];
        if (!nulls)
        {
          oldState->release();
          compartment.unlock();
          return 0;
        }
        OSC_MEMSET(nulls, 0, nullPadding);
        if (!dictionary.addHistory(nulls, nullPadding))
        {
          oldState->release();
          compartment.unlock();
          delete [] nulls;
          return 0;
        }
      }
     
      // Add from start of buffer to curr - 1
      if (!dictionary.addHistory(buffer + start, curr - start))
      {
        oldState->release();
        compartment.unlock();
        delete [] nulls;
        return 0;
      }
    }
    else
    {
      // If the buffer has shrunk, then we pretend the previous
      // state doesn't exist any longer. This really should
      // never happen -- but other implementations are unpredictable,
      // and this is the only sure-fire way to recover under such
      // circumstances.
      oldState->release();
      oldState = 0;
    }
  }

  if (!dictionary.addFuture(input, inputSize))
  {
    if (oldState) { oldState->release(); }
    compartment.unlock();
    delete [] nulls;
    return 0;
  }

  //////////////////////////////////////////////////////////////////////
  // Retrieve or create the compressor data
  //////////////////////////////////////////////////////////////////////

  osc::DeflateData *data = reinterpret_cast<osc::DeflateData *> 
                           (compartment.getCompressorData(COMPRESSOR_ID));
  if (!data)
  {
    data = new osc::DeflateData();
    if (!data)
    {
      if (oldState)
      {
        oldState->release();
      }
      compartment.unlock();
      delete [] nulls;
      return 0;
    }

    bool status = compartment.addCompressorData(data);
    if (!status)
    {
      delete data;
      if (oldState)
      {
        oldState->release();
      }
      compartment.unlock();
      delete [] nulls;
      return 0;
    }
  }

  osc::byte_t sentCaps = 0;

  //////////////////////////////////////////////////////////////////////
  // Create the SigComp message that we'll be writing into
  //////////////////////////////////////////////////////////////////////
  osc::SigcompMessage *sm = 0;

  if (oldState)
  {
    sm = new osc::SigcompMessage(oldState->getStateId().data(), 9,
                                 outputAllowance);
  }
  else
  {
    sm = new osc::SigcompMessage(DeflateBytecodes::getBytecodes(),
                                 DeflateBytecodes::getSize(),
                                 DeflateBytecodes::getLocation(),
                                 outputAllowance);
  }

  if (!sm)
  {
    compartment.unlock();
    if (oldState)
    {
      oldState->release();
    }
    delete [] nulls;
    return 0;
  }

  osc::BitBuffer bitBuffer(sm->getInput(), outputAllowance,
                           0,osc::BitBuffer::MSB_FIRST);

  // If we're sending bytecodes, we must also include our local
  // capabilities.
  if (sm->getBytecodes())
  {
    bitBuffer.appendMsbFirst(8, m_stateHandler.getCpbDmsSms());
  }
  sentCaps = m_stateHandler.getCpbDmsSms();

  // Add flag: advertise SIP dictionary?
  bitBuffer.appendMsbFirst(1, m_stateHandler.hasSipDictionary()?1:0);


  // Add indicator: decompression buffer size
  bitBuffer.appendMsbFirst(3, bufferSizeCode);

  // Add 4-bit serial number (0 - 15)
  data->incrementSerial();
  bitBuffer.appendMsbFirst(4, data->getCurrStateSerial());

  //////////////////////////////////////////////////////////////////
  // Perform the actual compression
  //////////////////////////////////////////////////////////////////
  unsigned int length;
  unsigned int distance;

  while (!dictionary.isFinished())
  {
    if (dictionary.findNextLengthAndDistance(length, distance))
    {
      encodeLength(bitBuffer, length);
      encodeDistance(bitBuffer, distance);
      dictionary.increment(length);
    }
    else
    {
      // If no match is found in the history, output the byte
      encodeLiteral(bitBuffer, dictionary.getCurrent());
      dictionary.increment();
    }
  }

  delete [] nulls;

  bitBuffer.padOutputByte(1);
  sm->setInputLength(bitBuffer.getBufferSize()/8);

  //////////////////////////////////////////////////////////////////
  // Check to ensure we're not about to overflow the decompression
  // memory size on the remote stack.
  //////////////////////////////////////////////////////////////////
  if (sm->getDatagramLength() > maxSigcompMessageSize)
  {
    delete(sm);
    if (oldState) { oldState->release(); }
    compartment.unlock();
    return 0;
  }

  //////////////////////////////////////////////////////////////////
  // Figure out what the new state on the remote end should look like
  //////////////////////////////////////////////////////////////////
  osc::State *newState = generateNewState(oldState, input, inputSize,
                                          bufferSize,
                                          data->getCurrStateSerial(),
                                          sentCaps);
  if (oldState)
  {
    oldState->release();
  }

  //////////////////////////////////////////////////////////////////
  // Add the state to this compartment and the global map.
  //////////////////////////////////////////////////////////////////
  if (!newState)
  {
    delete(sm);
    compartment.unlock();
    return 0;
  }

  newState->retain();
  newState = m_stateHandler.addState(newState);

  //////////////////////////////////////////////////////////////////
  // Mark the message as ACKed, if apropriate
  //----------------------------------------------------------------
  // If the message is sent over a reliable tranport
  // *or* the remote endpoint supports NACKs, then
  // we assume that this message will make it there
  // before the next message we send.
  //////////////////////////////////////////////////////////////////
  bool acked = reliableTransport;

  if (compartment.getRemoteSigcompVersion() >= 2)
  {
    acked = true;
  }

  compartment.addRemoteState(newState, 0, acked);
  data->storeCurrStateId(newState->getStateId());
  newState->release();

  compartment.unlock();

  return sm;
}

/**
  Allows deflate compressor to update necessary state based
  on feedback generated by the execution of our bytecodes
  in the remote endpoint.  This method is called by the
  stack whenever the application accepts a received message.

  @note The caller of this method must ensure the compartment
        is retained and locked for writing.
*/
void
osc::DeflateCompressor::handleFeedback(osc::Compartment &compartment)
{
  DEBUG_STACK_FRAME;

  // Retrieve or create the compressor data
  osc::DeflateData *data = reinterpret_cast<osc::DeflateData *> 
                           (compartment.getCompressorData(COMPRESSOR_ID));
  if (!data)
  {
    return;
  }

  const osc::Buffer &feedback = compartment.getReturnedFeedback();

  if (feedback.size() != 1 || feedback[0] >= 16)
  {
    return;
  }
  const Buffer &stateId = data->getStateId(feedback[0]);

  compartment.ackRemoteState(stateId);
}


void
osc::DeflateCompressor::storeNackInfo(osc::sha1_t &sha1,
                                      osc::Compartment &compartment)
{
  DEBUG_STACK_FRAME;
  compartment.writeLock();

  // Retrieve or create the compressor data
  osc::DeflateData *data = reinterpret_cast<osc::DeflateData *> 
                           (compartment.getCompressorData(COMPRESSOR_ID));
  if (!data)
  {
    compartment.unlock();
    return;
  }

  data->storeCurrNackHash(sha1);

  compartment.unlock();
}

void
osc::DeflateCompressor::handleNack(osc::SigcompMessage &nack,
                                   osc::Compartment &compartment)
{
  DEBUG_STACK_FRAME;
  compartment.writeLock();

  // Retrieve or create the compressor data
  osc::DeflateData *data = reinterpret_cast<osc::DeflateData *> 
                           (compartment.getCompressorData(COMPRESSOR_ID));
  if (!data)
  {
    compartment.unlock();
    return;
  }

  signed int index;

  // Remove any state or states that the NACKed message was
  // expected to create
  while ((index = data->findNackHash(nack.getNackSha1())) >= 0)
  {
    compartment.removeRemoteState(data->getStateId(index));
  }

  compartment.unlock();
}
