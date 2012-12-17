#ifndef __OSC__UDVM_OPCODES
#define __OSC__UDVM_OPCODES 1

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
  @file UdvmOpcodes.h
  @brief Partial implementation for osc::Udvm class.

  This file contains a number of *inlined* implementations
  of the various UDVM opcodes. Since only the UDVM itself
  will ever call the opcode methods, this file is to be
  included by Udvm.cpp and no one else.

  @see Udvm.cpp
*/

#include "Udvm.h"
#include "Types.h"
#include "NackCodes.h"
#include "Sha1Hasher.h"
#include "CrcComputer.h"
#include "BitBuffer.h"
#include "SigcompMessage.h"
#include "StateHandler.h"
#include "State.h"
#include "Buffer.h"
#include "StateChanges.h"
#include "Libc.h"

/**
  Sets a word within the UDVM memory -- checks for writes to
  out-of-bounds memory.

  @param location  UDVM memory position to write the word into
  @param value     Value to write into memory
*/
inline void
osc::Udvm::setWord(osc::u16 location, osc::u16 value)
{
  if (static_cast<osc::u32>(location) + 1 >= m_memorySize)
  {
    DECOMPRESSION_FAILURE(osc::SEGFAULT);
    return;
  }

  m_memory[location] = value >> 8;
  m_memory[location + 1]  = value & 0xff;
}

/**
  Gets a word within the UDVM memory -- checks for reads from
  out-of-bounds memory.

  @param location  UDVM memory position to read the word from
  @returns         Value from UDVM memory
*/
inline osc::u16
osc::Udvm::getWord(u16 location)
{
  if (static_cast<osc::u32>(location) + 1 >= m_memorySize)
  {
    DECOMPRESSION_FAILURE(osc::SEGFAULT);
    return 0;
  }

  return (m_memory[location] << 8) | m_memory[location+1];
}

/**
  Pushes a value on the the UDVM stack, as described in section
  8.3 of RFC 3320.

  @param value  Value to push onto the UDVM stack.
*/
inline void
osc::Udvm::pushStack(osc::u16 value)
{
  u16 stackLocation = getWord(STACK_LOCATION);
  u16 stackFill     = getWord(stackLocation);

  setWord(stackLocation + (stackFill * 2) + 2, value);
  setWord(stackLocation, stackFill + 1);
}

/**
  Pops a value from the the UDVM stack, as described in section
  8.3 of RFC 3320.

  @returns  Value popped from the UDVM stack.
*/

inline osc::u16
osc::Udvm::popStack()
{
  u16 stackLocation = getWord(STACK_LOCATION);
  u16 stackFill     = getWord(stackLocation);

  if (stackFill == 0)
  {
    DECOMPRESSION_FAILURE(osc::STACK_UNDERFLOW);
    return 0;
  }

  stackFill--;
  setWord(stackLocation, stackFill);

  return getWord(stackLocation + (stackFill * 2) + 2);
}

/**
  Consumes addtional cycles as required by RFC 3320.

  @retval true   Sufficient cycles are available
  @retval false  The UDVM has run out of cycles
 */

inline bool
osc::Udvm::consumeCycles(int cycles)
{
  m_cyclesUsed += cycles;
  if (m_cyclesUsed > m_cycleAllowance)
  {
    DECOMPRESSION_FAILURE(osc::CYCLES_EXHAUSTED);
    m_failureDetails[0] = m_stateHandler.getCyclesPerBit();
    m_failureDetailLength = 1;
    return false;
  }
  return true;
}

/**
  Handle opcode DECOMPRESSION-FAILURE (0).
  Takes 1 cycle
*/

inline void
osc::Udvm::opcode_decompression_failure()
{
  DECOMPRESSION_FAILURE(osc::USER_REQUESTED);
}

/**
  Handle opcode AND (1).
  Takes 1 cycle

  @param operand_1  Address of first operand (and destination)
  @param operand_2  Second operand
*/

inline void
osc::Udvm::opcode_and(osc::u16 operand_1, osc::u16 operand_2)
{
  setWord(operand_1, getWord(operand_1) & operand_2);
}

/**
  Handle opcode OR (2).
  Takes 1 cycle

  @param operand_1  Address of first operand (and destination)
  @param operand_2  Second operand
*/

inline void
osc::Udvm::opcode_or(osc::u16 operand_1, osc::u16 operand_2)
{
  setWord(operand_1, getWord(operand_1) | operand_2);
}

/**
  Handle opcode NOT (3).
  Takes 1 cycle

  @param operand_1  Address of operand (and destination)
*/

inline void
osc::Udvm::opcode_not(osc::u16 operand_1)
{
  setWord(operand_1, ~(getWord(operand_1)));
}

/**
  Handle opcode LSHIFT (4).
  Takes 1 cycle

  @param operand_1  Address of first operand (and destination)
  @param operand_2  Second operand
*/

inline void
osc::Udvm::opcode_lshift(osc::u16 operand_1, osc::u16 operand_2)
{
  setWord(operand_1, getWord(operand_1) << operand_2);
}

/**
  Handle opcode RSHIFT (5).
  Takes 1 cycle

  @param operand_1  Address of first operand (and destination)
  @param operand_2  Second operand
*/

