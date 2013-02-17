#ifndef __OSC__STACK
#define __OSC__STACK 1

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
  @file Stack.h
  @brief Header file for osc::Stack class.
*/


#include "Types.h"
#include "Udvm.h"

namespace osc
{
  class Compressor;
  class StateChanges;
  class SigcompMessage;
  class TcpStream;
  class StateHandler;

  /**
    The main interface between the application and the OpenSigcomp library.

    Upon startup, the application will generally create a single
    StateHandler instance. It then creates one Stack instance per
    thread. Finally, the application creates and adds one or more
    Compressors to each Stack. These compressors are added by the
    application in order of descending priority. For the current
    iteration, when a message is compressed, the stack will execute
    the compressors in order until one succeeds in compressing the
    message. Future iterations may include more complex mechanisms for
    determining the order in which compressors are executed, and which
    output is ultimately used.

    To compress a message, the application calls "compressMessage"
    on the Stack. This method includes a parameter that identifies
    the compartment to which the message corresponds. Upon successful
    compression, the stack returns a SigComp message suitable to be
    sent to a peer. Note that the SigComp stack will always return a
    SigComp message, even if none of the compressors succeeded.

    Upon receipt of a compressed datagram (UDP) message, the application
    will invoke the method "uncompressMessage(byte_t*)" on the stack. If
    the decompression is successful, this method returns a decompressed
    message plus a "StateChanges" object. This "StateChanges" object
    contains a list of state additions and deletions that have been
    requested by the bytecodes. The "uncompressMessage(byte_t*)" method
    will re-use a single UDVM that has been allocated by the Stack upon
    its creation.

    When an application creates a TCP connection for receiving compressed
    messages, it also creates a corresponding "TcpStream" object. This
    "TcpStream" object will contain any information that has been read
    from a TCP connection. (In future iterations, the TcpStream objects
    may also each contain their own UDVMs, so as to allow decompression
    of partially received messages). Whenever the application reads data
    from the TCP connection, it calls "addData" on the TcpStream. The
    application then calls "uncompressMessage(TcpStream&)" on the stack
    until the stack indicates that no further information is available.

    For both UDP and TCP, the application examines the decompressed
    message. If it finds it to be valid, it calls "provideCompartmentId"
    on the Stack; this method takes the compartment ID associated with
    the decompressed message and the "StateChanges" object that was
    returned from the earlier call to "uncompressMessage." The stack
    then updates the StateManager according to the instructions in the
    "StateChanges" object. If the application decides that the message
    is invalid, it simply destroys the StateChanges object.

    If either of the "uncompressMessage" methods return 0 bytes, the
    application is expected to call "getNack" on the stack. If an error
    has occurred during decompression, then this method returns a SigComp
    message which must be sent to the same address and port from which
    the corresponding compressed message was received.
  */


  class Stack
  {
    public:
      Stack(osc::StateHandler &);
      ~Stack();
 
      Stack * operator &(){ return this; }
      Stack const * operator &() const { return this; }

      void addCompressor(osc::Compressor *);

      size_t uncompressMessage(const void *input, size_t inputSize,
                               void *output, size_t outputSize,
                               osc::StateChanges *&);

      size_t uncompressMessage(osc::TcpStream &,
                               void *output, size_t outputSize,
                               osc::StateChanges *&);

      osc::SigcompMessage *getNack();

      void provideCompartmentId(osc::StateChanges *, 
                                const void *id, size_t idLength);


      osc::SigcompMessage *compressMessage(const void *, size_t,
                                           const void *id, size_t idLength,
                                           bool reliableTransport = false);

      bool closeCompartment(const void *id, size_t idLength);

#ifndef NO_TEMPLATES
      template <typename T>
        void provideCompartmentId(osc::StateChanges *, const T& id);

      template <typename T> 
        osc::SigcompMessage *compressMessage(const void *, size_t, 
                                             const T &id,
                                             bool reliableTransport = false);
      template <typename T>
        bool closeCompartment(const T& id);

#else
        void provideCompartmentId(osc::StateChanges *, int id);

        osc::SigcompMessage *compressMessage(const void *, size_t, 
                                             int id,
                                             bool reliableTransport = false);

        bool closeCompartment(int id);
#endif

      osc::nack_code_t getStatus() { return m_udvm.getNackCode(); }

    protected:
      enum {MAX_COMPRESSORS=8};

      size_t uncompressMessage(osc::SigcompMessage &,
                               void *output, size_t outputSize,
                               osc::StateChanges *&, 
                               bool stream);

    private:
      void handleNack(osc::SigcompMessage &);

      osc::StateHandler &m_stateHandler;
      count_t m_numCompressors;
      osc::Compressor* m_compressorList[MAX_COMPRESSORS];
      osc::Udvm m_udvm;
      osc::SigcompMessage *m_nack;

      /* if you define these, move them to public */
      Stack(Stack const &);
      Stack& operator=(Stack const &);
  };
}

#ifndef NO_TEMPLATES
/**
  Convenience method to provide compartment ID when compartment IDs
  are native types.

  @note It must be valid to perform byte-wise comparisons on the
        passed in IDs for this method to be valid.
 */
template <typename T> void 
osc::Stack::provideCompartmentId(osc::StateChanges *sc, const T& id)
{
  provideCompartmentId(sc, &id, sizeof(id));
}

/**
  Convenience method to compress a message when compartment IDs
  are native types.

  @note It must be valid to perform byte-wise comparisons on the
        passed in IDs for this method to be valid.
 */
template <typename T> osc::SigcompMessage *
osc::Stack::compressMessage(const void *m, size_t s, const T& id, bool r)
{
  return compressMessage(m, s, &id, sizeof(id), r);
}

/**
  Convenience method to close a compartment when compartment IDs
  are native types.

  @note It must be valid to perform byte-wise comparisons on the
        passed in IDs for this method to be valid.
 */
template <typename T> bool 
osc::Stack::closeCompartment(const T& id)
{
  return closeCompartment(&id, sizeof(id));
}

#else
/**
  Convenience method to provide compartment ID when compartment IDs
  are integers.
 */
inline
void 
osc::Stack::provideCompartmentId(osc::StateChanges *sc, int id)
{
  provideCompartmentId(sc, &id, sizeof(id));
}

/**
  Convenience method to compress a message when compartment IDs
  are integers.
 */
inline
osc::SigcompMessage *
osc::Stack::compressMessage(const void *m, size_t s, int id, bool r)
{
  return compressMessage(m, s, &id, sizeof(id), r);
}

/**
  Convenience method to close a compartment when compartment IDs
  are integers.
 */
inline
bool 
osc::Stack::closeCompartment(int id)
{
  return closeCompartment(&id, sizeof(id));
}
#endif

#endif
