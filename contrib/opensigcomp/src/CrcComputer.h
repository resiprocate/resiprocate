#ifndef __OSC__CRC_COMPUTER
#define __OSC__CRC_COMPUTER 1

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
  @file CrcComputer.h
  @brief Header file for osc::CrcComputer class.
*/


#include "Types.h"

namespace osc
{
  /**
    Implements RFC 1662 16-bit CRC calculation.

    CrcComputer is used by the UDVM to implement the CRC opcode.
    The algorithm used to calculate this CRC is defined by RFC 1662
 
    Users of the CrcComputer simply instantiate an instance of the
    CrcComputer, call "addData" (perhaps repeatedly, if more than
    one buffer is to be checksummed), and then call "getCrc" to
    retrieve the 16-bit CRC of the data.

    @note The implementation of this class is based on
          the code in RFC 1662
  */

  class CrcComputer
  {
    public:
      CrcComputer();
      ~CrcComputer();

      void addData(const byte_t *buffer, size_t size);
      u16 getCrc();
      void reset();
 
      CrcComputer * operator &(){ return this; }
      CrcComputer const * operator &() const { return this; }

    protected:

    private:
      /* if you define these, move them to public */
      CrcComputer(CrcComputer const &);
      CrcComputer& operator=(CrcComputer const &);

      osc::u16 m_crc;

      static osc::u16 s_table[256];
  };
}

#endif