inline void
osc::Udvm::opcode_rshift(osc::u16 operand_1, osc::u16 operand_2)
{
  setWord(operand_1, getWord(operand_1) >> operand_2);
}

/**
  Handle opcode ADD (6).
  Takes 1 cycle

  @param operand_1  Address of first operand (and destination)
  @param operand_2  Second operand
*/

inline void
osc::Udvm::opcode_add(osc::u16 operand_1, osc::u16 operand_2)
{
  setWord(operand_1, getWord(operand_1) + operand_2);
}

/**
  Handle opcode SUBTRACT (7).
  Takes 1 cycle

  @param operand_1  Address of first operand (and destination)
  @param operand_2  Second operand
*/

inline void
osc::Udvm::opcode_subtract(osc::u16 operand_1, osc::u16 operand_2)
{
  setWord(operand_1, getWord(operand_1) - operand_2);
}

/**
  Handle opcode MULTIPLY (8).
  Takes 1 cycle

  @param operand_1  Address of first operand (and destination)
  @param operand_2  Second operand
*/

inline void
osc::Udvm::opcode_multiply(osc::u16 operand_1, osc::u16 operand_2)
{
  setWord(operand_1, getWord(operand_1) * operand_2);
}

/**
  Handle opcode DIVIDE (9).
  Takes 1 cycle

  @param operand_1  Address of first operand (and destination)
  @param operand_2  Second operand
*/

inline void
osc::Udvm::opcode_divide(osc::u16 operand_1, osc::u16 operand_2)
{
  if (operand_2 == 0)
  {
    DECOMPRESSION_FAILURE(osc::DIV_BY_ZERO);
    return;
  }
  setWord(operand_1, getWord(operand_1) / operand_2);
}

/**
  Handle opcode REMAINDER (10).
  Takes 1 cycle

  @param operand_1  Address of first operand (and destination)
  @param operand_2  Second operand
*/

inline void
osc::Udvm::opcode_remainder(osc::u16 operand_1, osc::u16 operand_2)
{
  if (operand_2 == 0)
  {
    DECOMPRESSION_FAILURE(osc::DIV_BY_ZERO);
    return;
  }
  setWord(operand_1, getWord(operand_1) % operand_2);
}

inline osc::u16 
osc::Udvm::ceilingLog2(osc::u16 x)
{
  if (x <= 1) { return 0; }

  osc::u16 result = 0;

  x -= 1;

  while (x)
  {
    result++;
    x >>= 1;
  }

  return (result);
}

/**
  Handle opcode SORT-ASCENDING (11).
  Takes (1 + k * (ceiling(log2(k)) + n)) cycles

  @param start  Starting memory address of block of data to be sorted
  @param n      Number of lists present in data to be sorted
  @param k      Number of elements in each list
*/

inline void
osc::Udvm::opcode_sort_ascending(osc::u16 start, osc::u16 n, osc::u16 k)
{
  if (static_cast<osc::u32>(start) + n*k*2 >= m_memorySize)
  {
    DECOMPRESSION_FAILURE(osc::SEGFAULT);
    return;
  }

  // (SORT-ASCENDING) Adjust cycles left

  if (!consumeCycles(k * (ceilingLog2(k) + n)))
  {
    return;
  }

  u16 *temp = new u16[n*k];
  if (!temp)
  {
    DECOMPRESSION_FAILURE(osc::INTERNAL_ERROR);
    return;
  }

  u16 count[256];
  u16 *countPointer;
  byte_t *bytePointer;
  u16 sum, tempCount;
  int i, j;

  ////////////////////////////////////////////////////////////
  // Low-byte sorting: memory to temporary array
  ////////////////////////////////////////////////////////////

  // First, figure out how many of each byte we have
  OSC_MEMSET(count, 0, sizeof(count));
  bytePointer = m_memory + start + 1;
  for (i = 0; i < k; i++, bytePointer += 2)
  {
    count[*bytePointer]++;
  }

  // Turn count into an index by summing elements
  sum = 0;
  countPointer = count;
  for (i=256; i > 0; i--, countPointer++)
  {
    tempCount = *countPointer;
    *countPointer = sum;
    sum += tempCount;
  }

  // Now shuffle the values into the temporary array
  // according to their low byte value
  bytePointer = m_memory + start + 1;
  for (i=0; i < k; i++, bytePointer += 2)
  {
    countPointer = count + *bytePointer;
    
    for (j=0; j < n; j++)
    {
      temp[*countPointer + j*k] = getWord(start + 2*i + 2*j*k);
    }

    (*countPointer)++;
  }

  ////////////////////////////////////////////////////////////
  // High-byte sorting: temporary array to memory
  ////////////////////////////////////////////////////////////

  // First, figure out how many of each byte we have
  OSC_MEMSET(count, 0, sizeof(count));
  for (i = 0; i < k; i++)
  {
    count[temp[i]>>8]++;
  }

  // Turn count into an index by summing elements
  sum = 0;
  countPointer = count;
  for (i=256; i > 0; i--, countPointer++)
  {
    tempCount = *countPointer;
    *countPointer = sum;
    sum += tempCount;
  }

  // Now shuffle the values back into memory
  // according to their high byte value
  for (i=0; i < k; i++)
  {
    countPointer = count + (temp[i] >> 8);
    
    for (j=0; j < n; j++)
    {
      setWord(start + 2*i + 2*j*k, temp[*countPointer + j*k]);
    }

    (*countPointer)++;
  }

  delete [] temp;
}

