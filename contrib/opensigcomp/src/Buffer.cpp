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
  @file Buffer.cpp
  @brief Implementation of osc::Buffer class.
*/


#include "ProfileStack.h"
#include <assert.h>
#include "Libc.h"
#include "Buffer.h"
#include "Types.h"
#include "Sha1Hasher.h"

#ifdef DEBUG
#include <iomanip>
#endif

inline void
osc::Buffer::deleteBytes()
{
  if (!m_ownBytes)
  {
    return;
  }
#ifdef HAVE_REALLOC
  if (m_bytes)
  {
    OSC_FREE(m_bytes);
  }
#else
  delete [] m_bytes;
#endif
}

inline void 
osc::Buffer::allocateBytes(size_t size)
{
  m_ownBytes = true;
#ifdef HAVE_REALLOC
  m_bytes = reinterpret_cast<osc::byte_t *>
            (OSC_MALLOC(size*sizeof(osc::byte_t)));
#else
  m_bytes = new osc::byte_t[size];
#endif
}


/**
  Default constructor: creates an empty buffer.
*/
osc::Buffer::Buffer()
  : m_bytes(0), m_size(0), m_ownBytes(true)
{
  DEBUG_STACK_FRAME;
}

/**
  Builds a buffer based on an existing byte array allocated
  from somewhere.

  @param start Pointer to the character array from which this
               buffer is to be generated.

  @param size  Size of the character array from which this
               buffer is to be generated.

  @param ownBytes Flag indicating whether the buffer is to own the
                  bytes (in which case the byte array is copied),
                  or whether the caller is to continue to own the
                  byte array. This can be set to true only when
                  there is some guarantee that the byte array
                  will outlive the buffer.

  @todo This constructor violates the "const" promise on the byte array
        when ownBytes is false. Ideally, such would not be the case.
*/

osc::Buffer::Buffer(const osc::byte_t *start, size_t size, bool ownBytes)
  : m_bytes(0), m_size(size), m_ownBytes(ownBytes)
{
  DEBUG_STACK_FRAME;
  if (size == 0)
  {
    return;
  }

  if (!m_ownBytes)
  {
    m_bytes = const_cast<osc::byte_t *>(start);
    return;
  }

  allocateBytes(size);
  if (m_bytes)
  {
    if (start)
    {
      OSC_MEMMOVE (m_bytes, start, size);
    }
  }
  else
  {
    m_size = 0;
  }
}

/**
 This constructor copies the contents of another Buffer into the new Buffer

 @param buffer Buffer to be copied
*/
osc::Buffer::Buffer(osc::Buffer const &buffer)
  : m_bytes(0), m_size(buffer.m_size), m_ownBytes(true)
{
  DEBUG_STACK_FRAME;
  if (m_size == 0)
  {
    return;
  }

  allocateBytes(m_size);
  if (m_bytes)
  {
    OSC_MEMMOVE (m_bytes, buffer.m_bytes, m_size);
  }
  else
  {
    m_size = 0;
  }
}

/**
 Destructor that deallocates the memory used by the buffer.
*/
osc::Buffer::~Buffer()
{
  DEBUG_STACK_FRAME;
  deleteBytes();
}

/**
  Reassigns the buffer's value to be taken from the indicated
  byte buffer.

  @param start Pointer to the character array from which this
               buffer is to be generated.

  @param size  Size of the character array from which this
               buffer is to be generated.

*/
size_t
osc::Buffer::copy(const osc::byte_t *start, size_t size)
{
  DEBUG_STACK_FRAME;
  if (size == 0)
  {
    deleteBytes();
    m_bytes = 0;
    m_size = 0;
    return 0;
  }

#ifdef HAVE_REALLOC
  if (size < m_size && m_ownBytes)
  {
    void *tmp = OSC_REALLOC(m_bytes, size);
    if (tmp)
    {
      m_bytes = reinterpret_cast<osc::byte_t *>(tmp);
      m_size = size;
    }
  }
#endif

  if (size != m_size)
  {
    deleteBytes();
    m_size = size;
    allocateBytes(size);
  }

  if (m_bytes)
  {
    OSC_MEMMOVE (m_bytes, start, m_size);
  }
  else
  {
    m_size = 0;
  }
  return size;
}

/**
  Copies the source Buffer's contents into this Buffer after deallocating 
  this Buffer's current buffer.

  @param source Buffer to be copied.

  @returns the number of bytes copied
*/
size_t osc::Buffer::copy(osc::Buffer const &source)
{
  DEBUG_STACK_FRAME;
  return copy(source.m_bytes, source.m_size);
}

/** 
  This method will copy a portion of another Buffer to this Buffer which 
  will have its internal buffer deallocated.

  @param buffer the Buffer to copy from
  @param start the index to start copying from
  @param length the length of the buffer to copy

  @returns the size of the buffer after the new allocation
*/      
size_t osc::Buffer::copy(osc::Buffer const & buffer,
                         size_t const start,
                         size_t const length)
{
  DEBUG_STACK_FRAME;
  if (start + length > buffer.m_size)
  {
    return 0;
  }
  copy(buffer.m_bytes + start, length);
  return length;
}

