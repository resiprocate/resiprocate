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
  @file SigcompMessage.cpp
  @brief Implementation of osc::SigcompMessage class.
*/



#include "ProfileStack.h"
#include <assert.h>
#include "Libc.h"
#include "SigcompMessage.h"
#include "Sha1Hasher.h"
#include "Buffer.h"

#ifdef DEBUG
#include <iomanip>
#endif

/**
  Constructor for messages received from the wire

  @note This class is responsible for ensuring that the buffer
        doesn't get modified if this constructor has been used.
        The "m_ownBody" flag is used for this purpose.

  @todo The code needs a bit of a scrubdown to ensure that the
        m_ownBody flag always supresses changing the buffer passed
        into this constructor.
 */
osc::SigcompMessage::SigcompMessage(const osc::byte_t *buffer,
                                    size_t bufferLength)
  : m_header(buffer[0]),
    m_buffer(0),
    m_returnedFeedback(0), m_returnedFeedbackLength(0), 
    m_ownReturnedFeedback(false),
    m_body(0), m_bodySize(0), m_ownBody(false),
    m_status(osc::OK),
    m_stateId(0), m_stateIdLength(0),
    m_input(0), m_inputLength(0), m_inputBuffer(),
    m_codeLength(0), m_codeDestination(0), m_bytecodes(0),
    m_isNack(false), m_headerLength(0),
    m_streamBuffer(0), m_streamBufferLength(0),
    m_datagramBuffer(0)
{
  DEBUG_STACK_FRAME;
  if ((m_header & 0xF8) != 0xF8)
  {
    m_status = osc::INTERNAL_ERROR;
    m_body = const_cast<osc::byte_t*>(buffer) + 1;
    m_bodySize = bufferLength - 1;
    return;
  }

  if (bufferLength < 4)
  {
    m_status = osc::MESSAGE_TOO_SHORT;
    m_body = const_cast<osc::byte_t*>(buffer) + 1;
    m_bodySize = bufferLength - 1;
    return;
  }

  if (m_header & RETURNED_FEEDBACK_PRESENT)
  {
    m_returnedFeedback = const_cast<osc::byte_t*>(buffer) + 1;
    if ((*m_returnedFeedback) & 0x80)
    {
      m_returnedFeedbackLength = 1 + ((*m_returnedFeedback) & 0x7F);
    }
    else
    {
      m_returnedFeedbackLength = 1;
    }
    if (bufferLength < 4 + m_returnedFeedbackLength)
    {
      m_status = osc::MESSAGE_TOO_SHORT;
      return;
    }
  }

  m_body = const_cast<osc::byte_t*>(buffer) + 1 + m_returnedFeedbackLength;
  m_bodySize = bufferLength - 1 - m_returnedFeedbackLength;

  switch (m_header & 0x03)
  {
    case 0: m_stateIdLength = 0; break;
    case 1: m_stateIdLength = 6; break;
    case 2: m_stateIdLength = 9; break;
    case 3: m_stateIdLength = 12; break;
  }

  if (m_stateIdLength)
  {
    m_stateId = m_body;
    m_input = m_stateId + m_stateIdLength;
  }
  else
  {
    m_codeLength = (m_body[0] << 4) | (m_body[1] >> 4);
    m_codeDestination = ((m_body[1] & 0x0f) + 1) * 64;
    if (m_codeDestination < 128)
    {
      m_status = osc::INVALID_CODE_LOCATION;
      return;
    }
    m_bytecodes = m_body + 2;
    m_input = m_bytecodes + m_codeLength;
  }

  if (m_input > const_cast<osc::byte_t*>(buffer) + bufferLength)
  {
    m_status = osc::MESSAGE_TOO_SHORT;
    return;
  }
  m_inputLength = (const_cast<osc::byte_t*>(buffer) + bufferLength) - m_input;

  m_inputBuffer.reset(m_input, m_inputLength, m_inputLength * 8, 
                      osc::BitBuffer::MSB_FIRST, true);

  m_headerLength = bufferLength - m_inputLength;

  // Check for NACK messages
  if (m_stateIdLength == 0 && m_codeLength == 0)
  {
    if (m_inputLength < 24)
    {
      m_status = osc::MESSAGE_TOO_SHORT;
      return;
    }
    m_isNack = true;
  }
}

