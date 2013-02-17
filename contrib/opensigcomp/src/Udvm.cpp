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
  @file Udvm.cpp
  @brief Partial implementation of osc::Udvm class.

  @see UdvmOpcodes.h
*/


#include "ProfileStack.h"
#include <assert.h>
#include "Libc.h"
#include "Udvm.h"
#include "UdvmOpcodes.h"
#include "StateHandler.h"
#include "StateChanges.h"
#include "SigcompMessage.h"

#ifdef DEBUG_UDVM
#include <iostream>
#include <iomanip>
char *opcodeName[] =
{
  "DECOMPRESSION-FAILURE",
  "AND",
  "OR",
  "NOT",
  "LSHIFT",
  "RSHIFT",
  "ADD",
  "SUBTRACT",
  "MULTIPLY",
  "DIVIDE",
  "REMAINDER",
  "SORT-ASCENDING",
  "SORT-DESCENDING",
  "SHA-1",
  "LOAD",
  "MULTILOAD",
  "PUSH",
  "POP",
  "COPY",
  "COPY-LITERAL",
  "COPY-OFFSET",
  "MEMSET",
  "JUMP",
  "COMPARE",
  "CALL",
  "RETURN",
  "SWITCH",
  "CRC",
  "INPUT-BYTES",
  "INPUT-BITS",
  "INPUT-HUFFMAN",
  "STATE-ACCESS",
  "STATE-CREATE",
  "STATE-FREE",
  "OUTPUT",
  "END-MESSAGE"
};
#endif

/**
  Constructor for osc::Udvm.

  @todo Ideally, upon construction, there should be some
        way to indicate that this UDVM will be used
        <i>only</i> for stream (TCP) traffic. Doing so
        would allow us to allocate half the decompression
        memory size instead of the whole thing -- since
        the available memory on streams is only half the
        DMS.
 */
osc::Udvm::Udvm(StateHandler &sh, 
                osc::u32 decompressionMemorySize)
  : m_stateHandler(sh),
    m_memorySize(decompressionMemorySize),
    m_decompressionMemorySize(decompressionMemorySize),
    m_inputBuffer(0),
    m_outputBuffer(0),
    m_stateChanges(0),
    m_messageComplete(false)
{
  DEBUG_STACK_FRAME;
  if (m_memorySize > 0x10000)
  {
    m_memorySize = 0x10000;
  }

  m_memory = new byte_t[m_memorySize];

  reset();
}

/**
  Copy constructor for osc::Udvm.
 */
osc::Udvm::Udvm(Udvm const &r)
  : m_stateHandler(r.m_stateHandler)
{
  DEBUG_STACK_FRAME;
  /* Assign attributes */
  assert(0);
}

/**
  Destructor for osc::Udvm.
 */
osc::Udvm::~Udvm()
{
  DEBUG_STACK_FRAME;
  delete [] m_memory;
  delete m_stateChanges;
}

/**
  Assignment operator for osc::Udvm.
 */
osc::Udvm &
osc::Udvm::operator=(Udvm const &r)
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
osc::Udvm::reset()
{
  DEBUG_STACK_FRAME;
  m_memorySize = m_decompressionMemorySize;
  if (m_memorySize > 0x10000)
  {
    m_memorySize = 0x10000;
  }

  OSC_MEMSET(m_memory, 0, m_memorySize);
  m_failureReason = osc::OK;
  m_failureDetailLength = 0;
  m_nextParameter = 0;
  m_pc = 0;
  m_cyclesUsed = 0;
  m_cycleAllowance = 0;
  m_messageComplete = false;
  if (m_stateChanges)
  {
    delete(m_stateChanges);
  }
  m_stateChanges = new osc::StateChanges();

  m_inputBuffer = 0;
  m_outputBuffer = 0;
}

void
osc::Udvm::loadCode(osc::u16 position,
                    const osc::byte_t bytes[],
                    size_t size)
{
  DEBUG_STACK_FRAME;
  if (position + size > m_memorySize)
  {
    DECOMPRESSION_FAILURE(osc::BYTECODES_TOO_LARGE);
    m_failureDetails[0] = m_memorySize >> 8;
    m_failureDetails[1] = m_memorySize & 0xFF;
    m_failureDetailLength = 2;
    return;
  }

  if (position < 128)
  {
    DECOMPRESSION_FAILURE(osc::INVALID_CODE_LOCATION);
  }

  OSC_MEMMOVE(m_memory+position, bytes, size);
  m_pc = position;
}