/**
  Takes ownership of the bytes of the indicated buffer.  The buffer that
  is passed in is reset to contain zero bytes.

  @param source Buffer whose contents are to be subsumed.
*/
size_t osc::Buffer::subsume(osc::Buffer &source)
{
  DEBUG_STACK_FRAME;
  deleteBytes();

  m_bytes = source.m_bytes;
  m_size = source.m_size;
  m_ownBytes = source.m_ownBytes;

  source.m_bytes = 0;
  source.m_size = 0;
  source.m_ownBytes = true;

  return m_size;
}

/**
  Deallocates the internal buffer, resetting this buffer to be empty.

  @returns the number of bytes deallocated
*/
size_t osc::Buffer::free()
{
  DEBUG_STACK_FRAME;
  size_t size = m_size;
  deleteBytes();
  m_bytes = 0;
  m_size = 0;
  return size;
}

/**
  Operator for direct access to a single byte in the buffer.

  @param index the index to access
  @returns the byte at the indicated index
*/
osc::byte_t& osc::Buffer::operator [](size_t const index)
{
  DEBUG_STACK_FRAME;
  return m_bytes[index];
}

/**
  Operator for direct read-only access to a single byte in the buffer.

  @param index the index to access
  @returns the byte at the indicated index
*/
osc::byte_t osc::Buffer::operator [](size_t const index) const
{
  DEBUG_STACK_FRAME;
  return m_bytes[index];
}

/**
  Copies a the contents of another Buffer after deallocating the old
  internal buffer

  @param  buffer The buffer to copy
  @returns this buffer
*/
osc::Buffer& osc::Buffer::operator = (osc::Buffer const &buffer)
{
  DEBUG_STACK_FRAME;
  copy(buffer.m_bytes, buffer.m_size);
  return *this;
}

/**
  Lexocographically compares this buffer to another buffer.

  @param  buffer the buffer evaluate against

  @retval true Both buffers are empty or if byte m of this buffer
               is less than byte m of the the other buffer, where n is
               the size of the smaller buffer, m<=n, 0<=i=<m, each byte
               m-i of both buffers is equivalent, and n > 0.
*/

bool osc::Buffer::operator < (osc::Buffer const &buffer) const
{
  DEBUG_STACK_FRAME;
  size_t smaller = (m_size < buffer.m_size) ? m_size : buffer.m_size;

  size_t pos = 0;

  while (pos < smaller)
  {
    if (m_bytes[pos] != buffer.m_bytes[pos])
    {
      return m_bytes[pos] < buffer.m_bytes[pos];
    }
    pos++;
  }

  // If they're the same up to the end of the shorter string,
  // but are different lengths, then we consider the shorter
  // string to be lexocographically smaller.
  return (size() < buffer.size());
}

/**
  Checks to see if the buffers are identical.

  @param  buffer the buffer evaluate against
*/

bool osc::Buffer::operator == (osc::Buffer const &buffer) const
{
  DEBUG_STACK_FRAME;
  if (m_size != buffer.m_size)
  {
    return false;
  }
  for (size_t i = 0; i < m_size; i++)
  {
    if (m_bytes[i] != buffer.m_bytes[i])
    {
      return false;
    }
  }
  return true;
}

/**
  This is a very special purpose method that reallocates the
  buffer to the size indicated and then returns a mutable
  pointer to that new buffer.
*/
osc::byte_t *
osc::Buffer::getMutableBuffer(size_t size)
{
  DEBUG_STACK_FRAME;
  if (size == 0)
  {
    deleteBytes();
    m_bytes = 0;
    m_size = 0;
    return 0;
  }

  if (size != m_size)
  {
    deleteBytes();
    m_size = size;
    allocateBytes(size);
    if (!m_bytes)
    {
      m_size = 0;
    }
  }

  return m_bytes;
}

/**
  Shortens the buffer to the indicated size by removing an
  appropriate number of characters from the end.

  @param size The new size for the buffer
*/

void
osc::Buffer::truncate(size_t size)
{
  DEBUG_STACK_FRAME;
  if (size >= m_size)
  {
    return;
  }

  if (size == 0)
  {
    free();
    return;
  }

  if (m_ownBytes)
  {
#ifdef HAVE_REALLOC
    void *tmp = OSC_REALLOC(m_bytes, size);
    if (tmp)
    {
      m_size = size;
      m_bytes = reinterpret_cast<osc::byte_t *>(tmp);
    }
#else
    osc::byte_t *old = m_bytes;
    allocateBytes(size);
    if (!m_bytes)
    {
      // Allocation has failed. Point back to the old buffer,
      // but consider it the new size.
      m_bytes = old;
      m_size = size;
      return;
    }
    m_size = size;
    OSC_MEMMOVE(m_bytes, old, size);
#endif
  }
  else
  {
    m_size = size;
  }
}

