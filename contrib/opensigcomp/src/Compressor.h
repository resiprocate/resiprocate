#ifndef __OSC__COMPRESSOR
#define __OSC__COMPRESSOR 1

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
  @file Compressor.h
  @brief Header file for osc::Compressor class.
*/


#include "Types.h"

namespace osc
{
  class Compartment;
  class StateHandler;
  class SigcompMessage;

  /**
    Pure virtual base class for all compressors.

    Compressor is an interface that defines how the Stack interfaces
    to the various compression techniques. It has one important method,
    predictably named "compress." This method takes in the message
    to be compressed along with the Compartment associated with that
    message. The compressor will use this Compartment to store and
    retrieve information associated with the remote endpoint.

    It is possible that descendants of the Compressor class may contain
    methods for modifying the operation of the compressor themselves.

    All classes derived from Compressor have the following
    responsibilities:

    @li Retrieving old state(s), if any, from the Compartment.

    @li Compressing the provided message.

    @li Including the bytecodes or state identifier necessary to
        decompress the compressed message.
  */

  class Compressor
  {
    public:
      Compressor(osc::StateHandler &);
      virtual ~Compressor();
 
      Compressor * operator &(){ return this; }
      Compressor const * operator &() const { return this; }

      virtual SigcompMessage *compress (Compartment &compartment,
                                        const byte_t *input, size_t inputSize,
                                        bool reliableTransport) = 0;

      /// @note This method assumes the compartment is write-locked
      virtual void handleFeedback(Compartment &compartment) {;}

      virtual void storeNackInfo(osc::sha1_t &, osc::Compartment &) {;}
      virtual void handleNack(osc::SigcompMessage &nack, osc::Compartment &) {;}

    protected:
      Compressor(Compressor const &);

      osc::StateHandler &m_stateHandler;

    private:
      /* if you define these, move them to public */
      Compressor& operator=(Compressor const &);

    private:
  };
} 

#endif
