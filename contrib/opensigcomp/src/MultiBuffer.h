#ifndef __OSC__MULTI_BUFFER
#define __OSC__MULTI_BUFFER 1

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
  @file MultiBuffer.h
  @brief Header file for osc::MultiBuffer class.
*/


#include "Buffer.h"
#include "Types.h"
#include "Libc.h"

namespace osc
{
  /**
    Stitches together a series of byte arrays so that
    they can be accessed as if they were one contiguous array.

    @note MultiBuffer never takes ownership of any of the
          byte arrays that it stitches together. It is the
          user's responsibility to ensure that (a) the arrays
          outlive the MultiBuffer, and (b) the arrays are
          deallocated when they are no longer useful.
  */

  class MultiBuffer
  {
    public:
      MultiBuffer(size_t sizeHint=8192);
      ~MultiBuffer();
 
      MultiBuffer * operator &(){ return this; }
      MultiBuffer const * operator &() const { return this; }

      bool addBlock(const osc::byte_t *, size_t);
      bool addBlock(const osc::Buffer &);
#ifdef OPTIMIZE_SIZE
      osc::byte_t operator[](size_t index) const;
#else
      operator const osc::byte_t*() const;
#endif
      size_t getSize() const { return m_size; }

    protected:

    private:
      /* if you define these, move them to public */
      MultiBuffer(MultiBuffer const &);
      MultiBuffer& operator=(MultiBuffer const &);

      typedef struct
      {
        const osc::byte_t *start;
        size_t size;
      } block_t;

      union
      {
        struct
        {
          block_t * block;
          count_t   numBlocks;
          count_t   maxBlocks;
        } size;

        struct
        {
          osc::byte_t *buffer;
          size_t       bufferCapacity;
        } speed;
      } m_buffer;

      size_t    m_size;
  };
}

/**
  @retval true Success
  @retval false Out of memory
 */
inline bool
osc::MultiBuffer::addBlock(const osc::Buffer &buffer)
{
  return addBlock(buffer.data(), buffer.size());
}

#endif
