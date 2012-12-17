#ifndef __OSC__DEFLATE_COMPRESSOR
#define __OSC__DEFLATE_COMPRESSOR 1

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
  @file DeflateCompressor.h
  @brief Header file for osc::DeflateCompressor class.
*/


#include "Compressor.h"
#include "Types.h"

namespace osc
{
  class BitBuffer;
  class StateHandler;
  class State;

  /**
    A SigComp compressor based on the Deflate algorithm.

    This compressor is based on Deflate as described in RFC 1951.
    In particular, it implements block type "01" (compressed blocks with
    a fixed Huffman table). Handling of transparent blocks and blocks
    with variable Huffman tables is not performed.

    Users of the SigComp stack do not need to access any methods of the
    DeflateCompressor class except for the constructor.
  */

  class DeflateCompressor : public Compressor
  {
    public:
      enum {COMPRESSOR_ID = 0x4445464C};

      DeflateCompressor(osc::StateHandler &);
      virtual ~DeflateCompressor();
 
      DeflateCompressor * operator &(){ return this; }
      DeflateCompressor const * operator &() const { return this; }

      virtual SigcompMessage *compress (Compartment &compartment,
                                        const byte_t *input, size_t inputSize,
                                        bool reliableTransport);

      virtual void handleFeedback(Compartment &compartment);
      virtual void storeNackInfo(osc::sha1_t &, osc::Compartment &);
      virtual void handleNack(osc::SigcompMessage &nack, osc::Compartment &);

    protected:
      bool encodeLiteral(BitBuffer &buffer, byte_t symbol);
      bool encodeLength(BitBuffer &buffer, int length);
      bool encodeDistance(BitBuffer &buffer, int distance);

      /**
        Structure used by the DeflateCompressor to create a mapping
        from length and distance values to pre-packed bitfields.
      */
      typedef struct
      {
        u8 bits;
        u16 value;
      }
      lookup_table_t;

      static lookup_table_t c_literalTable[];
      static lookup_table_t c_lengthTable[];

    private:

      osc::State *generateNewState(osc::State *oldState,
                                   const osc::byte_t *input,
                                   size_t inputSize,
                                   size_t bufferSize,
                                   osc::byte_t serial,
                                   osc::byte_t sentCaps);
                                   

      /* if you define these, move them to public */
      DeflateCompressor(DeflateCompressor const &);
      DeflateCompressor& operator=(DeflateCompressor const &);
  };
}

#endif