/**
  Handle opcode SORT-DESCENDING (12).
  Takes (1 + k * (ceiling(log2(k)) + n)) cycles

  @param start  Starting memory address of block of data to be sorted
  @param n      Number of lists present in data to be sorted
  @param k      Number of elements in each list
*/

inline void
osc::Udvm::opcode_sort_descending(osc::u16 start, osc::u16 n, osc::u16 k)
{
  if (static_cast<osc::u32>(start) + n*k*2 >= m_memorySize)
  {
    DECOMPRESSION_FAILURE(osc::SEGFAULT);
    return;
  }

  if (!consumeCycles(k * (ceilingLog2(k) + n)))
  {
    return;
  }

  u16 *temp = new u16[n*k];
  if (!temp)
  {
    DECOMPRESSION_FAILURE(osc::INTERNAL_ERROR);
    return;
  }
  u16 count[256];
  u16 *countPointer;
  byte_t *bytePointer;
  u16 sum, tempCount;
  int i, j;

  ////////////////////////////////////////////////////////////
  // Low-byte sorting: memory to temporary array
  ////////////////////////////////////////////////////////////

  // First, figure out how many of each byte we have
  OSC_MEMSET(count, 0, sizeof(count));
  bytePointer = m_memory + start + 1;
  for (i = 0; i < k; i++, bytePointer += 2)
  {
    count[0xFF - *bytePointer]++;
  }

  // Turn count into an index by summing elements
  sum = 0;
  countPointer = count;
  for (i=256; i > 0; i--, countPointer++)
  {
    tempCount = *countPointer;
    *countPointer = sum;
    sum += tempCount;
  }

  // Now shuffle the values into the temporary array
  // according to their low byte value
  bytePointer = m_memory + start + 1;
  for (i=0; i < k; i++, bytePointer += 2)
  {
    countPointer = count + (0xff - *bytePointer);
    
    for (j=0; j < n; j++)
    {
      temp[*countPointer + j*k] = getWord(start + 2*i + 2*j*k);
    }

    (*countPointer)++;
  }

  ////////////////////////////////////////////////////////////
  // High-byte sorting: temporary array to memory
  ////////////////////////////////////////////////////////////

  // First, figure out how many of each byte we have
  OSC_MEMSET(count, 0, sizeof(count));
  for (i = 0; i < k; i++)
  {
    count[0xff - (temp[i]>>8)]++;
  }

  // Turn count into an index by summing elements
  sum = 0;
  countPointer = count;
  for (i=256; i > 0; i--, countPointer++)
  {
    tempCount = *countPointer;
    *countPointer = sum;
    sum += tempCount;
  }

  // Now shuffle the values back into memory
  // according to their high byte value
  for (i=0; i < k; i++)
  {
    countPointer = count + (0xff - (temp[i] >> 8));
    
    for (j=0; j < n; j++)
    {
      setWord(start + 2*i + 2*j*k, temp[*countPointer + j*k]);
    }

    (*countPointer)++;
  }

  delete [] temp;

}

/**
  Handle opcode SHA-1 (13).
  Takes (1 + length) cycles

  @param position     Starting memory address
  @param length       Number of bytes to digest
  @param destination  Location to place the 20-byte digest

  @note This implementation uses a lot of copying; however,
        the SHA-1 opcode is not used very often in practice,
        so this should not be a major problem. This situation
        could be improved by recreating the byte-copying
        rules directly here.
*/

inline void
osc::Udvm::opcode_sha_1(osc::u16 position,
                        osc::u16 length,
                        osc::u16 destination)
{
  // Subject to byte copying rules
  Sha1Hasher hasher;
  byte_t *input = new byte_t[length];
  if (!input)
  {
    DECOMPRESSION_FAILURE(osc::INTERNAL_ERROR);
    return;
  }
  byte_t hash[20];

  if (!consumeCycles(length))
  {
    delete [] input;
    return;
  }

  byteCopy(input, position, length);
  hasher.addData(input, length);
  hasher.getHash(hash);
  byteCopy(destination, hash, 20);
  delete [] input;
}

/**
  Handle opcode LOAD (14).
  Takes 1 cycle

  @param address  Address into which value is to be loaded
  @param value    Value to load into destination
*/

inline void
osc::Udvm::opcode_load(osc::u16 address, osc::u16 value)
{
  setWord(address, value);
}

/**
  Handle opcode MULTILOAD (15).
  Takes (1 + n) cycles.

  Parameters include (n) multitype values to load.

  @param address  Address into which values are to be loaded
  @param n        Number of values to be loaded
*/