/**
  Constructor for locally-generated compressed messages with
  bytecodes.
 */
osc::SigcompMessage::SigcompMessage(
  osc::byte_t *byteCodes,
  osc::u16     codeLength,
  osc::u16     codeDestination,
  osc::u16     inputSize
) :
  m_header(0xF8),
  m_buffer(0),
  m_returnedFeedback(0), m_returnedFeedbackLength(0),
  m_ownReturnedFeedback(false),
  m_ownBody(true),
  m_status(osc::OK),
  m_stateId(0), m_stateIdLength(0),
  m_codeLength(codeLength), m_codeDestination(codeDestination),
  m_isNack(false), m_headerLength(0),
  m_streamBuffer(0), m_datagramBuffer(0)
{
  DEBUG_STACK_FRAME;
  if ((codeDestination & 0x003f) || codeDestination < 128
      || codeLength > 1024)
  {
    m_status = osc::INTERNAL_ERROR;
    return;
  }

  m_bodySize = 2 + codeLength + inputSize;
  m_body = new osc::byte_t[m_bodySize];
  m_body[0] = codeLength >> 4;
  m_body[1] = (codeLength << 4) | ((codeDestination-1) / 64);
  m_bytecodes = m_body + 2;
  OSC_MEMMOVE(m_bytecodes, byteCodes, codeLength);

  m_input = m_bytecodes + codeLength;
  m_inputLength = inputSize;
  m_inputBuffer.reset(m_input, m_inputLength, m_inputLength * 8,
                      osc::BitBuffer::MSB_FIRST, true);
}

/**
  Constructor for locally-generated compressed messages with
  partial state identifier.
 */
osc::SigcompMessage::SigcompMessage(
  const osc::byte_t *stateId,
  osc::u16     stateIdLength,
  osc::u16     inputSize
) :
  m_buffer(0),
  m_returnedFeedback(0), m_returnedFeedbackLength(0),
  m_ownReturnedFeedback(false),
  m_ownBody(true),
  m_status(osc::OK),
  m_stateIdLength(stateIdLength),
  m_codeLength(0), m_codeDestination(0), m_bytecodes(0),
  m_isNack(false), m_headerLength(0),
  m_streamBuffer(0), m_datagramBuffer(0)
{
  DEBUG_STACK_FRAME;
  switch(m_stateIdLength)
  {
    case 6:  m_header = 0xF9; break;
    case 9:  m_header = 0xFA; break;
    case 12: m_header = 0xFB; break;
    default: m_status = osc::INTERNAL_ERROR; return;
  }
  m_bodySize = m_stateIdLength + inputSize;
  m_body = new osc::byte_t[m_bodySize];

  m_stateId = m_body;
  OSC_MEMMOVE(m_stateId, stateId, m_stateIdLength);

  m_input = m_stateId + m_stateIdLength;
  m_inputLength = inputSize;
  m_inputBuffer.reset(m_input, m_inputLength, m_inputLength * 8,
                      osc::BitBuffer::MSB_FIRST, true);
}

/**
  Constructor for locally-generated NACK messages.
 */
osc::SigcompMessage::SigcompMessage(
  osc::nack_code_t      reason,
  osc::u8               opcode,
  osc::u16              pc,
  osc::SigcompMessage  &failedMessage,
  const osc::byte_t    *errorDetails,
  size_t                errorDetailLength
) :
  m_header(0xF8),
  m_buffer(0),
  m_returnedFeedback(0), m_returnedFeedbackLength(0),
  m_ownReturnedFeedback(false),
  m_ownBody(true),
  m_status(osc::OK),
  m_stateId(0), m_stateIdLength(0),
  m_codeLength(0), m_codeDestination(0), m_bytecodes(0),
  m_isNack(true), m_headerLength(0),
  m_streamBuffer(0), m_datagramBuffer(0)
{
  DEBUG_STACK_FRAME;
  m_bodySize = 26 + errorDetailLength;
  m_body = new osc::byte_t[m_bodySize];
  m_body[0] = 0;
  m_body[1] = 1;
  m_input = m_body + 2;
  m_inputLength = 24 + errorDetailLength;

  m_input[0] = reason;
  m_input[1] = opcode;
  m_input[2] = (pc >> 8) & 0xff;
  m_input[3] = pc & 0xff;

  // Hash failed message
  failedMessage.getSha1Hash(m_input+4);

  if (errorDetailLength)
  {
    OSC_MEMMOVE(m_input + 24, errorDetails, errorDetailLength);
  }

  m_inputBuffer.reset(m_input, m_inputLength, m_inputLength * 8, 
                      osc::BitBuffer::MSB_FIRST, true);
}

