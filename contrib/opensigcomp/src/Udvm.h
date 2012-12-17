#ifndef __OSC__UDVM
#define __OSC__UDVM 1

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
  @file Udvm.h
  @brief Header file for osc::Udvm class.
*/


#include "Types.h"
#include "NackCodes.h"

#define DECOMPRESSION_FAILURE(reason)\
  {\
    m_failureReason = reason; \
    m_failureDetailLength = 0; \
    m_failureOpcode = m_memory[m_pc]; \
    m_failurePc = m_pc; \
    m_failureFile = __FILE__; \
    m_failureLine = __LINE__; \
  }

namespace osc
{
  class StateHandler;
  class BitBuffer;
  class StateChanges;
  class SigcompMessage;

  /**
    Implements the Universal Decompressor Virtual Machine.

    The Udvm is the heart of the decompression side of the SigComp
    stack. The Udvm class's main functionality is provided by a loop
    inside the "execute" method. This loop steps through the UDVM's
    memory, executing the instructions it encounters. Such execution is
    performed using a large switch statement, which calls methods that
    represent the UDVM opcodes defined in RFC 3320.
  */

  class Udvm
  {
    friend class Disassembler;
    
    /* The following allows the unit test driver to access the internals
       of this class */
    friend bool test_osc_Udvm_paramDecode();
    friend bool test_osc_Udvm_torture();
    friend bool runOneTest(osc::Udvm &udvm, int i);
    
    public:
      Udvm(osc::StateHandler &, 
           osc::u32 dms = (65536 * 2));
      ~Udvm();
 
      Udvm * operator &(){ return this; }
      Udvm const * operator &() const { return this; }

      void execute();

      void init(osc::SigcompMessage &sm, bool stream = false);

      void setInputBuffer(osc::BitBuffer &bb) { m_inputBuffer = &bb; }
      void setOutputBuffer(osc::BitBuffer &bb) { m_outputBuffer = &bb; }

      void setStateInformation(osc::u16 stateIdLength, osc::u16 stateLength);
      void addInput(osc::byte_t *, size_t);
      osc::StateChanges *getProposedStates();

      bool isFailed(){ return m_failureReason != OK; }
      bool isComplete(){ return m_messageComplete; }

      osc::SigcompMessage * getNack(osc::SigcompMessage &);
      osc::nack_code_t getNackCode(){ return m_failureReason; }

      // Debugging methods
      char *getNackFile() { return m_failureFile; }
      int getNackLine() { return m_failureLine; }

    protected:

    private:
      /* if you define these, move them to public */
      Udvm(Udvm const &);
      Udvm& operator=(Udvm const &);

      /* Handling of SigcompMessages */
      void reset();

      void loadCode(osc::u16 position,
                    const osc::byte_t *bytes,
                    size_t size);

      void loadState(const osc::byte_t *stateId,
                     osc::u16 idLength);

      /* Parameter Decoding */
      u16 getLiteralParameter();
      u16 getReferenceParameter();
      u16 getMultitypeParameter(osc::u16 *ptr = 0, bool *pointerType = 0);
      u16 getAddressParameter() { return getMultitypeParameter() + m_pc; }

      /* Convenience functions for memory access */
      void setWord(u16 location, u16 value);
      u16 getWord(u16 location);
      void pushStack(u16 value);
      u16 popStack();

      /* Copies memory, using byte copy rules */
      u16 byteCopy(u16 destination, const byte_t *source, size_t length);
      byte_t *byteCopy(byte_t *destination, u16 source, size_t length);
      u16 byteCopy(u16 destination, u16 source, size_t length);

      /* Adjusts cycles remaining */
      bool consumeCycles(int cycles);

      /** Register Locations */
      enum
      {
        UDVM_MEMORY_SIZE        = 0,
        CYCLES_PER_BIT          = 2,
        SIGCOMP_VERSION         = 4,
        PARTIAL_STATE_ID_LENGTH = 6,
        STATE_LENGTH            = 8,
        RESERVED                = 10,
        BYTE_COPY_LEFT          = 64,
        BYTE_COPY_RIGHT         = 66,
        INPUT_BIT_ORDER         = 68,
        STACK_LOCATION          = 70
      };

