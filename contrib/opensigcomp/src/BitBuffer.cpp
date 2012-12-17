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
  @file BitBuffer.cpp
  @brief Implementation of osc::BitBuffer class.
*/


#include "ProfileStack.h"
#include "Libc.h"
#include <assert.h>
#include "BitBuffer.h"

/**
  Constructor for osc::BitBuffer.
 */
osc::BitBuffer::BitBuffer(byte_t buffer[],
                          size_t bufferSize,
                          int currBits,
                          packOrder_t packOrder, 
                          bool readOnly)
{
  DEBUG_STACK_FRAME;
  reset(buffer, bufferSize, currBits, packOrder, readOnly);
}

/**
  Copy constructor for osc::BitBuffer.
 */
osc::BitBuffer::BitBuffer(BitBuffer const &r)
{
  DEBUG_STACK_FRAME;
  /* Assign attributes */
  assert(0);
}

/**
  Destructor for osc::BitBuffer.
 */
osc::BitBuffer::~BitBuffer()
{
  DEBUG_STACK_FRAME;
}

void
osc::BitBuffer::reset(byte_t buffer[],
                      size_t bufferSize,
                      int currBits,
                      packOrder_t packOrder,
                      bool readOnly)
{
  DEBUG_STACK_FRAME;
  m_readOnly = readOnly;
  m_buffer = (buffer);
  m_bufferSize = (bufferSize);
  m_startByte = (0);
  m_endByte = (currBits / 8);
  m_startBit = (0);
  m_endBit = (currBits % 8);
  m_packOrder = (packOrder);

  if (!m_readOnly && buffer && (bufferSize != 0))
  {
    if (m_bufferSize - m_endByte > 0)
    {
      // Zero out unused trailing bytes
      OSC_MEMSET(buffer + m_endByte + 1, 0, m_bufferSize - m_endByte - 1);
    }

    // Zero out unused bits of final byte
    if (m_packOrder == LSB_FIRST)
    {
      m_buffer[m_endByte] &= (1 << m_endBit) - 1;
    }
    else
    {
      m_buffer[m_endByte] &= ~((1 << (8-m_endBit)) - 1);
    }
  }
}

/**
  Assignment operator for osc::BitBuffer.
 */
osc::BitBuffer &
osc::BitBuffer::operator=(BitBuffer const &r)
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
osc::BitBuffer::appendLsbFirst(int length, u16 value)
{
  DEBUG_STACK_FRAME;
  if (m_readOnly) 
  {
    return false;
  }

  if(m_packOrder == LSB_FIRST)
  {
    value &= ((1 << length) - 1);

    int emptyBitsInByte;
    while (length)
    {
      if (m_endByte == m_bufferSize)
      {
        return false;
      }

      emptyBitsInByte = 8 - m_endBit;

      if (length >= emptyBitsInByte)
      {
        m_buffer[m_endByte] |= (value <<  m_endBit) & 0xff;
        value >>= emptyBitsInByte;
        length -= emptyBitsInByte;
        m_endBit = 0;
        m_endByte++;
      }
      else
      {
        m_buffer[m_endByte] |= (value << m_endBit);
        m_endBit += length;
        length = 0;
      }
    }
  }

  else /* MSB_FIRST */
  {
    u16 mask = 0x01;

    while(length)
    {
      if (m_endByte == m_bufferSize)
      {
        return false;
      }

      if (value & mask)
      {
        m_buffer[m_endByte] |= (0x80 >> m_endBit);
      }

      if (m_endBit == 7)
      {
        m_endBit = 0;
        m_endByte ++;
      }
      else
      {
        m_endBit++;
      }

      mask <<= 1;
      length--;
    }
  }

  return true;
}

bool 
osc::BitBuffer::appendMsbFirst(int length, u16 value)
{
  DEBUG_STACK_FRAME;
  if (m_readOnly) 
  {
    return false;
  }

  if(m_packOrder == LSB_FIRST)
  {
    u16 mask = 0x01 << (length-1);

    while(mask)
    {
      if (m_endByte == m_bufferSize)
      {
        return false;
      }

      if (value & mask)
      {
        m_buffer[m_endByte] |= (0x01 << m_endBit);
      }

      if (m_endBit == 7)
      {
        m_endBit = 0;
        m_endByte ++;
      }
      else
      {
        m_endBit++;
      }

      mask >>= 1;
    }
  }

  else /* MSB_FIRST */
  {
    value &= ((1 << length) - 1);

    int emptyBitsInByte;
    while (length)
    {
      if (m_endByte == m_bufferSize)
      {
        return false;
      }

      emptyBitsInByte = 8 - m_endBit;

      if (length >= emptyBitsInByte)
      {
        m_buffer[m_endByte] |= (value >> (length - emptyBitsInByte)) &
                               ((1 << emptyBitsInByte) - 1);

        length -= emptyBitsInByte;
        m_endBit = 0;
        m_endByte++;
      }
      else
      {
        m_buffer[m_endByte] |= (value << (emptyBitsInByte - length));
        m_endBit += length;
        length = 0;
      }
    }

  }

  return true;
}

bool
osc::BitBuffer::appendBytes(osc::byte_t *buffer, size_t size)
{
  DEBUG_STACK_FRAME;
  if (m_readOnly) 
  {
    return false;
  }

  if (m_endBit != 0)
  {
    return false;
  }
  if (m_endByte + size >= m_bufferSize)
  {
    return false;
  }

  OSC_MEMMOVE(m_buffer + m_endByte, buffer, size);
  m_endByte += size;

  return true;
}

