#ifndef __OSC__BIT_BUFFER
#define __OSC__BIT_BUFFER 1

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
  @file BitBuffer.h
  @brief Header file for osc::BitBuffer class.
*/


#include "Types.h"

namespace osc
{
  /**
    Used by the UDVM to read input and write output in a bit-oriented fashion.

    The BitBuffer operates on an underlying buffer of bytes that it does
    not own. When it is created, the constructor takes a pointer to the
    buffer on which it operates along with the length of that buffer. The
    constructor also takes an argument that specifies the order in
    which the bits in the bytes of that buffer are to be processed
    (most significant bit first, or least significant bit first). Once
    the BitBuffer is constructed, this order cannot be changed unless
    padOutputByte() is first called to ensure that the output is on an
    even byte boundary.  For example, consider a BitBuffer set to pack
    Least-Significant-Bit first, operating on a three-byte buffer,
    as shown in Figure 1.

    @image html bitbuffer1.gif "Figure 1: Empty Three-Byte BitBuffer"

    Now, assume that the bits "1", "1", "0", and "1"
    are added (in that order). After such an addition, the underlying
    buffer would look as shown in Figure 2

    @image html bitbuffer2.gif "Figure 2: LSB-First Three-Byte BitBuffer Containing 4 Bits"

    After adding the bits "0", "0", "0", "1", "1",
    to the same buffer, it would look as shown in Figure 3.

    @image html bitbuffer3.gif "Figure 3: LSB-First Three-Byte BitBuffer Containing 9 Bits"

    By contrast, if the BitBuffer had been instantiated
    Most-Significant-Bit first, it would look as shown in Figure 4 after
    the same 9 bits have been added.

    @image html bitbuffer4.gif "Figure 4: MSB-First Three-Byte BitBuffer Containing 9 Bits"

    Reading of bits from a BitBuffer has precisely the same ordering
    considerations as writing to it.

    In practice, up to 16 bits can be written to or read from a BitBuffer
    at a time; such bits are represented as a single 16-bit integer in
    the interface to this class. If fewer than 16 bits are to be written
    or read to the BitBuffer, only the least significant bits are used,
    regardless of the order those bits are to be interpreted. Calls to
    the methods "appendLsbFirst" and "readLsbFirst" are to interpret the
    bits so that the least significant bit is read or written first. Calls
    to the methods "appendMsbFirst" and "readMsbFirst" are to interpret
    the bits so that the most significant bit is read or written first.

    For example, consider a 3 byte BitBuffer, to which you are adding
    11 bits, represented as the 16-bit integer "0x0326." appendLsbFirst
    called on a LSB oriented BitBuffer will copy the bits as shown in
    Figure 5; appendMsbFirst called on the same buffer will copy the
    bits as shown in Figure 6. Similarly, the same two operations on
    a MSB oriented BitBuffer will copy bits as shown in Figure 7 and
    Figure 8 respectively.

    @image html bitbuffer5.gif "Figure 5: appendLsbFirst into LSB-First Buffer"


    @image html bitbuffer6.gif "Figure 6: appendMsbFirst into LSB-First Buffer"


    @image html bitbuffer7.gif "Figure 7: appendLsbFirst into MSB-First Buffer"


    @image html bitbuffer8.gif "Figure 8: appendMsbFirst into MSB-First Buffer"

    As before, the order in which bits are processed is the same for
    reading from the BitBuffer as it is for writing to it.
  */

  class BitBuffer
  {
    public:
      enum packOrder_t
      {
        MSB_FIRST,
        LSB_FIRST
      };

      BitBuffer(byte_t buffer[] = 0,
                size_t bufferSize = 0,
                int currBits = 0,
                packOrder_t packOrder = MSB_FIRST, 
                bool readOnly = false);

      ~BitBuffer();
 
      BitBuffer * operator &(){ return this; }
      BitBuffer const * operator &() const { return this; }

      void reset(byte_t buffer[],
                 size_t bufferSize,
                 int currBits = 0,
                 packOrder_t packOrder = MSB_FIRST, 
                 bool readOnly = false);

      bool appendLsbFirst(int length, u16 value);
      bool appendMsbFirst(int length, u16 value);
      bool appendBytes(osc::byte_t*, size_t);
      osc::byte_t *appendBytes(size_t);

      u16 readLsbFirst(int length);
      u16 readMsbFirst(int length);
      bool readBytes(osc::byte_t*, size_t);
      osc::byte_t *readBytes(size_t);

      size_t getBufferSize();
      void padOutputByte(bool bit = 0);
      void discardPartialInputByte();

      packOrder_t getPackOrder() {return m_packOrder;}
      bool setPackOrder(packOrder_t);

    protected:

    private:
      /* if you define these, move them to public */
      BitBuffer(BitBuffer const &);
      BitBuffer& operator=(BitBuffer const &);

    private:
      /**
        @note This class DOES NOT OWN the memory in m_buffer.
       */
      byte_t *m_buffer;

      // Size of the buffer IN BYTES
      size_t m_bufferSize;

      size_t m_startByte;
      size_t m_endByte;
      unsigned int m_startBit:3;
      unsigned int m_endBit:3;

      packOrder_t m_packOrder;
      bool m_readOnly;
  };
}

#endif
