#ifndef __OSC__COMPRESSOR_DATA
#define __OSC__COMPRESSOR_DATA 1

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
  @file CompressorData.h
  @brief Header file for osc::CompressorData class.
*/


#include "Types.h"
#include "DeflateCompressor.h"
 
#ifdef DEBUG
#include <ostream>
#endif

namespace osc
{
  /**
    Interface for objects used to store compressor-specific state information.

    The per-compartment information stored by a compressor is stored in
    a list of CompressorData objects. The base CompressorData objects
    contain only one method -- a pure virtual method -- called
    "getDataType."  This method returns a 32-bit integer that serves as
    an identifier for the type of CompressorData (e.g. Deflate versus
    Lzjh). While this could generally be accomplished using RTTI,
    the constraints of some embedded environments make the use of RTTI
    impossible or undesirable; consequently, we use our own scheme. The
    32-bit integer should generally contain an ASCII representation
    that is an abbreviation of the compression scheme. For example,
    the value returned by DeflateData is 0x4445464C ("DEFL"), while the
    value returned by LzjhData is 0x4C5A4A48 ("LZJH").
  */

  class CompressorData
  {
    public:
      CompressorData();
      virtual ~CompressorData();
 
      CompressorData * operator &(){ return this; }
      CompressorData const * operator &() const { return this; }

      virtual compressor_id_t getDataType() = 0;

#ifdef DEBUG
      virtual void dump(std::ostream &) const = 0;
#endif

    protected:

    private:
      /* if you define these, move them to public */
      CompressorData(CompressorData const &);
      CompressorData& operator=(CompressorData const &);
  };

#ifdef DEBUG
  std::ostream& operator<< (std::ostream &, const osc::CompressorData &);
#endif
}

#endif