/**
  Generates a SHA-1 digest based on the contents of this buffer.
  @param sha1Buffer the buffer to place the completed digest in
*/

void osc::Buffer::getSha1Digest(osc::Buffer &sha1Buffer) const
{
  DEBUG_STACK_FRAME;
  osc::Sha1Hasher hasher;
  hasher.addData(m_bytes, m_size);
  hasher.getHash(sha1Buffer.getMutableBuffer(20));
}

/**
  Perform the bit mixing described on 
  http://burtleburtle.net/bob/hash/evahash.html.

  This code was pulled out of Buffer::getLookup2Hash so that it can be
  made a function under gcc -Os, but is inlined (a la the original macro)
  for other situations.
*/
inline void
osc::Buffer::mix (osc::u32 &a, osc::u32 &b, osc::u32 &c)
{
  DEBUG_STACK_FRAME;
  a = a-b;  a = a-c;  a = a^(c>>13);
  b = b-c;  b = b-a;  b = b^(a<<8);
  c = c-a;  c = c-b;  c = c^(b>>13);
  a = a-b;  a = a-c;  a = a^(c>>12);
  b = b-c;  b = b-a;  b = b^(a<<16);
  c = c-a;  c = c-b;  c = c^(b>>5);
  a = a-b;  a = a-c;  a = a^(c>>3);
  b = b-c;  b = b-a;  b = b^(a<<10);
  c = c-a;  c = c-b;  c = c^(b>>15);
}

/**
  @param length The maximum number of bytes to hash.  If set to 0 it
                will return a hash over all bytes in the buffer

  @returns  the 32bit LOOKUP2 hash

  @see http://burtleburtle.net/bob/hash/evahash.html for a complete
       description. 
*/

osc::u32 osc::Buffer::getLookup2Hash(size_t length, osc::u32 state) const
{
  DEBUG_STACK_FRAME;
  //Ensure a valid length
  if( length > m_size || length == 0 ) 
  {
    length = m_size;
  }
  size_t remaining = length;
  osc::u32 a, b = 0x5461436F;
  a = b;
  osc::u32 c = state;
  osc::byte_t* k = m_bytes;
  
  //Do repetitive processing as long as the buffer is "large"
  while(remaining >= 12)
  {
    //Load a,b,c with values from buffer
    a = a +             k[0] 
          + (((osc::u32)k[1])<<8) 
          + (((osc::u32)k[2])<<16)
          + (((osc::u32)k[3])<<24);

    b = b +             k[4] 
          + (((osc::u32)k[5])<<8) 
          + (((osc::u32)k[6])<<16) 
          + (((osc::u32)k[7])<<24);

    c = c +             k[8] 
          + (((osc::u32)k[9])<<8) 
          + (((osc::u32)k[10])<<16) 
          + (((osc::u32)k[11])<<24);

    //Hippy bitmixing
    mix(a,b,c);

    //Advance the input pointer 12 bytes
    k = k + 12;
    remaining = remaining - 12;
  }

  c = c + length;
  //Now that the remaining number of bytes is small:
  switch ( remaining )
  {
    //Stuff a,b, and c
    case 11: c = c + (((osc::u32)(k[10])) << 24);
    case 10: c = c + (((osc::u32)(k[9])) << 16);
    case 9:  c = c + (((osc::u32)(k[8])) << 8);
    case 8:  b = b + (((osc::u32)(k[7])) << 24);
    case 7:  b = b + (((osc::u32)(k[6])) << 16);
    case 6:  b = b + (((osc::u32)(k[5])) << 8);
    case 5:  b = b + k[4];
    case 4:  a = a + (((osc::u32)(k[3])) << 24);
    case 3:  a = a + (((osc::u32)(k[2])) << 16);
    case 2:  a = a + (((osc::u32)(k[1])) << 8);
    case 1:  a = a + k[0];

  }

  //More bitmixing
  mix(a,b,c);

  return c;
}

#ifdef DEBUG
/**
  Inserts a hexadecimal representation of the buffer into the
  indicated stream (debugging only).
*/

void
osc::Buffer::dump(std::ostream &os) const
{
  DEBUG_STACK_FRAME;
  std::ios::fmtflags flags = os.flags();
  char fill = os.fill();
  os << std::hex << std::setfill('0') << std::noshowbase;
  for (size_t i=0; i < m_size; i++)
  {
    os << std::setw(2) << static_cast <unsigned int>(m_bytes[i]) << " ";
  }
  os.fill(fill);
  os.flags(flags);
}

/**
  Streaming operator for displaying Buffer objects (debugging only).
*/
std::ostream &
osc::operator<< (std::ostream &os, const osc::Buffer &b)
{
  DEBUG_STACK_FRAME;
  b.dump(os);
  return os;
}
#endif