void
osc::Udvm::loadState(const osc::byte_t *stateId, osc::u16 idLength)
{
  DEBUG_STACK_FRAME;
  assert(idLength > 0);
  osc::state_id_t id(stateId, idLength);
  osc::nack_code_t findStatus;
  osc::State *state = m_stateHandler.getState(id, findStatus);


  if (!state)
  {
    DECOMPRESSION_FAILURE(findStatus);

    // Add state ID to NACK
    OSC_MEMMOVE(m_failureDetails, stateId, idLength);
    m_failureDetailLength = idLength;
    return;
  }

  if (state->getMinimumAccessLength() > idLength)
  {
    DECOMPRESSION_FAILURE(osc::STATE_NOT_FOUND);
    // Add state ID to NACK
    OSC_MEMMOVE(m_failureDetails, stateId, idLength);
    m_failureDetailLength = idLength;
    state->release();
    return;
  }

  if (state->getAddress() + state->getStateSize() > m_memorySize)
  {
    DECOMPRESSION_FAILURE(osc::BYTECODES_TOO_LARGE);
    m_failureDetails[0] = m_memorySize >> 8;
    m_failureDetails[1] = m_memorySize & 0xFF;
    m_failureDetailLength = 2;
    state->release();
    return;
  }

  OSC_MEMMOVE(m_memory+state->getAddress(),
          state->getStateDataRawBuffer(),
          state->getStateSize());

  setStateInformation(idLength, state->getStateSize());

  m_pc = state->getInstruction();
  state->release();
}


void
osc::Udvm::init(osc::SigcompMessage &sm, bool stream)
{
  DEBUG_STACK_FRAME;
  reset();

  if (!sm.isValid())
  {
    DECOMPRESSION_FAILURE(sm.getStatus());
    return;
  }

  //
  // According to RFC 3320, the space available is:
  //
  //  - The decompression memory size, MINUS
  //    the size of the message, for datagrams.
  //
  //  - For streams, half the decompression memory
  //    size.
  //

  if (stream)
  {
    m_memorySize /= 2;
  }
  else
  {
    m_memorySize -= sm.getDatagramLength();
  }

  if (sm.getBytecodeLength())
  {
    loadCode(sm.getBytecodeDestination(),
             sm.getBytecodes(),
             sm.getBytecodeLength());
  }
  else
  {
    loadState(sm.getStateId(), sm.getStateIdLength());
  }
  m_inputBuffer = &sm.getBitBuffer();
  m_cycleAllowance = (1000 + sm.getHeaderLength() * 8) 
                     * m_stateHandler.getCyclesPerBit();

  // Set up useful values and reserved space
  setWord(UDVM_MEMORY_SIZE, m_memorySize);
  setWord(CYCLES_PER_BIT, m_stateHandler.getCyclesPerBit());
  setWord(SIGCOMP_VERSION, m_stateHandler.getSigcompVersion());
  OSC_MEMSET(m_memory + RESERVED, 0, 32 - RESERVED);
}

void
osc::Udvm::setStateInformation(osc::u16 stateIdLength, osc::u16 stateLength)
{
  DEBUG_STACK_FRAME;
  setWord(PARTIAL_STATE_ID_LENGTH, stateIdLength);
  setWord(STATE_LENGTH, stateLength);
}

/**
  @note The caller owns the returned SigcompMessage.
 */
osc::SigcompMessage *
osc::Udvm::getNack(osc::SigcompMessage &badMessage)
{
  DEBUG_STACK_FRAME;
  if (m_failureReason)
  {
    return new osc::SigcompMessage(m_failureReason,
                                   m_failureOpcode,
                                   m_failurePc,
                                   badMessage,
                                   m_failureDetails,
                                   m_failureDetailLength);
  }

  // Something has gone horribly wrong if we reach this point.
  return new osc::SigcompMessage(osc::INTERNAL_ERROR,
                                 m_memory[m_pc],
                                 m_pc,
                                 badMessage);
}

/**
  @note This method destructively removes the states from
        the UDVM; the caller owns the StateChanges object
        that is returned.
 */
osc::StateChanges *
osc::Udvm::getProposedStates()
{
  DEBUG_STACK_FRAME;
  return m_stateChanges;
  m_stateChanges = 0;
}

