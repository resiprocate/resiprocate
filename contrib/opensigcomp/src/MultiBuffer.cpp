/* ***********************************************************************
   Open SigComp -- Implementation of RFC 3320 Signaling Compression

   Copyright 2006 Estacado Systems, LLC

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
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   license for additional details.

   To discuss alternate licensing terms, contact info@estacado.net
 *********************************************************************** */

/**
  @file MultiBuffer.cpp
  @brief Implementation of osc::MultiBuffer class.
*/



#include "ProfileStack.h"
#include <assert.h>
#include "Libc.h"
#include "MultiBuffer.h"
#include "Buffer.h"

#ifdef OPTIMIZE_SIZE
/**
  Constructor for osc::MultiBuffer.
 */
osc::MultiBuffer::MultiBuffer(size_t sizeHint) : 
  m_size(0)
{
  DEBUG_STACK_FRAME;
  m_buffer.size.block = (0);
  m_buffer.size.numBlocks = (0);
  m_buffer.size.maxBlocks = (4); 
  m_buffer.size.block = new block_t[m_buffer.size.maxBlocks];
  if (!m_buffer.size.block)
  {
    m_buffer.size.maxBlocks = 0;
  }
}

/**
  Destructor for osc::MultiBuffer.
 */
osc::MultiBuffer::~MultiBuffer()
{
  DEBUG_STACK_FRAME;
  delete [] m_buffer.size.block;
}


/**
  @retval true Success
  @retval false Out of memory
 */

bool
osc::MultiBuffer::addBlock(const osc::byte_t *start, 
                           size_t size)
{
  DEBUG_STACK_FRAME;
  if (size == 0)
  {
    return true;
  }
  if (m_buffer.size.numBlocks == m_buffer.size.maxBlocks)
  {
    block_t *tmp = m_buffer.size.block;
    m_buffer.size.block = new block_t[m_buffer.size.maxBlocks * 2];
    if (!m_buffer.size.block)
    {
      m_buffer.size.block = tmp;
      return false;
    }
    OSC_MEMMOVE(m_buffer.size.block, tmp,
                m_buffer.size.maxBlocks * sizeof(block_t));
    m_buffer.size.maxBlocks *= 2;
    delete[] tmp;
  }

  m_buffer.size.block[m_buffer.size.numBlocks].start = start;
  m_buffer.size.block[m_buffer.size.numBlocks].size = size;
  m_buffer.size.numBlocks++;
  m_size += size;
  return true;
}

osc::byte_t
osc::MultiBuffer::operator [](size_t index) const
{
  DEBUG_STACK_FRAME;
  size_t offset = 0;
  size_t curr = 0;
  while (curr < m_buffer.size.numBlocks && 
         index - offset >= m_buffer.size.block[curr].size)
  {
    offset += m_buffer.size.block[curr].size;
    curr++;
  }
  return m_buffer.size.block[curr].start[index-offset];
}

#else 

/*
  The following are a speed-optimized implementation of the MultiBuffer.
  Instead of referring to the various blocks of memory in-place, this
  implementation copies the various blocks into a single, contiguous
  buffer. This is somewhat wasteful of space, but it is much faster.
 */

osc::MultiBuffer::MultiBuffer(size_t sizeHint) :
  m_size(0)
{
  DEBUG_STACK_FRAME;
  m_buffer.speed.bufferCapacity = sizeHint;
  m_buffer.speed.buffer = new osc::byte_t[m_buffer.speed.bufferCapacity];
  if (!m_buffer.speed.buffer)
  {
    m_buffer.speed.bufferCapacity = 0;
  }
}

osc::MultiBuffer::~MultiBuffer()
{
  DEBUG_STACK_FRAME;
  delete [] m_buffer.speed.buffer;
}

bool
osc::MultiBuffer::addBlock(const osc::byte_t *start, size_t size)
{
  DEBUG_STACK_FRAME;
  if (size == 0)
  {
    return true;
  }
  if (m_buffer.speed.bufferCapacity == 0)
  {
    return false;
  }

  // If there's no more room left, we allocate additional capacity
  if (m_size + size > m_buffer.speed.bufferCapacity)
  {
    size_t newCapacity = m_buffer.speed.bufferCapacity * 2;
    if (m_size + size > newCapacity)
    {
      newCapacity += size;
    }
    osc::byte_t *newBuffer = new osc::byte_t[newCapacity];
    if (!newBuffer)
    {
      return false;
    }
    OSC_MEMMOVE(newBuffer, m_buffer.speed.buffer, m_size);
    m_buffer.speed.bufferCapacity = newCapacity;
    delete[] m_buffer.speed.buffer;
    m_buffer.speed.buffer = newBuffer;
  }

  OSC_MEMMOVE(m_buffer.speed.buffer + m_size, start, size);
  m_size += size;
  return true;
}

osc::MultiBuffer::operator const osc::byte_t*() const
{
  return m_buffer.speed.buffer;
}

#endif

/**
  Copy constructor for osc::MultiBuffer.
 */
osc::MultiBuffer::MultiBuffer(MultiBuffer const &r)
{
  DEBUG_STACK_FRAME;
  /* Assign attributes */
  assert(0);
}

/**
  Assignment operator for osc::MultiBuffer.
 */
osc::MultiBuffer &
osc::MultiBuffer::operator=(MultiBuffer const &r)
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