inline void
osc::Udvm::opcode_multiload(osc::u16 address, osc::u16 n)
{
  int i;

  if (!consumeCycles(n))
  {
    return;
  }

  if ((address <= m_pc) && (address + (n * 2) > m_pc))
  {
    DECOMPRESSION_FAILURE(osc::MULTILOAD_OVERWRITTEN);
    return;
  }
 
  for (i = 0; i < n; i++)
  {
    setWord(address + i * 2, getMultitypeParameter());
    if ((m_pc < address) && (m_nextParameter > address))
    {
      DECOMPRESSION_FAILURE(osc::MULTILOAD_OVERWRITTEN);
      return;
    }
  }
}

/**
  Handle opcode PUSH (16).
  Takes 1 cycle

  @param value  Value to push on to the UDVM stack
*/

inline void
osc::Udvm::opcode_push(osc::u16 value)
{
  pushStack(value);
}

/**
  Handle opcode POP (17).
  Takes 1 cycle

  @param address  Address to which the popped value should be written
*/

inline void
osc::Udvm::opcode_pop(osc::u16 address)
{
  setWord(address, popStack());
}

/**
  Handle opcode COPY (18).
  Takes (1 + length) cycles

  @param position     Address of source
  @param length       Number of bytes to copy
  @param destination  Address of destination
*/

inline void
osc::Udvm::opcode_copy(osc::u16 position,
                       osc::u16 length,
                       osc::u16 destination)
{
  // Subject to byte copying rules
  if (!consumeCycles(length))
  {
    return;
  }
  
  byteCopy(destination, position, length);
}

/**
  Handle opcode COPY-LITERAL (19).
  Takes (1 + length) cycles

  @param position     Address of source
  @param length       Number of bytes to copy
  @param destination  Address containing address of destination
*/

inline void
osc::Udvm::opcode_copy_literal(osc::u16 position,
                               osc::u16 length,
                               osc::u16 destination)
{

  // Subject to byte copying rules
  if (!consumeCycles(length))
  {
    return;
  }

  if (!length)
  {
    return;
  }

  u16 nextByte = byteCopy(getWord(destination), position, length);
  setWord(destination, nextByte);
}

/**
  Handle opcode COPY-OFFSET (20).
  Takes (1 + length) cycles

  @param offset       Used to calculate destination; the UDVM
                      counts this many bytes backwards from
                      the destination to determine where to
                      copy data from.
  @param length       Number of bytes to copy
  @param destination  Address of address of destination
*/

inline void
osc::Udvm::opcode_copy_offset(osc::u16 offset,
                              osc::u16 length,
                              osc::u16 destination)
{
  // Subject to byte copying rules

  u16 position;
  u16 destinationAddress = getWord(destination);
  u16 byteCopyLeftAddress = getWord(BYTE_COPY_LEFT);
  u16 byteCopyRightAddress = getWord(BYTE_COPY_RIGHT);

  // Step backwards to the beginning of the buffer.
  if (destinationAddress >= byteCopyLeftAddress)
  {
    if (byteCopyLeftAddress < byteCopyRightAddress)
    {
      // To help ease the confusion here a bit:
      //
      //  - "index" represents a number of bytes into the circular buffer
      //
      //  - "bufferSize" is the length of the circular buffer
      //
      //  - "wraps" is the number of times the position would wrap across the
      //    circular buffer boundary when counting backwards (length) bytes.
    
      u16 index = destinationAddress - byteCopyLeftAddress;
    
      if (index < offset)
      {
        u16 bufferSize = byteCopyRightAddress - byteCopyLeftAddress;
  
        u16 wraps = (offset - index) / bufferSize;
        wraps += ((offset - index) % bufferSize)?1:0;
        index += wraps * bufferSize;
      }

      position = (byteCopyLeftAddress + index) - offset;
    }
    else
    {
      // This is kind of a degenerate case:
      // byte_copy_left is greater than (or equal to)
      // byte_copy_right. This isn't called out as illegal
      // in RFC3320, so we technically need to accomodate it.
      // Bizarre, but apparently legal. Notably, the torture
      // tests do not cover this circumstance.

      position = destinationAddress - offset -
                 (byteCopyLeftAddress - byteCopyRightAddress);
    }
  }
  else
  {
    position = destinationAddress - offset;
  }

  opcode_copy_literal(position, length, destination);
}

/**
  Handle opcode MEMSET (21).
  Takes (1 + length) cycles

  @param address      Location for destination
  @param length       Number of bytes to write
  @param start_value  Starting value for values to be written
  @param offset       Increment for subsequent values
*/

inline void
osc::Udvm::opcode_memset(osc::u16 address,
                         osc::u16 length,
                         osc::u16 start_value,
                         osc::u16 offset)
{
  // Subject to byte copying rules
  u16 byteCopyRightAddress = getWord(BYTE_COPY_RIGHT);
  u16 byteCopyLeftAddress = getWord(BYTE_COPY_LEFT);

  if (!consumeCycles(length))
  {
    return;
  }

  while (length)
  {
    if (address >= m_memorySize)
    {
      DECOMPRESSION_FAILURE(osc::SEGFAULT);
      return;
    }

    m_memory[address] = (start_value & 0xff);
    start_value = (start_value + offset) & 0xff;

    address++;
    if (address == byteCopyRightAddress)
    {
      address = byteCopyLeftAddress;
    }

    length--;
  }
}

