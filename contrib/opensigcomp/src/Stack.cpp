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
  @file Stack.cpp
  @brief Implementation of osc::Stack class.
*/



#include "ProfileStack.h"
#include <assert.h>
#include "Stack.h"
#include "Compressor.h"
#include "Compartment.h"
#include "StateChanges.h"
#include "SigcompMessage.h"
#include "TcpStream.h"
#include "StateHandler.h"
#include "BitBuffer.h"
#include "NackCodes.h"

/**
  Constructor for osc::Stack.

  @param stateHandler  Reference to state handler that this
                       stack uses to store and retreive
                       compression-related state. A single
                       state handler may be shared among several
                       Stacks.
 */
osc::Stack::Stack(osc::StateHandler &stateHandler)
  : m_stateHandler(stateHandler), 
    m_numCompressors(0),
    m_udvm(stateHandler, stateHandler.getDecompressionMemorySize()),
    m_nack(0)
{
  DEBUG_STACK_FRAME;
}

/**
  Copy constructor for osc::Stack.
 */
osc::Stack::Stack(Stack const &r)
  : m_stateHandler(r.m_stateHandler),
    m_udvm(r.m_stateHandler, 0)
{
  DEBUG_STACK_FRAME;
  /* Assign attributes */
  assert(0);
}

/**
  Destructor for osc::Stack.
 */
osc::Stack::~Stack()
{
  DEBUG_STACK_FRAME;
  for (osc::count_t i = 0; i < m_numCompressors; i++)
  {
    delete m_compressorList[i];
  }
  delete m_nack;
}

/**
  Assignment operator for osc::Stack.
 */
osc::Stack &
osc::Stack::operator=(Stack const &r)
{
  DEBUG_STACK_FRAME;
  if (&r == this)
  {
    return *this;
  }
  /* Assign attributes */
  assert(0);
  return *this;
}

/**
  Adds the specified compressor to the stack. Compressors
  are invoked in the order in which they are added. The
  first compressor that can create valid output for the
  message is used for the message. In almost all cases,
  the first compressor added will be able to compress the
  message.

  It is not possible to remove compressors from the stack
  once they have been added.

  After the application adds a compressor to the stack, the
  application must not reference the compressor again. If
  multiple stacks are constructed, each must have its own
  compressor instance(s).

  @param compressor  Pointer to the compressor to be added.
                     The stack takes ownership of the compressor.

  @note The stack takes ownership of the compressor after it
        has been added.
 */
void
osc::Stack::addCompressor(osc::Compressor * compressor)
{
  DEBUG_STACK_FRAME;
  if (m_numCompressors < MAX_COMPRESSORS)
  {
    m_compressorList[m_numCompressors++] = compressor;
  }
  else
  {
    delete compressor;
  }
}

/**
  Decompresses a datagram-oriented SigComp message.

  @param input        Pointer to a buffer containing a compressed
                      SigComp message.

  @param inputSize    Number of bytes present in the input buffer.

  @param output       Pointer to a buffer into which the uncompressed
                      application message is to be placed.

  @param outputSize   Number of bytes available in output buffer.
                      The amount of output generated will never
                      exceed 65535 bytes in size.

  @param stateChanges Reference to a pointer into which the stack
                      will write a pointer to a new StateChanges
                      object. The application will use this StateChanges
                      object to provide a compartment ID once it verifies
                      the uncompressed message. (See provideCompartmentId).
                      If the application does not call provideCompartmentId 
                      with this StateChanges object, then it is responsible
                      for deleting it.

  @returns            Number of bytes written into the output buffer
                      (i.e. size of the decompressed message).

  @retval 0           An error has occurred during decompression.
 */

size_t
osc::Stack::uncompressMessage(const void *input, size_t inputSize,
                              void *output, size_t outputSize,
                              osc::StateChanges *& stateChanges)
{
  DEBUG_STACK_FRAME;
  osc::SigcompMessage sm(reinterpret_cast<const osc::byte_t *>(input),
                         inputSize);
  return uncompressMessage(sm, output, outputSize, stateChanges, false);
}

/**
  Decompresses a stream-oriented SigComp message.

  @param tcpStream    Reference to a TcpStream object from which
                      the compressed messages are to be extracted.

  @param output       Pointer to a buffer into which the uncompressed
                      application message is to be placed.

  @param outputSize   Number of bytes available in output buffer.
                      The amount of output generated will never
                      exceed 65535 bytes in size.

  @param stateChanges Reference to a pointer into which the stack
                      will write a pointer to a new StateChanges
                      object. The application will use this StateChanges
                      object to provide a compartment ID once it verifies
                      the uncompressed message. (See provideCompartmentId).
                      If the application does not call provideCompartmentId 
                      with this StateChanges object, then it is responsible
                      for deleting it.

  @returns            Number of bytes written into the output buffer
                      (i.e. size of the decompressed message).

  @retval 0           An error has occurred during decompression or
                      there is not sufficient data in the TcpStream
                      object to comprise a full SigComp message.
 */