/**
  Copy constructor for osc::SigcompMessage.
 */
osc::SigcompMessage::SigcompMessage(SigcompMessage const &r)
{
  DEBUG_STACK_FRAME;
  /* Assign attributes */
  assert(0);
}

/**
  Destructor for osc::SigcompMessage.
 */
osc::SigcompMessage::~SigcompMessage()
{
  DEBUG_STACK_FRAME;
  if (m_ownBody)
  {
    delete[] m_body;
  }

  if (m_ownReturnedFeedback)
  {
    delete[] m_returnedFeedback;
  }

  delete[] m_datagramBuffer;
  delete[] m_streamBuffer;

  delete[] m_buffer;
}

/**
  Assignment operator for osc::SigcompMessage.
 */
osc::SigcompMessage &
osc::SigcompMessage::operator=(SigcompMessage const &r)
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

void
osc::SigcompMessage::takeOwnership(osc::byte_t *buffer)
{
  DEBUG_STACK_FRAME;
  delete(m_buffer);
  m_buffer = buffer;
}

void
osc::SigcompMessage::addReturnedFeedback(const osc::byte_t *feedback,
                                         osc::u16 feedbackLength)
{
  DEBUG_STACK_FRAME;
  if (feedbackLength == 0)
  {
    return;
  }

  delete [] m_datagramBuffer;
  m_datagramBuffer = 0;
  delete [] m_streamBuffer;
  m_streamBuffer = 0;
  m_streamBufferLength = 0;

  m_header |= RETURNED_FEEDBACK_PRESENT;

  if (m_ownReturnedFeedback)
  {
    delete m_returnedFeedback;
  }

  m_ownReturnedFeedback = true;
  m_returnedFeedback = new osc::byte_t[feedbackLength];
  OSC_MEMMOVE(m_returnedFeedback, feedback, feedbackLength);
  m_returnedFeedbackLength = feedbackLength;
}

void
osc::SigcompMessage::addReturnedFeedback(const osc::Buffer &b) 
{
  DEBUG_STACK_FRAME;
  addReturnedFeedback(b.data(),b.size());
}

/**
  Returns a pointer to a buffer appropriate for sending to
  the network over a datagram-oriented transport.

  The returned buffer remains the property of the SigcompMessage
  instance, and is valid as long as the SigcompMessage is not
  mutated.

  @see getDatagramLength
 */
osc::byte_t *
osc::SigcompMessage::getDatagramMessage()
{
  DEBUG_STACK_FRAME;
  if (m_datagramBuffer)
  {
    return m_datagramBuffer;
  }
  size_t length = getDatagramLength();
  m_datagramBuffer = new osc::byte_t[length];
  m_datagramBuffer[0] = m_header;

  if (m_returnedFeedback)
  {
    OSC_MEMMOVE(m_datagramBuffer+1, m_returnedFeedback,
                m_returnedFeedbackLength);
  }
  OSC_MEMMOVE(m_datagramBuffer+ 1+ m_returnedFeedbackLength,
              m_body, m_bodySize);

  return m_datagramBuffer;
}

/**
  Returns the length of the buffer returned by getDatagramMessage.

  @see getDatagramMessage
 */
size_t
osc::SigcompMessage::getDatagramLength() const
{
  DEBUG_STACK_FRAME;
  return (1 + getReturnedFeedbackLength() + m_bodySize);
}