/**
  Handle opcode JUMP (22).
  Takes 1 cycle

  @param address  New value for program counter
*/

inline void
osc::Udvm::opcode_jump(osc::u16 address)
{
  if (address > m_memorySize)
  {
    DECOMPRESSION_FAILURE(osc::SEGFAULT);
    return;
  }
  m_nextParameter = address;
}

/**
  Handle opcode COMPARE (23).
  Takes 1 cycle

  @param value_1    First value to be compared
  @param value_2    Second value to be compared
  @param address_1  Program counter value if value_1 < value_2
  @param address_2  Program counter value if value_1 == value_2
  @param address_3  Program counter value if value_1 > value_2
*/

inline void
osc::Udvm::opcode_compare(osc::u16 value_1,
                          osc::u16 value_2,
                          osc::u16 address_1,
                          osc::u16 address_2,
                          osc::u16 address_3)
{
  if (value_1 < value_2)
  {
    m_nextParameter = address_1;
  }
  else if (value_1 > value_2)
  {
    m_nextParameter = address_3;
  }
  else
  {
    m_nextParameter = address_2;
  }

  if (m_nextParameter >= m_memorySize)
  {
    DECOMPRESSION_FAILURE(osc::SEGFAULT);
    return;
  }
}

/**
  Handle opcode CALL (24).
  Takes 1 cycle

  @param address  Address to jump to
*/

inline void
osc::Udvm::opcode_call(osc::u16 address)
{
  pushStack(m_nextParameter);
  opcode_jump(address);
}

/**
  Handle opcode RETURN (25).
  Takes 1 cycle
*/

inline void
osc::Udvm::opcode_return()
{
  opcode_jump(popStack());
}

/**
  Handle opcode SWITCH (26).
  Takes (1 + n) cycles.

  Parmeters include (n) addresses in address type format.

  @param n  Number of potential branches
  @param j  Number of branch to take
*/

inline void
osc::Udvm::opcode_switch(osc::u16 n, osc::u16 j)
{
  osc::u16 target = 0;

  if (j >= n)
  {
    DECOMPRESSION_FAILURE(osc::SWITCH_VALUE_TOO_HIGH);
    return;
  }

  if (!consumeCycles(n))
  {
    return;
  }
 
  for (int i = 0; i <= j; i++)
  {
    target = getAddressParameter();
  } 

  if (target >= m_memorySize)
  {
    DECOMPRESSION_FAILURE(osc::SEGFAULT);
    return;
  }

  opcode_jump(target);
}

/**
  Handle opcode CRC (27).
  Takes (1 + length) cycles

  @param value     Expected CRC value
  @param position  Start of buffer to compute CRC over
  @param length    Length of buffer to compute CRC over
  @param address   Address to jump to if CRC is not as expected
*/

inline void
osc::Udvm::opcode_crc(osc::u16 value,
                      osc::u16 position,
                      osc::u16 length,
                      osc::u16 address)
{
  // Probably subject to byte copy rules, although RFC 3320 seems
  // to inadvertantly fail to mention this fact.
  CrcComputer computer;
  byte_t *input = new byte_t[length];
  if (!input)
  {
    DECOMPRESSION_FAILURE(osc::INTERNAL_ERROR);
    return;
  }
  u16 crc;

  if (!consumeCycles(length))
  {
    delete [] input;
    return;
  }

  byteCopy(input, position, length);
  computer.addData(input, length);
  crc = computer.getCrc();
  delete [] input;

  if (crc != value)
  {
    opcode_jump(address);
  }
}

/**
  Handle opcode INPUT-BYTES (28).
  Takes (1 + length) cycles

  @param length       Requested number of bytes
  @param destination  Address to copy input into
  @param address      Address to jump to if there is not enough
                      input available to satisfy the request
*/

inline void
osc::Udvm::opcode_input_bytes(osc::u16 length,
                              osc::u16 destination,
                              osc::u16 address)
{
  // Subject to byte copying rules
  
  if (!consumeCycles(length))
  {
    return;
  }

  // Discard any unused bits
  m_inputBuffer->discardPartialInputByte();

  osc::byte_t *source = m_inputBuffer->readBytes(length);

  if (source)
  {
    byteCopy(destination, source, length);
    m_cycleAllowance += 8 * length * m_stateHandler.getCyclesPerBit();
  }
  else
  {
    opcode_jump(address);
  }
}

/**
  Handle opcode INPUT-BITS (29).
  Takes 1 cycle

  @param length       Number of bits requested
  @param destination  Address to which bits are to be copied
  @param address      Address to jump to if there is not enough
                      input available to satisfy the request
*/