osc::u16
osc::Udvm::getLiteralParameter()
{
  DEBUG_STACK_FRAME;
  osc::u16 retval = 0;

  switch (m_memory[m_nextParameter] & 0xc0)
  {
    case 0x00:
    case 0x40:
      retval = m_memory[m_nextParameter];
      m_nextParameter += 1;
      break;

    case 0x80:
      retval = getWord(m_nextParameter) & 0x3fff;
      m_nextParameter += 2;
      break;

    case 0xc0:
      retval =  getWord(m_nextParameter + 1);
      m_nextParameter += 3;
      break;
  }

#ifdef DEBUG_UDVM
  std::cout << " #" << retval;
#endif

  return retval;
}

osc::u16
osc::Udvm::getReferenceParameter()
{
  DEBUG_STACK_FRAME;
  osc::u16 pointer = 0;

  switch (m_memory[m_nextParameter] & 0xc0)
  {
    case 0x00:
    case 0x40:
      pointer = m_memory[m_nextParameter] * 2;
      m_nextParameter += 1;
      break;

    case 0x80:
      pointer = (getWord(m_nextParameter) & 0x3fff) * 2;
      m_nextParameter += 2;
      break;

    case 0xc0:
      if (m_memory[m_nextParameter] != 0xc0)
      {
        DECOMPRESSION_FAILURE(osc::INVALID_OPERAND);
      }

      pointer = getWord(m_nextParameter + 1);
      m_nextParameter += 3;
      break;
  }

#ifdef DEBUG_UDVM
  std::cout << " $" << pointer << " [" << getWord(pointer) << "]";
#endif

  return (pointer);
}

osc::u16
osc::Udvm::getMultitypeParameter(osc::u16 *ptr, bool *pointerType)
{
  DEBUG_STACK_FRAME;
  osc::u16 pointer = 0;
  osc::u16 retval = 0;

  if (pointerType) 
  { 
    *pointerType = false;
  }

  switch (m_memory[m_nextParameter] & 0xf0)
  {
    // 00nnnnnn
    case 0x00:
    case 0x10:
    case 0x20:
    case 0x30:
      retval = m_memory[m_nextParameter];
      m_nextParameter++;
      break;

    // 01nnnnnn
    case 0x40:
    case 0x50:
    case 0x60:
    case 0x70:
      pointer = (m_memory[m_nextParameter] & 0x3f) * 2;
      retval = getWord(pointer);
      if (pointerType) 
      { 
        *pointerType = true;
      }
      m_nextParameter++;
      break;

    // 1000xxxx
    case 0x80:
      switch (m_memory[m_nextParameter])
      {
        // 10000000 nnnnnnnn nnnnnnnn
        case 0x80:
          retval = getWord(m_nextParameter + 1);
          m_nextParameter += 3;
          break;

        // 10000001 nnnnnnnn nnnnnnnn
        case 0x81:
          pointer = getWord(m_nextParameter + 1);
          retval = getWord(pointer);
          if (pointerType) 
          { 
            *pointerType = true;
          }
          m_nextParameter += 3;
          break;

        // Bad Prefixes
        case 0x82:
        case 0x83:
        case 0x84:
        case 0x85:
          DECOMPRESSION_FAILURE(osc::INVALID_OPERAND);
          break;

        // 1000011n
        case 0x86: retval = 64;     m_nextParameter++; break;
        case 0x87: retval = 128;    m_nextParameter++; break;

        // 10001nnn
        case 0x88: retval = 256;    m_nextParameter++; break;
        case 0x89: retval = 512;    m_nextParameter++; break;
        case 0x8a: retval = 1024;   m_nextParameter++; break;
        case 0x8b: retval = 2048;   m_nextParameter++; break;
        case 0x8c: retval = 4096;   m_nextParameter++; break;
        case 0x8d: retval = 8192;   m_nextParameter++; break;
        case 0x8e: retval = 16384;  m_nextParameter++; break;
        case 0x8f: retval = 32768;  m_nextParameter++; break;
      }
      break;

    // 1001nnnn nnnnnnnn
    case 0x90:
      retval = (getWord(m_nextParameter) & 0x0fff) + 61440;
      m_nextParameter += 2;
      break;

    // 101nnnnn nnnnnnnn
    case 0xa0:
    case 0xb0:
      retval = getWord(m_nextParameter) & 0x1fff;
      m_nextParameter += 2;
      break;

    // 110nnnnn nnnnnnnn
    case 0xc0:
    case 0xd0:
      pointer = getWord(m_nextParameter) & 0x1fff;
      retval = getWord(pointer);
      if (pointerType) 
      { 
        *pointerType = true;
      }
      m_nextParameter += 2;
      break;

    // 111nnnnn
    case 0xe0:
    case 0xf0:
      retval = (m_memory[m_nextParameter] & 0x1f) + 65504;
      m_nextParameter++;
      break;

    default:
      DECOMPRESSION_FAILURE(osc::INVALID_OPERAND);
      break;
  }

  if (ptr)
  {
    *ptr = pointer;
  }

#ifdef DEBUG_UDVM
  if (pointer)
  {
    std::cout << " $" << pointer << " [" << retval << "]";
  }
  else
  {
    std::cout << " " << retval;
  }
#endif

  return retval;
}