osc::byte_t *
osc::BitBuffer::appendBytes(size_t size)
{
  DEBUG_STACK_FRAME;
  if (m_readOnly) 
  {
    return 0;
  }

  if (m_endBit != 0)
  {
    return 0;
  }
  if (m_endByte + size >= m_bufferSize)
  {
    return 0;
  }

  osc::byte_t* retval = m_buffer + m_endByte;
  m_endByte += size;

  return retval;
}

osc::u16 
osc::BitBuffer::readLsbFirst(int length)
{
  DEBUG_STACK_FRAME;
  int retval = 0;
  if(m_packOrder == LSB_FIRST)
  {
    int bitsLeftInByte;
    int offset = 0;
    while (length)
    {
      bitsLeftInByte = 8 - m_startBit;
      if (length >= bitsLeftInByte)
      {
        retval |= (m_buffer[m_startByte] >> m_startBit) << offset;
        offset += bitsLeftInByte;
        length -= bitsLeftInByte;
        m_startBit=0;
        m_startByte++;
        if (m_startByte > m_endByte)
        {
          return retval;
        }
      }
      else
      {
        retval |= ((m_buffer[m_startByte] >> m_startBit)
                   & ((1 << length ) - 1)) << offset;
        m_startBit += length;
        length = 0;
      }
    }

  }

  else /* MSB_FIRST */
  {
    u16 mask = 0x01;
    while (length)
    {
      if (m_buffer[m_startByte] & (0x80 >> m_startBit))
      {
        retval |= mask;
      }

      if (m_startBit == 7)
      {
        m_startBit = 0;
        m_startByte++;
        if (m_startByte > m_endByte)
        {
          return retval;
        }
      }
      else
      {
        m_startBit++;
      }

      length--;
      mask <<= 1;
    }
  }
  return retval;
}

osc::u16 
osc::BitBuffer::readMsbFirst(int length)
{
  DEBUG_STACK_FRAME;
  int retval = 0;
  if(m_packOrder == LSB_FIRST)
  {
    while (length)
    {
      retval <<= 1;
      if (m_buffer[m_startByte] & (0x01 << m_startBit))
      {
        retval |= 1;
      }
      if (m_startBit == 7)
      {
        m_startBit = 0;
        m_startByte++;
        if (m_startByte > m_endByte)
        {
          return retval;
        }
      }
      else
      {
        m_startBit++;
      }
      length--;
    }
  }
  else /* MSB_FIRST */
  {
    int bitsLeftInByte;
    while (length)
    {
      bitsLeftInByte = 8 - m_startBit;
      if (length >= bitsLeftInByte)
      {
        retval <<= bitsLeftInByte;
        retval |= m_buffer[m_startByte] & ((1 << bitsLeftInByte) - 1);
        length -= bitsLeftInByte;
        m_startBit = 0;
        m_startByte++;
        if (m_startByte > m_endByte)
        {
          return retval;
        }
      }
      else
      {
        retval <<= length;
        retval |= m_buffer[m_startByte] >> (bitsLeftInByte - length)
                  & ((1 << length) - 1);
        m_startBit += length;
        length = 0;
      }
    }
  }
  return retval;
}

bool
osc::BitBuffer::readBytes(osc::byte_t *buffer, size_t size)
{
  DEBUG_STACK_FRAME;
  if (m_startBit != 0)
  {
    return false;
  }
  if (m_startByte + size > m_endByte)
  {
    return false;
  }

  OSC_MEMMOVE(buffer, m_buffer + m_startByte, size);
  m_startByte += size;

  return true;
}

osc::byte_t *
osc::BitBuffer::readBytes(size_t size)
{
  DEBUG_STACK_FRAME;
  if (m_startBit != 0)
  {
    return 0;
  }
  if (m_startByte + size > m_endByte)
  {
    return 0;
  }

  osc::byte_t* retval = m_buffer+m_startByte;
  m_startByte += size;

  return retval;
}

/**
  Returns the number of bits currently in the buffer
*/
size_t 
osc::BitBuffer::getBufferSize()
{
  DEBUG_STACK_FRAME;
  int size = ((m_endByte - m_startByte) * 8) - m_startBit + m_endBit;
  if (size < 0)
  {
    size = 0;
  }
  return size;
}

void 
osc::BitBuffer::padOutputByte(bool bit)
{
  DEBUG_STACK_FRAME;
  if (m_endBit == 0)
  {
    return;
  }

  if (bit)
  {
    if (m_packOrder == LSB_FIRST)
    {
      m_buffer[m_endByte] |= ~((1 << (m_endBit)) - 1);
    }
    else
    {
      m_buffer[m_endByte] |= (1 << (8-m_endBit)) - 1;
    }
  }

  m_endByte++;
  m_endBit = 0;
}

void
osc::BitBuffer::discardPartialInputByte()
{
  DEBUG_STACK_FRAME;
  if (m_startBit != 0)
  {
    m_startByte++;
    m_startBit = 0;
  }
}

bool
osc::BitBuffer::setPackOrder(osc::BitBuffer::packOrder_t packOrder)
{
  DEBUG_STACK_FRAME;
  if (m_packOrder == packOrder)
  {
    return true;
  }

  if (m_startBit != 0 || m_endBit != 0)
  {
    return false;
  }

  m_packOrder = packOrder;
  return true;
}