inline void
osc::Udvm::opcode_input_bits(osc::u16 length,
                             osc::u16 destination,
                             osc::u16 address)
{
  osc::u16 inputBitOrder = getWord(INPUT_BIT_ORDER);
  if (inputBitOrder & RESERVED_BITS)
  {
    DECOMPRESSION_FAILURE(osc::BAD_INPUT_BITORDER);
    return;
  }

  if (length > 16)
  {
    DECOMPRESSION_FAILURE(osc::TOO_MANY_BITS_REQUESTED);
    return;
  }
  
  osc::BitBuffer::packOrder_t packOrder = inputBitOrder & P_BIT 
                                          ? osc::BitBuffer::LSB_FIRST
                                          : osc::BitBuffer::MSB_FIRST;

  if (m_inputBuffer->getPackOrder() != packOrder)
  {
    m_inputBuffer->discardPartialInputByte();
    m_inputBuffer->setPackOrder(packOrder);
  }

  if (length > m_inputBuffer->getBufferSize())
  {
    opcode_jump(address);
    return;
  }

  if (inputBitOrder & F_BIT)
  {
    setWord(destination, m_inputBuffer->readLsbFirst(length));
  }
  else
  {
    setWord(destination, m_inputBuffer->readMsbFirst(length));
  }
  m_cycleAllowance += length * m_stateHandler.getCyclesPerBit();
}

/**
  Handle opcode INPUT-HUFFMAN (30).
  Takes (1 + n) cycles.

  Parameters include the following multitype fields
  for each of (n):
    - bits
    - lower_bound
    - upper_bound
    - uncompressed

  @param destination  Address to which bits are to be copied
  @param address      Address to jump to if there is not enough
                      input available to satisfy the request
  @param n            Number of sets of Huffman codes
*/

inline void
osc::Udvm::opcode_input_huffman(osc::u16 destination, 
                                osc::u16 address,
                                osc::u16 n)
{
  if (n == 0)
  {
    return;
  }

  if (!consumeCycles(n))
  {
    return;
  }

  osc::u16 inputBitOrder = getWord(INPUT_BIT_ORDER);
  if (inputBitOrder & RESERVED_BITS)
  {
    DECOMPRESSION_FAILURE(osc::BAD_INPUT_BITORDER);
    return;
  }
  
  osc::BitBuffer::packOrder_t packOrder = inputBitOrder & P_BIT 
                                          ? osc::BitBuffer::LSB_FIRST
                                          : osc::BitBuffer::MSB_FIRST;

  if (m_inputBuffer->getPackOrder() != packOrder)
  {
    m_inputBuffer->discardPartialInputByte();
    m_inputBuffer->setPackOrder(packOrder);
  }

  osc::u8 totalBits = 0;
  bool matchFound = false;
  // The following variable names are taken directly from RFC3320
  osc::u16 bits;
  osc::u16 lower_bound;
  osc::u16 upper_bound;
  osc::u16 uncompressed;
  osc::u16 H = 0; // Cumulative total

  while (n)
  {
    bits         = getMultitypeParameter();
    lower_bound  = getMultitypeParameter();
    upper_bound  = getMultitypeParameter();
    uncompressed = getMultitypeParameter();

    totalBits += bits;
    if (totalBits > 16)
    {
      DECOMPRESSION_FAILURE(osc::TOO_MANY_BITS_REQUESTED);
      return;
    }

    if (!matchFound)
    {
      if (bits > m_inputBuffer->getBufferSize())
      {
        opcode_jump(address);
        return;
      }

      if (inputBitOrder & H_BIT)
      {
        H = (H << bits) | m_inputBuffer->readLsbFirst(bits);
      }
      else
      {
        H = (H << bits) | m_inputBuffer->readMsbFirst(bits);
      }

      if (H >= lower_bound && H <= upper_bound)
      {
        matchFound = true;
        H += uncompressed - lower_bound;
      }
    }
    
    n--;
  }

  // We have now read in all the parameters. If there is no match,
  // there's been a decompression failure.
  if (!matchFound)
  {
    DECOMPRESSION_FAILURE(osc::HUFFMAN_NO_MATCH);
    return;
  }
  setWord(destination, H);
  m_cycleAllowance += totalBits * m_stateHandler.getCyclesPerBit();
}

/**
  Handle opcode STATE-ACCESS (31).
  Takes (1 + state_length) cycles

  @param partial_identifier_start  [Add parameter description]
  @param partial_identifier_length  [Add parameter description]
  @param state_begin  [Add parameter description]
  @param state_length  [Add parameter description]
  @param state_address  [Add parameter description]
  @param state_instruction  [Add parameter description]
*/