size_t
osc::Stack::uncompressMessage(osc::TcpStream &s,
                              void *output, size_t outputSize,
                              osc::StateChanges *&stateChanges)
{
  DEBUG_STACK_FRAME;
  size_t retval=0;
  osc::SigcompMessage *sm = s.getNextMessage();
  if (sm)
  {
    retval = uncompressMessage(*sm, output, outputSize, stateChanges, true);
    delete sm;
  }
  return retval;
}

/**
  Creates a SigComp NACK message if the previous attempt to
  decompress a message failed.
  
  @returns   A pointer to a SigComp message which the application
             is to send to the address from which the most recent
             SigComp message was received. The caller owns this
             object and is responsbile for destroying it.

  @retval  0  No error has occurred, so no NACK message has been
              generated.

  @see RFC 4077
 */

osc::SigcompMessage *
osc::Stack::getNack()
{
  DEBUG_STACK_FRAME;
  SigcompMessage *tmp = m_nack;
  m_nack = 0;
  return tmp;
}

/**
  Provides a compartment id for a decompressed message.

  @param stateChanges  Pointer to the StateChanges object
                       that was handed to the application
                       as part of the uncompressMessage
                       operation. The Stack takes ownership
                       of this object.

  @param id            Pointer to the compartment identifier
                       for the compartment with which the
                       StateChanges are to be associated.
                       Compartment identifiers are chosen
                       unilaterally by the application.

  @param idLength      Size of the compartment identifier,
                       in bytes.
 */
void
osc::Stack::provideCompartmentId(osc::StateChanges *stateChanges,
                                 const void *id, size_t idLength)
{
  DEBUG_STACK_FRAME;
  assert(id != 0);
  assert(idLength != 0);

  if (!stateChanges)
  {
    return;
  }

  osc::Compartment *compartment = 
    m_stateHandler.getCompartment(osc::compartment_id_t(
                       reinterpret_cast<const osc::byte_t*>(id), 
                       idLength));

  if (!compartment)
  {
    return;
  }

  compartment->writeLock();
  m_stateHandler.processChanges(*stateChanges, *compartment);

  // Let each compressor take a whack at the pinata.
  for (osc::count_t i = 0; i < m_numCompressors; i++)
  {
    m_compressorList[i]->handleFeedback(*compartment);
  }

  compartment->unlock();
  compartment->release();
}

/**
  Compresses an application message into a SigComp message

  @param input     Pointer to a buffer containing the uncompressed
                   application-level message that is to be compressed.

  @param inputSize Number of bytes in the input message

  @param id            Pointer to the compartment identifier
                       for the compartment with which this
                       message is to be associated.
                       Compartment identifiers are chosen
                       unilaterally by the application.

  @param idLength      Size of the compartment identifier,
                       in bytes.

  @param reliableTransport  Flag indicating whether the request
                            is to be sent over a reliable transport.

  @returns  A SigcompMessage instance from which an appropriate
            buffer can be extracted for sending to the network.
            The caller owns this object and is responsible for
            destroying it.

  @see SigcompMessage
 */

osc::SigcompMessage *
osc::Stack::compressMessage(const void *input , size_t inputSize,
                            const void *id, size_t idLength,
                            bool reliableTransport)
{
  DEBUG_STACK_FRAME;
  osc::SigcompMessage *sm = 0;

  osc::Compartment *compartment = 
    m_stateHandler.getCompartment(osc::compartment_id_t(
                       reinterpret_cast<const osc::byte_t*>(id), 
                       idLength));

  if (!compartment)
  {
    return 0;
  }

  int compressorIndex = -1;
  // The first compressor that successfully produces a compressed
  // message wins.
  for (osc::count_t i = 0; i < m_numCompressors && !sm; i++)
  {
    sm = m_compressorList[i]->compress(*compartment, 
                               reinterpret_cast<const osc::byte_t*>(input),
                               inputSize,
                               reliableTransport);
    if (sm)
    {
      compressorIndex = i;
    }
  }

  // If no compressor could handle this message, send "uncompressed"
  // See draft-ietf-rohc-sigcomp-impl-guide.
  if (!sm)
  {
    osc::byte_t uncompressedBytecodes [] = 
      {0x1c, 0x01, 0x86, 0x09, 0x22, 0x86, 0x01, 0x16, 0xf9, 0x23};
    sm = new osc::SigcompMessage(uncompressedBytecodes,
                                 sizeof(uncompressedBytecodes),
                                 128,
                                 inputSize);
    OSC_MEMMOVE(sm->getInput(), input, inputSize);
  }

  // Add returned feedback (from compartment requested feedback)
  compartment->writeLock();
  sm->addReturnedFeedback(compartment->getRequestedFeedback());
  compartment->freeRequestedFeedback();


  // Record this message's hash so we can locate the proper compartment
  // if we later get a NACK for it.
  if (m_stateHandler.getSigcompVersion() >= 2)
  {
    osc::sha1_t sha1;
    sm->getSha1Hash(sha1.digest,20);

    m_stateHandler.addNack(sha1, compartment);
    compartment->unlock();

    if (compressorIndex >= 0)
    {
      m_compressorList[compressorIndex]->storeNackInfo(sha1, *compartment);
    }
  }
  else
  {
    compartment->unlock();
  }

  compartment->release();

  return sm;
}