/**
  Implement byte copying rules per section 8.4 of RFC 3320.

  @param dst    UDVM address of the destination to which bytes
                are to be copied
  @param src    Pointer to the source from which bytes are
                to be copied
  @param length Number of bytes to be copied

  @returns      Address of last destination byte copied
 */
osc::u16
osc::Udvm::byteCopy(osc::u16 dst, const byte_t *src, size_t length)
{
  DEBUG_STACK_FRAME;
  osc::u16 byteCopyLeft = getWord(BYTE_COPY_LEFT);
  osc::u16 byteCopyRight = getWord(BYTE_COPY_RIGHT);

  while (length)
  {
    // Check for segmentation faults
    if (dst == m_memorySize)
    {
      DECOMPRESSION_FAILURE(osc::SEGFAULT);
      return dst;
    }

    m_memory[dst] = *src;
    dst++;
    src++;

    // Obey byte_copy_right and byte_copy_left
    if (dst == byteCopyRight)
    {
      dst = byteCopyLeft;
    }

    length--;
  }
  return dst;
}


/**
  Implement byte copying rules per section 8.4 of RFC 3320.

  @param dst    Pointer to the destination to which bytes
                are to be copied
  @param src    UDVM address of the source from which bytes are
                to be copied
  @param length Number of bytes to be copied

  @returns      Pointer to last destination byte copied
 */
osc::byte_t *
osc::Udvm::byteCopy(osc::byte_t *dst, osc::u16 src, size_t length)
{
  DEBUG_STACK_FRAME;
  osc::u16 byteCopyLeft = getWord(BYTE_COPY_LEFT);
  osc::u16 byteCopyRight = getWord(BYTE_COPY_RIGHT);

  while (length)
  {
    // Check for segmentation faults
    if (src == m_memorySize)
    {
      DECOMPRESSION_FAILURE(osc::SEGFAULT);
      return dst;
    }

    *dst = m_memory[src];
    dst++;
    src++;

    // Obey byte_copy_right and byte_copy_left
    if (src == byteCopyRight)
    {
      src = byteCopyLeft;
    }

    length--;
  }
  return dst;
}

/**
  Implement byte copying rules per section 8.4 of RFC 3320.

  @param dst    UDVM address of the destination to which bytes
                are to be copied
  @param src    UDVM address of the source from which bytes are
                to be copied
  @param length Number of bytes to be copied

  @returns      Address of last destination byte copied
 */
osc::u16
osc::Udvm::byteCopy(osc::u16 dst, osc::u16 src, size_t length)
{
  DEBUG_STACK_FRAME;
  osc::u16 byteCopyLeft = getWord(BYTE_COPY_LEFT);
  osc::u16 byteCopyRight = getWord(BYTE_COPY_RIGHT);

  while (length)
  {
    // Check for segmentation faults
    if (dst == m_memorySize || src == m_memorySize)
    {
      DECOMPRESSION_FAILURE(osc::SEGFAULT);
      return dst;
    }

    m_memory[dst] = m_memory[src];
    dst++;
    src++;

    // Obey byte_copy_right and byte_copy_left
    if (dst == byteCopyRight)
    {
      dst = byteCopyLeft;
    }
    if (src == byteCopyRight)
    {
      src = byteCopyLeft;
    }

    length--;
  }
  return dst;
}