inline void
osc::Udvm::opcode_state_access(osc::u16 partial_identifier_start,
                               osc::u16 partial_identifier_length,
                               osc::u16 state_begin,
                               osc::u16 state_length,
                               osc::u16 state_address,
                               osc::u16 state_instruction)
{
  // Subject to byte copying rules
  if (partial_identifier_length < 6 || partial_identifier_length > 20)
  {
    DECOMPRESSION_FAILURE(osc::INVALID_STATE_ID_LENGTH);
    return;
  }

  if (state_length == 0 && state_begin != 0)
  {
    DECOMPRESSION_FAILURE(osc::INVALID_STATE_PROBE);
    return;
  }

  osc::state_id_t id(m_memory + partial_identifier_start,
                     partial_identifier_length);
  osc::nack_code_t findStatus;
  osc::State *state = m_stateHandler.getState(id, findStatus);
 
  if (!state)
  {
    DECOMPRESSION_FAILURE(findStatus);

    // Add state ID to NACK
    OSC_MEMMOVE(m_failureDetails, m_memory + partial_identifier_start,
            partial_identifier_length);
    m_failureDetailLength = partial_identifier_length;
    return;
  }

  if (state->getMinimumAccessLength() > partial_identifier_length)
  {
    state->release();
    DECOMPRESSION_FAILURE(osc::STATE_NOT_FOUND);

    // Add state ID to NACK
    OSC_MEMMOVE(m_failureDetails, m_memory + partial_identifier_start,
            partial_identifier_length);
    m_failureDetailLength = partial_identifier_length;
    return;
  }

  if (state_length == 0)
  {
    state_length = state->getStateSize();
  }

  if (state_address == 0)
  {
    state_address = state->getAddress();
  }

  if (state_instruction == 0)
  {
    state_instruction = state->getInstruction();
  }

  if (static_cast<size_t>(state_begin + state_length) > state->getStateSize())
  {
    DECOMPRESSION_FAILURE(osc::STATE_TOO_SHORT);
    OSC_MEMMOVE(m_failureDetails, m_memory + partial_identifier_start,
            partial_identifier_length);
    m_failureDetailLength = partial_identifier_length;
    state->release();
    return;
  }

  if (consumeCycles(state_length))
  {
    const osc::byte_t *buffer = state->getStateDataRawBuffer();
    byteCopy(state_address, buffer + state_begin, state_length);
  }

  state->release();

  if (state_instruction)
  {
    opcode_jump(state_instruction);
  }
}

/**
  Handle opcode STATE-CREATE (32).
  Takes (1 + state_length) cycles

  @param state_length  [Add parameter description]
  @param state_address  [Add parameter description]
  @param state_instruction  [Add parameter description]
  @param minimum_access_length  [Add parameter description]
  @param state_retention_priority  [Add parameter description]
*/

inline void
osc::Udvm::opcode_state_create(osc::u16 state_length,
                               osc::u16 state_address,
                               osc::u16 state_instruction,
                               osc::u16 minimum_access_length,
                               osc::u16 state_retention_priority)
{
  // Probably subject to byte copy rules, although RFC 3320 seems
  // to inadvertantly fail to mention this fact.

  if (minimum_access_length < 6 || minimum_access_length > 20)
  {
    DECOMPRESSION_FAILURE(osc::INVALID_STATE_ID_LENGTH);
    return;
  }

  if (state_retention_priority == 65535)
  {
    DECOMPRESSION_FAILURE(osc::INVALID_STATE_PRIORITY);
    return;
  }

  if (!consumeCycles(state_length))
  {
    return;
  }

  if (minimum_access_length < 6 || minimum_access_length > 20)
  {
    DECOMPRESSION_FAILURE(osc::INVALID_STATE_ID_LENGTH);
    return;
  }

  if (!m_stateChanges->addProtoState(state_length,
                                     state_address,
                                     state_instruction,
                                     minimum_access_length,
                                     state_retention_priority))
  {
    DECOMPRESSION_FAILURE(osc::TOO_MANY_STATE_REQUESTS);
  }
}

/**
  Handle opcode STATE-FREE (33).
  Takes 1 cycle

  @param partial_identifier_start  [Add parameter description]
  @param partial_identifier_length  [Add parameter description]
*/

inline void
osc::Udvm::opcode_state_free(osc::u16 partial_identifier_start,
                             osc::u16 partial_identifier_length)
{
  if (partial_identifier_length < 6 || partial_identifier_length > 20)
  {
    DECOMPRESSION_FAILURE(osc::INVALID_STATE_ID_LENGTH);
    return;
  }

  osc::state_id_t id(m_memory + partial_identifier_start,
                     partial_identifier_length);
  
  if (!m_stateChanges->deleteState(id))
  {
    DECOMPRESSION_FAILURE(osc::TOO_MANY_STATE_REQUESTS);
  }
}

/**
  Handle opcode OUTPUT (34).
  Takes (1 + output_length) cycles

  @param output_start   Address of bytes to output
  @param output_length  Length of bytes to output
*/

inline void
osc::Udvm::opcode_output(osc::u16 output_start,
                         osc::u16 output_length)
{
  // Subject to byte copying rules

  if (!consumeCycles(output_length))
  {
    return;
  }

  if (!m_outputBuffer)
  {
    DECOMPRESSION_FAILURE(osc::INTERNAL_ERROR);
    return;
  }

  if ((m_outputBuffer->getBufferSize()/8) + output_length > 0x10000)
  {
    DECOMPRESSION_FAILURE(osc::OUTPUT_OVERFLOW);
    return;
  }

  osc::byte_t *dest = m_outputBuffer->appendBytes(output_length);

  if (!dest)
  {
    DECOMPRESSION_FAILURE(osc::INTERNAL_ERROR);
    return;
  }

  byteCopy(dest, output_start, output_length);
}

/**
  Handle opcode END-MESSAGE (35).
  Takes (1 + state_length) cycles

  @param requested_feedback_location  [Add parameter description]
  @param returned_parameters_location  [Add parameter description]
  @param state_length  [Add parameter description]
  @param state_address  [Add parameter description]
  @param state_instruction  [Add parameter description]
  @param minimum_access_length  [Add parameter description]
  @param state_retention_priority  [Add parameter description]
*/

