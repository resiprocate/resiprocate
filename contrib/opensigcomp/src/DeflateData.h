#ifndef __OSC__DEFLATE_DATA
#define __OSC__DEFLATE_DATA 1

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
  @file DeflateData.h
  @brief Header file for osc::DeflateData class.
*/


#include "CompressorData.h"
#include "Buffer.h"

namespace osc
{
  /**
    Per-compartment state information used by the DeflateCompressor. 

    This class tracks the 16 most recently created states for a given
    compartment (subject to the size limitations imposed by the remote
    endpoint). It also keeps track of NACK hashes so that a state failure
    can be used to remove the proper state.

    @see CompressorData
  */

  class DeflateData : public CompressorData
  {
    private:
      enum {MAP_SIZE = 16};

    public:
      DeflateData();
      virtual ~DeflateData();
 
      DeflateData * operator &(){ return this; }
      DeflateData const * operator &() const { return this; }

      virtual compressor_id_t getDataType()
        { return osc::DeflateCompressor::COMPRESSOR_ID; }

      osc::byte_t getCurrStateSerial() const { return m_nextState; }

      void storeCurrStateId(const osc::Buffer&);
      void storeCurrNackHash(const osc::sha1_t&);
      void incrementSerial();

      signed int findNackHash(const osc::sha1_t &);

      const osc::Buffer& getStateId(osc::u8 serial) const;

#ifdef DEBUG
      void dump(std::ostream &) const;
#endif

    private:

      osc::state_id_t m_stateId[MAP_SIZE];
      osc::u32 m_nackHash[MAP_SIZE];
      osc::u8 m_nextState:4;

      /* if you define these, move them to public */
      DeflateData(DeflateData const &);
      DeflateData& operator=(DeflateData const &);
  };
}

#endif
