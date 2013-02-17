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
  @file DeflateDictionary.cpp
  @brief Implementation of osc::DeflateDictionary class.
*/



#include "ProfileStack.h"
#include <assert.h>
#include "Libc.h"
#include "DeflateDictionary.h"

/**
  Constructor for osc::DeflateDictionary.
 */
osc::DeflateDictionary::DeflateDictionary(size_t sizeHint)
 : m_data(sizeHint), m_curr(0), m_maxDistance(32768), m_indexNext(0)
{
  DEBUG_STACK_FRAME;

#ifdef OPTIMIZE_SIZE
  m_indexHead = new osc::u16[128];
  // Set all indices to 0xFFFF (65535)
  OSC_MEMSET(m_indexHead, 0xFF, 128 * sizeof(osc::u16));
#else
  m_indexHead = new osc::u16[65536];
  // Set all indices to 0xFFFF (65535)
  OSC_MEMSET(m_indexHead, 0xFF, 65536 * sizeof(osc::u16));
#endif

}

/**
  Copy constructor for osc::DeflateDictionary.
 */
osc::DeflateDictionary::DeflateDictionary(DeflateDictionary const &r)
{
  DEBUG_STACK_FRAME;
  /* Assign attributes */
  assert(0);
}

/**
  Destructor for osc::DeflateDictionary.
 */
osc::DeflateDictionary::~DeflateDictionary()
{
  DEBUG_STACK_FRAME;
  delete [] m_indexNext;
  delete [] m_indexHead;
}

/*
  @retval true Success
  @retval false Out of memory
*/
bool
osc::DeflateDictionary::addHistory(const byte_t *buffer, size_t size)
{
  DEBUG_STACK_FRAME;
  if (size == 0)
  {
    return true;
  }

  if (!m_data.addBlock(buffer, size))
  {
    return false;
  }
  m_curr += size;

  // It is illegal to call this once "addFuture" has been called.
  assert(m_curr == m_data.getSize());

  return true;
}


/*
  @retval true Success
  @retval false Out of memory
*/
bool
osc::DeflateDictionary::addFuture(const byte_t *buffer, size_t size)
{
  DEBUG_STACK_FRAME;
  if (!m_data.addBlock(buffer, size))
  {
    return false;
  }
  return addIndex();
}

bool
osc::DeflateDictionary::findNextLengthAndDistance(unsigned int &length, 
                                                  unsigned int &distance)
{
  DEBUG_STACK_FRAME;
  unsigned int j;
  osc::u16 i;

  length = 0;
  distance = 0;

  // Lengths shorter than 3 never get compressed.  
  if ((m_curr < 3) || (m_curr + 3 >= getSize()))
  {
    return false;
  }

#ifdef OPTIMIZE_SIZE
  i = m_indexHead[m_data[m_curr] & 0x7F];
#else
#  ifdef UNALIGNED_U16_OKAY
  i = m_indexHead[*(reinterpret_cast<const osc::u16*>(m_data+m_curr))];
#  else
  i = m_indexHead[(m_data[m_curr] << 8) | (m_data[m_curr + 1])];
#  endif
#endif

  for (; i != 65535; i = m_indexNext[i])
  {
    if (m_curr - i > m_maxDistance)
    {
      break;
    }

#ifdef OPTIMIZE_SIZE
    j = 0;
#else
    j = 2;
#endif
    while (m_curr+j < getSize() && m_data[i+j] == m_data[m_curr+j] && j < 258)
    {
      j++;
    }
    if (j > length)
    {
      length = j;
      distance = m_curr - i;
      if (j == 258)
      {
        return true;
      }
    }
  }

  // Gotta find a length of 3 or better, or there's just no
  // compression to be had.
  if (length >= 3)
  {
    return true;
  }
  return false;
}

void osc::DeflateDictionary::increment (unsigned int size)
{
  DEBUG_STACK_FRAME;
  size_t newSize = m_curr + size;

  if (newSize + 3 > getSize())
  {
    m_curr = newSize;
    return;
  }

  // Add the new character(s) to the index
#ifdef OPTIMIZE_SIZE
  osc::byte_t byte;
  for (; m_curr < newSize; m_curr++)
  {
    byte = m_data[m_curr] & 0x7F;
    m_indexNext[m_curr] = m_indexHead[byte];
    m_indexHead[byte] = m_curr;
  }
#else
  osc::u16 bytes;
  for (; m_curr < newSize; m_curr++)
  {
#  ifdef UNALIGNED_U16_OKAY
    bytes = *(reinterpret_cast<const osc::u16*>(m_data+m_curr));
#  else
    bytes = (m_data[m_curr] << 8) | (m_data[m_curr+1]);
#  endif
    m_indexNext[m_curr] = m_indexHead[bytes];
    m_indexHead[bytes] = m_curr;
  }
#endif
}

/**
  Assignment operator for osc::DeflateDictionary. Don't use this.
 */
osc::DeflateDictionary &
osc::DeflateDictionary::operator=(DeflateDictionary const &r)
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

/*
  @retval true Success
  @retval false Out of memory
*/
bool
osc::DeflateDictionary::addIndex()
{
  DEBUG_STACK_FRAME;
  if (m_indexNext)
  {
    delete[] m_indexNext;
  }
  m_indexNext = new u16[getSize()];
  if (!m_indexNext)
  {
    return false;
  }

  size_t temp = m_curr;
  m_curr = 0;
  increment(temp);
  return true;
}