inline void
osc::Udvm::opcode_end_message(osc::u16 requested_feedback_location,
                              osc::u16 returned_parameters_location,
                              osc::u16 state_length,
                              osc::u16 state_address,
                              osc::u16 state_instruction,
                              osc::u16 minimum_access_length,
                              osc::u16 state_retention_priority)
{
  // Subject to byte copying rules

  if (!consumeCycles(state_length))
  {
    return;
  }

  m_messageComplete = true;

  if (minimum_access_length >= 6 && minimum_access_length <= 20 &&
      state_retention_priority != 65535 && state_length > 0)
  {
    if (minimum_access_length < 6 || minimum_access_length > 20)
    {
      DECOMPRESSION_FAILURE(osc::INVALID_STATE_ID_LENGTH);
      return;
    }

    m_stateChanges->addProtoState(state_length,
                                  state_address,
                                  state_instruction,
                                  minimum_access_length,
                                  state_retention_priority);
  }
  // Turn all proto-states into real states
  for (osc::u8 i=0; i < m_stateChanges->getNumOperations(); i++)
  {
    if (m_stateChanges->getOperationType(i) == osc::StateChanges::PROTO_STATE)
    {
      osc::State *state = new osc::State(m_stateChanges->getAddress(i),
                                         m_stateChanges->getInstruction(i),
                                         m_stateChanges->getAccessLength(i));

      if (!state)
      {
        DECOMPRESSION_FAILURE(osc::INTERNAL_ERROR);
        return;
      }

      osc::byte_t *stateBuffer = 
                 state->getMutableBuffer(m_stateChanges->getLength(i));

      if (m_stateChanges->getLength(i) > 0)
      {
        if (!stateBuffer)
        {
          DECOMPRESSION_FAILURE(osc::INTERNAL_ERROR);
          return;
        }

        byteCopy(stateBuffer, 
                 m_stateChanges->getAddress(i), 
                 m_stateChanges->getLength(i));

      }

      state->finalizeMutableBuffer();

      m_stateChanges->setState(i, state);
    }
  }

  /********************* Process requested feedback *********************/
  if (requested_feedback_location)
  {
    if (static_cast<osc::u32>(requested_feedback_location) >= m_memorySize)
    {
      DECOMPRESSION_FAILURE(osc::SEGFAULT);
      return;
    }

    osc::u8 flags = m_memory[requested_feedback_location];

    m_stateChanges->setIBitValid();
 
    if (flags & I_BIT)
    {
      m_stateChanges->setIBit();
    }

    if (flags & S_BIT)
    {
      m_stateChanges->setSBit();
    }

    if (flags & Q_BIT)
    {
      if (static_cast<osc::u32>(requested_feedback_location) + 1 
          >= m_memorySize)
      {
        DECOMPRESSION_FAILURE(osc::SEGFAULT);
        return;
      }
      osc::byte_t *start = m_memory + requested_feedback_location + 1;
      osc::u16 length = 1;
      if (*start & 0x80)
      {
        length += (*start) & 0x7F;
        if (static_cast<osc::u32>(requested_feedback_location) + length 
            >= m_memorySize)
        {
          DECOMPRESSION_FAILURE(osc::SEGFAULT);
          return;
        }
      }
      m_stateChanges->setRequestedFeedback(start, length);
    }

  }

  /********************* Process returned parameters *********************/
  if (returned_parameters_location)
  {
    if (static_cast<osc::u32>(returned_parameters_location) + 1>= m_memorySize)
    {
      DECOMPRESSION_FAILURE(osc::SEGFAULT);
      return;
    }

    osc::u8 cpbDmsSms = m_memory[returned_parameters_location];
    osc::u8 sigcompVersion = m_memory[returned_parameters_location + 1];

    if (cpbDmsSms)
    {
      m_stateChanges->setCpbDmsSms(cpbDmsSms);
    }

    if (sigcompVersion)
    {
      m_stateChanges->setRemoteSigcompVersion(sigcompVersion);
    }

    osc::byte_t *start;
    osc::u8 length; 

    // Get all the remote state IDs
    returned_parameters_location += 2;
    while ((static_cast<osc::u32>(returned_parameters_location) < m_memorySize)
           && (static_cast<unsigned int>(
                m_memory[returned_parameters_location] - 6) < (20 - 6)))
    {
      length = m_memory[returned_parameters_location];

      if (static_cast<osc::u32>(returned_parameters_location) + 1 + length
          >= m_memorySize)
      {
        DECOMPRESSION_FAILURE(osc::SEGFAULT);
        return;
      }

      start = m_memory + returned_parameters_location + 1;
      m_stateChanges->addRemoteState(state_id_t(start, length));

      returned_parameters_location += length + 1;
    }

    if (static_cast<osc::u32>(returned_parameters_location) >= m_memorySize)
    {
      DECOMPRESSION_FAILURE(osc::SEGFAULT);
      return;
    }
  }
}

#endif