/**
  Returns a pointer to a buffer appropriate for sending to
  the network over a stream-oriented transport.

  The returned buffer remains the property of the SigcompMessage
  instance, and is valid as long as the SigcompMessage is not
  mutated.

  @see getStreamLength

  @todo Make this a little less braindead about escaping
 */

osc::byte_t *
osc::SigcompMessage::getStreamMessage()
{
  DEBUG_STACK_FRAME;
  if (m_streamBuffer)
  {
    return m_streamBuffer;
  }

  // First, scan the message to determine what needs to be escaped.
  // We start by setting "extraBytes" to 2, which takes into account
  // the 0xFF 0xFF delimiter at the end of the message.
  int extraBytes = 2;
  unsigned int i;
  if (m_header == 0xFF)
  {
    extraBytes++;
  }

  for (i = 0; i < m_returnedFeedbackLength; i++)
  {
    if (m_returnedFeedback[i] == 0xFF) 
    {
      extraBytes++;
    }
  }

  for (i = 0; i < m_bodySize; i++)
  {
    if (m_body[i] == 0xFF) 
    {
      extraBytes++;
    }
  }

  m_streamBufferLength = 1 + getReturnedFeedbackLength() + m_bodySize 
                           + extraBytes;
  m_streamBuffer = new osc::byte_t[m_streamBufferLength];

  osc::byte_t *curr;

  m_streamBuffer[0] = m_header;
  curr = m_streamBuffer + 1;
  if (m_header == 0xFF)
  {
    m_streamBuffer[1] = 0x00;
    curr++;
  }

  if (m_returnedFeedback)
  {
    for (i = 0; i < m_returnedFeedbackLength; i++)
    {
      *curr = m_returnedFeedback[i];
      if (*curr == 0xFF)
      {
        curr++;
        *curr = 0x00;
      }
      curr++;
    }
  }

  for (i = 0; i < m_bodySize; i++)
  {
    *curr = m_body[i];
    if (*curr == 0xFF)
    {
      curr++;
      *curr = 0x00;
    }
    curr++;
  }

  // Add on the terminator
  *curr = 0xFF;
  curr++;
  *curr = 0xFF;

  return m_streamBuffer;
}

/**
  Returns the length of the buffer returned by getStreamMessage.

  @see getStreamMessage
 */
size_t
osc::SigcompMessage::getStreamLength()
{
  DEBUG_STACK_FRAME;
  if (!m_streamBuffer)
  {
    getStreamMessage();
  }
  return m_streamBufferLength;
}

void
osc::SigcompMessage::setInputLength(size_t length)
{
  DEBUG_STACK_FRAME;
  if (length > m_inputLength)
  {
    m_status = osc::INTERNAL_ERROR;
    return;
  }
  int diff = m_inputLength - length;
  m_inputLength = length;
  m_bodySize -= diff;
  m_inputBuffer.reset(m_input, m_inputLength, m_inputLength * 8, 
                      osc::BitBuffer::MSB_FIRST, true);
}

void
osc::SigcompMessage::getSha1Hash(osc::byte_t buffer[20], 
                                 size_t maxLength) const
{
  DEBUG_STACK_FRAME;
  osc::Sha1Hasher hasher;

  if (m_datagramBuffer)
  {
    hasher.addData(m_datagramBuffer, getDatagramLength());
  }
  else
  {
    hasher.addData(&m_header, sizeof(m_header));
    if (m_returnedFeedback)
    {
      hasher.addData(m_returnedFeedback, m_returnedFeedbackLength);
    }
    hasher.addData(m_body, m_bodySize);
  }

  hasher.getHash(buffer, maxLength);
}