      /** Masks for INPUT_BIT_ORDER */
      enum
      {
        /** Indicates the order in which the INPUT-BITS instruction
            interprets bits. If set, they are interpreted LSB first;
            otherwise MSB first */
        F_BIT         = 0x0004,
        /** Indicates the order in which the INPUT-HUFFMAN instruction
            interprets bits. If set, they are interpreted LSB first;
            otherwise MSB first */
        H_BIT         = 0x0002,
        /** Indicates the order in which bits are passed from the
            dispatcher to the input bit instructions. If set,
            they are passed LSB first; otherwise, MSB first. */
        P_BIT         = 0x0001,
        RESERVED_BITS = 0xFFF8
      };

      /** Masks for requested feedback header byte */
      enum
      {
        /** Set if requested feedback item is present */
        Q_BIT = 0x0004,

        /** Set if compartment is to be closed */
        S_BIT = 0x0002,

        /** Set if advertised states are no longer interesting */
        I_BIT = 0x0001
      };

      /* Opcodes */
      void opcode_decompression_failure();
      void opcode_and(u16 operand_1, u16 operand_2);
      void opcode_or(u16 operand_1, u16 operand_2);
      void opcode_not(u16 operand_1);
      void opcode_lshift(u16 operand_1, u16 operand_2);
      void opcode_rshift(u16 operand_1, u16 operand_2);
      void opcode_add(u16 operand_1, u16 operand_2);
      void opcode_subtract(u16 operand_1, u16 operand_2);
      void opcode_multiply(u16 operand_1, u16 operand_2);
      void opcode_divide(u16 operand_1, u16 operand_2);
      void opcode_remainder(u16 operand_1, u16 operand_2);
      void opcode_sort_ascending(u16 start, u16 n, u16 k);
      void opcode_sort_descending(u16 start, u16 n, u16 k);
      void opcode_sha_1(u16 position, u16 length, u16 destination);
      void opcode_load(u16 address, u16 value);
      void opcode_multiload(u16 address, u16 n);
      void opcode_push(u16 value);
      void opcode_pop(u16 address);
      void opcode_copy(u16 position, u16 length, u16 destination);
      void opcode_copy_literal(u16 position, u16 length, u16 destination);
      void opcode_copy_offset(u16 offset, u16 length, u16 destination);
      void opcode_memset(u16 address, u16 length, u16 start_value, u16 offset);
      void opcode_jump(u16 address);

      void opcode_compare(u16 value_1, u16 value_2, 
                          u16 address_1, u16 address_2, u16 address_3);

      void opcode_call(u16 address);
      void opcode_return();
      void opcode_switch(u16 n, u16 j);
      void opcode_crc(u16 value, u16 position, u16 length, u16 address);
      void opcode_input_bytes(u16 length, u16 destination, u16 address);
      void opcode_input_bits(u16 length, u16 destination, u16 address);
      void opcode_input_huffman(u16 destination, u16 address, u16 n);

      void opcode_state_access(u16 partial_identifier_start,
                               u16 partial_identifier_length,
                               u16 state_begin,
                               u16 state_length,
                               u16 state_address,
                               u16 state_instruction);

      void opcode_state_create(u16 state_length, 
                               u16 state_address,
                               u16 state_instruction,
                               u16 minimum_access_length,
                               u16 state_retention_priority);

      void opcode_state_free(u16 partial_identifier_start,
                             u16 partial_identifier_length);

      void opcode_output(u16 output_start, u16 output_length);

      void opcode_end_message(u16 requested_feedback_location,
                              u16 returned_parameters_location,
                              u16 state_length,
                              u16 state_address,
                              u16 state_instruction,
                              u16 minimum_access_length,
                              u16 state_retention_priority);

      osc::u16 ceilingLog2(osc::u16);

      osc::StateHandler &m_stateHandler;
      osc::byte_t *m_memory;
      osc::u32 m_memorySize;
      osc::u32 m_decompressionMemorySize;
      osc::u32 m_messageSize;

      osc::BitBuffer *m_inputBuffer;
      osc::BitBuffer *m_outputBuffer;

      osc::u16 m_nextParameter;
      osc::u16 m_pc;
      osc::u32 m_cycleAllowance;
      osc::u32 m_cyclesUsed;

      osc::StateChanges *m_stateChanges;

      bool m_messageComplete;

      // Failure information
      osc::nack_code_t m_failureReason;
      osc::byte_t      m_failureDetails[20];
      size_t           m_failureDetailLength; 
      osc::byte_t      m_failureOpcode;
      osc::u16         m_failurePc;
      int              m_failureLine;
      char *           m_failureFile;
  };
}

#endif
