#ifndef OSC_BUFFER_H
#define OSC_BUFFER_H 1

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
  @file Buffer.h
  @brief Header file for osc::Buffer class.
*/



#include "Types.h"
#include "Libc.h"

#ifdef DEBUG
#include <ostream>
#endif

namespace osc
{
  /**
    A convenience class for encapsulating buffers of bytes.

    This class provides for simplistic ownership management
    and manipulation of buffers of bytes. Its primary purpose
    is to avoid the need to pass around pointers and length
    values whenever byte buffers are used.
  */
  class Buffer
  {
    public:
      Buffer();
      Buffer(const osc::byte_t *start, size_t size, bool ownBytes = true);
      Buffer(osc::Buffer const &buffer);
      ~Buffer();

      size_t copy(const osc::byte_t *start, size_t size);
      size_t copy(osc::Buffer const &source);
      size_t copy(osc::Buffer const & buffer, size_t const start, 
                  size_t const length);
      size_t subsume(osc::Buffer &source);
      size_t free();
      size_t allocate(size_t const size);

      osc::byte_t& operator [](size_t const index);
      osc::byte_t operator [](size_t const index) const;

      osc::Buffer& operator = (osc::Buffer const &buffer);
      
      bool operator < (osc::Buffer const &buffer) const;
      
      bool operator == (osc::Buffer const &buffer) const;
      
      /// returns the length of the buffer
      size_t size() const { return m_size; }

      /// returns a pointer to the start of the buffer
      const osc::byte_t *data() const { return m_bytes; }

      osc::byte_t *getMutableBuffer(size_t size);
      
      void getSha1Digest(osc::Buffer &sha1Buffer) const;
      osc::u32 getLookup2Hash(size_t len=0,osc::u32 state=0) const;

#ifdef DEBUG
      void dump(std::ostream &) const;
#endif

      void truncate (size_t newSize);

    protected:

      osc::byte_t *m_bytes;
      size_t       m_size;
      bool         m_ownBytes;

    private:
      static void mix(osc::u32 &a, osc::u32 &b, osc::u32 &c);
      void deleteBytes();
      void allocateBytes(size_t size);
  };

#ifdef DEBUG
  std::ostream& operator<< (std::ostream &, const osc::Buffer &);
#endif
}
#endif