void
osc::Stack::handleNack(osc::SigcompMessage &nack)
{
  DEBUG_STACK_FRAME;
  // Find corresponding compartment
  osc::Compartment *compartment = 
    m_stateHandler.findNackedCompartment(nack.getNackSha1());

  if (!compartment)
  {
    // The nack doesn't seem to correspond to a compartment.
    return;
  }

  switch (nack.getNackReason())
  {
    case STATE_NOT_FOUND           :
      // Remove offending state
      {
        osc::Buffer stateId(nack.getNackDetails(), 
                            nack.getNackDetailLength(),
                            false);
        compartment->writeLock();
        compartment->removeRemoteState(stateId);
        compartment->unlock();
      }
      break;

    case CYCLES_EXHAUSTED          :
    case USER_REQUESTED            :
    case SEGFAULT                  :
    case TOO_MANY_STATE_REQUESTS   :
    case INVALID_STATE_ID_LENGTH   :
    case INVALID_STATE_PRIORITY    :
    case OUTPUT_OVERFLOW           :
    case STACK_UNDERFLOW           :
    case BAD_INPUT_BITORDER        :
    case DIV_BY_ZERO               :
    case SWITCH_VALUE_TOO_HIGH     :
    case TOO_MANY_BITS_REQUESTED   :
    case INVALID_OPERAND           :
    case HUFFMAN_NO_MATCH          :
    case MESSAGE_TOO_SHORT         :
    case INVALID_CODE_LOCATION     :
    case BYTECODES_TOO_LARGE       :
    case INVALID_OPCODE            :
    case INVALID_STATE_PROBE       :
    case ID_NOT_UNIQUE             :
    case MULTILOAD_OVERWRITTEN     :
    case STATE_TOO_SHORT           :
    case INTERNAL_ERROR            :
    case FRAMING_ERROR             :
    default:
      // For all other cases, we assume that the picture that we
      // have of remote state must be completely wrong. Destroy
      // what we expect them to have in terms of state.
      compartment->writeLock();
      compartment->resetRemoteStates();
      compartment->unlock();
      break;
  }

  for (osc::count_t i = 0; i < m_numCompressors; i++)
  {
    m_compressorList[i]->handleNack(nack, *compartment);
  }

  compartment->release();
}

size_t
osc::Stack::uncompressMessage(osc::SigcompMessage &sm,
                              void *output, size_t outputSize,
                              osc::StateChanges *& stateChanges, 
                              bool stream)
{
  DEBUG_STACK_FRAME;
  stateChanges = 0;

  if (m_nack)
  {
    delete(m_nack);
    m_nack = 0;
  }

  if (sm.isNack())
  {
    handleNack(sm);
    return 0;
  }

  osc::BitBuffer outputBuffer(reinterpret_cast<osc::byte_t *>(output),
                              outputSize);

  m_udvm.init(sm, stream);

  // This check is merely an optimization: an invalid SigComp
  // message will cause the UDVM to fail even before execution.
  if (sm.isValid())
  {
    m_udvm.setOutputBuffer(outputBuffer);
    m_udvm.execute();
  }

  if (m_udvm.isFailed())
  {
    if (m_stateHandler.getSigcompVersion() >= 2)
    {
      m_nack = m_udvm.getNack(sm);
    }
    return 0;
  }

  stateChanges = m_udvm.getProposedStates();

  if (stateChanges)
  {
    stateChanges->setReturnedFeedback
      (sm.getReturnedFeedback(), sm.getReturnedFeedbackLength());
  }

  return outputBuffer.getBufferSize()/8;
}

/**
  Indicate that the application is finished with the indicated
  compartment. This may or may not actually cause the compartment
  to be deleted. Any future application references to the
  specified compartment ID will cause a new compartment to be
  created.

  @note This method doesn't actually do anything yet
  @todo Implement compartment closing
 */

bool 
osc::Stack::closeCompartment(const void *id, size_t idLength)
{
  DEBUG_STACK_FRAME;
  assert(id != 0);
  assert(idLength != 0);
/*
  m_stateHandler.removeCompartment(osc::compartment_id_t(
                       reinterpret_cast<const osc::byte_t*>(id), 
                       idLength));
*/
  return false;
}

/* ********************************************************************** */
/* General documentation for the Open SigComp project follows             */
/* ********************************************************************** */

/**
  @namespace osc
  @brief Namespace for all classes and functions in the Open SigComp library.
*/

/**
  @mainpage Open SigComp Doxygen Documentation

  These pages provide detailed documentation for the classes
  that comprise the Open SigComp library. For more information,
  please see the Open SigComp project homepage at
  http://www.opensigcomp.org/.
*/