void
osc::Udvm::execute()
{
  DEBUG_STACK_FRAME;
  register osc::u16 p1, p2, p3, p4, p5, p6, p7;
  while (m_cycleAllowance > m_cyclesUsed && !isFailed() && !isComplete())
  {
    m_nextParameter = m_pc+1;
    m_cyclesUsed += 1;

#ifdef DEBUG_UDVM
   if (m_memory[m_pc] <= 35)
   {
     std::cout << std::setw(5) << m_pc << " " <<  opcodeName[m_memory[m_pc]];
   }
#endif

    switch (m_memory[m_pc])
    {
      case 0:
        opcode_decompression_failure();
      break;
  
      case 1:
        p1 = getReferenceParameter(); // $operand_1
        p2 = getMultitypeParameter(); // %operand_2
        opcode_and(p1,p2);
      break;
  
      case 2:
        p1 = getReferenceParameter(); // $operand_1
        p2 = getMultitypeParameter(); // %operand_2
        opcode_or(p1,p2);
      break;
  
      case 3:
        opcode_not(
          getReferenceParameter()  // $operand_1
        );
      break;
  
      case 4:
        p1 = getReferenceParameter(); // $operand_1
        p2 = getMultitypeParameter(); // %operand_2
        opcode_lshift(p1,p2);
      break;
  
      case 5:
        p1 = getReferenceParameter(); // $operand_1
        p2 = getMultitypeParameter(); // %operand_2
        opcode_rshift(p1,p2);
      break;
  
      case 6:
        p1 = getReferenceParameter(); // $operand_1
        p2 = getMultitypeParameter(); // %operand_2
        opcode_add(p1,p2);
      break;
  
      case 7:
        p1 = getReferenceParameter(); // $operand_1
        p2 = getMultitypeParameter(); // %operand_2
        opcode_subtract(p1,p2);
      break;
  
      case 8:
        p1 = getReferenceParameter(); // $operand_1
        p2 = getMultitypeParameter(); // %operand_2
        opcode_multiply(p1,p2);
      break;
  
      case 9:
        p1 = getReferenceParameter(); // $operand_1
        p2 = getMultitypeParameter(); // %operand_2
        opcode_divide(p1,p2);
      break;
  
      case 10:
        p1 = getReferenceParameter(); // $operand_1
        p2 = getMultitypeParameter(); // %operand_2
        opcode_remainder(p1,p2);
      break;
  
      case 11:
        p1 = getMultitypeParameter(); // %start
        p2 = getMultitypeParameter(); // %n
        p3 = getMultitypeParameter(); // %k
        opcode_sort_ascending(p1,p2,p3);
      break;
  
      case 12:
        p1 = getMultitypeParameter(); // %start
        p2 = getMultitypeParameter(); // %n
        p3 = getMultitypeParameter(); // %k
        opcode_sort_descending(p1,p2,p3);
      break;
  
      case 13:
        p1 = getMultitypeParameter(); // %position
        p2 = getMultitypeParameter(); // %length
        p3 = getMultitypeParameter(); // %destination
        opcode_sha_1(p1,p2,p3);
      break;
  
      case 14:
        p1 = getMultitypeParameter(); // %address
        p2 = getMultitypeParameter(); // %value
        opcode_load(p1,p2);
      break;
  
      case 15:
        p1 = getMultitypeParameter(); // %address
        p2 =  getLiteralParameter();  // #n
        opcode_multiload(p1,p2);
      break;
  
      case 16:
        opcode_push(
          getMultitypeParameter()  // %value
        );
      break;
  
      case 17:
        opcode_pop(
          getMultitypeParameter()  // %address
        );
      break;
  
      case 18:
        p1 = getMultitypeParameter(); // %position
        p2 = getMultitypeParameter(); // %length
        p3 = getMultitypeParameter(); // %destination
        opcode_copy(p1,p2,p3);
      break;
  
      case 19:
        p1 = getMultitypeParameter(); // %position
        p2 = getMultitypeParameter(); // %length
        p3 = getReferenceParameter(); // $destination
        opcode_copy_literal(p1,p2,p3);
      break;
  
      case 20:
        p1 = getMultitypeParameter(); // %offset
        p2 = getMultitypeParameter(); // %length
        p3 = getReferenceParameter(); // $destination
        opcode_copy_offset(p1,p2,p3);
      break;
  
      case 21:
        p1 = getMultitypeParameter(); // %address
        p2 = getMultitypeParameter(); // %length
        p3 = getMultitypeParameter(); // %start_value
        p4 = getMultitypeParameter(); // %offset
        opcode_memset(p1,p2,p3,p4);
      break;
  
      case 22:
        opcode_jump(
          getAddressParameter()    // @address
        );
      break;
  
      case 23:
        p1 = getMultitypeParameter(); // %value_1
        p2 = getMultitypeParameter(); // %value_2
        p3 = getAddressParameter();   // @address_1
        p4 = getAddressParameter();   // @address_2
        p5 = getAddressParameter();   // @address_3
        opcode_compare(p1,p2,p3,p4,p5);
      break;
  
      case 24:
        opcode_call(
          getAddressParameter()    // @address
        );
      break;
  
      case 25:
        opcode_return();
      break;
  
      case 26:
        p1 = getLiteralParameter();   // #n
        p2 = getMultitypeParameter(); // %j
        opcode_switch(p1,p2);
      break;
  
      case 27:
        p1 = getMultitypeParameter(); // %value
        p2 = getMultitypeParameter(); // %position
        p3 = getMultitypeParameter(); // %length
        p4 = getAddressParameter();   // @address
        opcode_crc(p1,p2,p3,p4);
      break;
  
      case 28:
        p1 = getMultitypeParameter(); // %length
        p2 = getMultitypeParameter(); // %destination
        p3 = getAddressParameter();   // @address
        opcode_input_bytes(p1,p2,p3);
      break;
  
      case 29:
        p1 = getMultitypeParameter(); // %length
        p2 = getMultitypeParameter(); // %destination
        p3 = getAddressParameter();   // @address
        opcode_input_bits(p1,p2,p3);
      break;
  
      case 30:
        p1 = getMultitypeParameter(); // %destination
        p2 = getAddressParameter();   // @address
        p3 = getLiteralParameter();   // #n
        opcode_input_huffman(p1,p2,p3);
      break;
  
      case 31:
        p1 = getMultitypeParameter(); // %partial_identifier_start
        p2 = getMultitypeParameter(); // %partial_identifier_length
        p3 = getMultitypeParameter(); // %state_begin
        p4 = getMultitypeParameter(); // %state_length
        p5 = getMultitypeParameter(); // %state_address
        p6 = getMultitypeParameter(); // %state_instruction
        opcode_state_access(p1,p2,p3,p4,p5,p6);
      break;
  
      case 32:
        p1 = getMultitypeParameter(); // %state_length
        p2 = getMultitypeParameter(); // %state_address
        p3 = getMultitypeParameter(); // %state_instruction
        p4 = getMultitypeParameter(); // %minimum_access_length
        p5 = getMultitypeParameter(); // %state_retention_priority
        opcode_state_create(p1,p2,p3,p4,p5);
      break;
  
      case 33:
        p1 = getMultitypeParameter(); // %partial_identifier_start
        p2 = getMultitypeParameter(); // %partial_identifier_length
        opcode_state_free(p1,p2);
      break;
  
      case 34:
        p1 = getMultitypeParameter(); // %output_start
        p2 = getMultitypeParameter(); // %output_length
        opcode_output(p1,p2);
      break;
  
      case 35:
        p1 = getMultitypeParameter(); // %requested_feedback_location
        p2 = getMultitypeParameter(); // %returned_parameters_location
        p3 = getMultitypeParameter(); // %state_length
        p4 = getMultitypeParameter(); // %state_address
        p5 = getMultitypeParameter(); // %state_instruction
        p6 = getMultitypeParameter(); // %minimum_access_length
        p7 = getMultitypeParameter(); // %state_retention_priority
        opcode_end_message(p1,p2,p3,p4,p5,p6,p7);
      break;

      default:
        DECOMPRESSION_FAILURE(osc::INVALID_OPCODE);
  
    }
    m_pc = m_nextParameter;
#ifdef DEBUG_UDVM
   std::cout << std::endl;
#endif
  }

  if (!isComplete() && m_cyclesUsed >= m_cycleAllowance)
  {
    DECOMPRESSION_FAILURE(osc::CYCLES_EXHAUSTED);
    m_failureDetails[0] = m_stateHandler.getCyclesPerBit();
    m_failureDetailLength = 1;
  }
}
