#ifndef __OSC__TCP_STREAM
#define __OSC__TCP_STREAM 1

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
  @file TcpStream.h
  @brief Header file for osc::TcpStream class.
*/


#include "Types.h"
#include "Buffer.h"
#include "SigcompMessage.h"

namespace osc
{
  class MultiBuffer;

  /**
    Tracks information that has arrived on a single TCP stream.

    The TcpStream class is a rolling buffer that queues the data received
    from the network for a specific TCP connection.  It performs framing
    and un-escaping for messages received from the network. Because of
    the nature of stream protocols, the TcpStream class may contain
    multiple and/or fractional SigComp messages. In general, after
    adding data to a TcpStream object, the application should attempt
    to decompress messages from the TcpStream until the call to
    osc::Stack::uncompressMessage returns 0.
  */

  class TcpStream
  {
    public:
      typedef enum
      {
        OK,
        FRAMING_ERROR,
        OUT_OF_MEMORY
      }
      status_t;

      TcpStream();
      ~TcpStream();
 
      TcpStream * operator &(){ return this; }
      TcpStream const * operator &() const { return this; }

      status_t addData(const void *data, size_t);
      osc::SigcompMessage *getNextMessage();

    protected:
      /**
        Node for creating a non-invasive singly-linked list of
        SigComp messages read from a TCP stream.
      */
      struct Frame
      {
        osc::SigcompMessage *message;
        Frame *next;
      };

      Frame *m_headFrame;
      Frame *m_tailFrame;

      osc::Buffer partialFrame;

    private:
      status_t addMessageToFrameList(const osc::MultiBuffer &mb,
                                 size_t start, size_t end,
                                 size_t resultSize);

      /* if you define these, move them to public */
      TcpStream(TcpStream const &);
      TcpStream& operator=(TcpStream const &);
  };
}

#endif