/*
osc::buffer_descriptor_t * escapeBytes(osc::buffer_descriptor_t * buffer)
{
  DEBUG_STACK_FRAME;
  size_t length = buffer->bufferLength;
  osc::buffer_descriptor_t * temp = osc::createBuffer(length + ((length/128)+1)*2);
  size_t pos = 0;
  size_t outpos = 0;
  while(pos < length)
  {
    switch(buffer->buffer[pos])
    {
      case 0xFF:
      {
        size_t marker = outpos + 1; //Marks the location of the length byte
        osc::byte_t len = 0;
        temp->buffer[outpos]=0xFF;
        outpos++;
        pos++;        
        while(pos < length && len < 128)
        {
          len++;
          outpos++;
          temp->buffer[outpos] = buffer->buffer[pos];
          pos++;
        }
        temp->buffer[marker] = len;
      }
      default:
      {
        temp->buffer[outpos] = buffer->buffer[pos];
      }
    }
    pos++;
    outpos++;
  }

  //Copying buffer to correct sized buffer
  osc::buffer_descriptor_t * out = &osc::copySubStringOfBuffer(*temp,0,outpos); 
  osc::freeBuffer(*temp);
  return out;
}
*/

#ifdef DEBUG
void
osc::SigcompMessage::dump(std::ostream &os, unsigned int indent) const
{
  DEBUG_STACK_FRAME;
  osc::Buffer header(&m_header, sizeof(m_header));
  os << std::setw(indent) << ""
     << "[SigcompMessage " << this << "]" << std::endl;

  os << std::setw(indent) << ""
     << "  Header       = "  << header << std::endl;

  osc::Buffer hash;
  getSha1Hash(hash.getMutableBuffer(20));
  os << std::setw(indent) << ""
     << "  Hash         = "  << hash << std::endl;

  if (!isValid())
  {
    os << std::setw(indent) << "" << "  INVALID MESSAGE" << std::endl;
    return;
  }

  if (getReturnedFeedback())
  {
    os << std::setw(indent) << "" 
       << "  Feedback     = ";
    osc::Buffer t(m_returnedFeedback, m_returnedFeedbackLength);
    os << t << std::endl;
  }

  if (isNack())
  {
    osc::Buffer sha1(getNackSha1().digest, 20);
    osc::Buffer details(getNackDetails(), getNackDetailLength());
    os << std::setw(indent) << "" 
       << "  Type         = Nack" << std::endl;

    os << std::setw(indent) << "" 
       << "  Reason       = " << static_cast<unsigned int>(m_input[0]);
    if (m_input[0] < END_OF_NACK_CODE_LIST)
    {
      os << " (" << s_nackCode[m_input[0]] << ")";
    }
    os << std::endl;

    os << std::setw(indent) << "" 
       << "  OpCode       = " << static_cast<unsigned int>(getNackOpcode()) 
       << std::endl;

    os << std::setw(indent) << "" 
       << "  PC           = " << getNackPc() << std::endl;

    os << std::setw(indent) << "" 
       << "  SHA-1        = " << sha1 << std::endl;

    os << std::setw(indent) << "" 
       << "  Details      = " << details << std::endl;
    return;
  }

  if (getBytecodes())
  {
    osc::Buffer bytecodes(getBytecodes(), getBytecodeLength());
    os << std::setw(indent) << "" 
       << "  Type         = Bytecodes" << std::endl;

    os << std::setw(indent) << "" 
       << "  Bytecode Len = " << getBytecodeLength() << std::endl;

    os << std::setw(indent) << "" 
       << "  Bytecode Loc = " << getBytecodeDestination() << std::endl;

    os << std::setw(indent) << "" 
       << "  Bytecodes    = " << bytecodes << std::endl;
  }
  else
  {
    osc::Buffer stateid(getStateId(), getStateIdLength());

    os << std::setw(indent) << "" 
       << "  Type         = State ID" << std::endl;

    os << std::setw(indent) << "" 
       << "  State ID Len = " 
       << static_cast<unsigned int>(getStateIdLength()) << std::endl;

    os << std::setw(indent) << "" 
       << "  State ID     = " << stateid << std::endl;
  }

  osc::Buffer input (getInput(), getInputLength());
  os << std::setw(indent) << "" 
     << "  Input Length = " << getInputLength() << std::endl;
  os << std::setw(indent) << "" 
     << "  Input        = " << input << std::endl;
}

std::ostream &
osc::operator<< (std::ostream &os, const osc::SigcompMessage &b)
{
  DEBUG_STACK_FRAME;
  b.dump(os);
  return os;
}
#endif
