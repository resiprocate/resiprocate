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

#include <iostream>
#include <iomanip>
#include <assert.h>
#include "Disassembler.h"

osc::Disassembler::opcode_data_t
osc::Disassembler::opcodeData[OPCODE_COUNT] =
{
  // Opcode 
  {
    "DECOMPRESSION-FAILURE",
    0,             // fixedParameters
    0,             // repeatingParameters
    {
    }
  },

  // Opcode 1
  {
    "AND",
    2,             // fixedParameters
    0,             // repeatingParameters
    {
      REFERENCE,   // $operand_1
      MULTITYPE    // %operand_2
    }
  },

  // Opcode 2
  {
    "OR",
    2,             // fixedParameters
    0,             // repeatingParameters
    {
      REFERENCE,   // $operand_1
      MULTITYPE    // %operand_2
    }
  },

  // Opcode 3
  {
    "NOT",
    1,             // fixedParameters
    0,             // repeatingParameters
    {
      REFERENCE    // $operand_1
    }
  },

  // Opcode 4
  {
    "LSHIFT",
    2,             // fixedParameters
    0,             // repeatingParameters
    {
      REFERENCE,   // $operand_1
      MULTITYPE    // %operand_2
    }
  },

  // Opcode 5
  {
    "RSHIFT",
    2,             // fixedParameters
    0,             // repeatingParameters
    {
      REFERENCE,   // $operand_1
      MULTITYPE    // %operand_2
    }
  },

  // Opcode 6
  {
    "ADD",
    2,             // fixedParameters
    0,             // repeatingParameters
    {
      REFERENCE,   // $operand_1
      MULTITYPE    // %operand_2
    }
  },

  // Opcode 7
  {
    "SUBTRACT",
    2,             // fixedParameters
    0,             // repeatingParameters
    {
      REFERENCE,   // $operand_1
      MULTITYPE    // %operand_2
    }
  },

  // Opcode 8
  {
    "MULTIPLY",
    2,             // fixedParameters
    0,             // repeatingParameters
    {
      REFERENCE,   // $operand_1
      MULTITYPE    // %operand_2
    }
  },

  // Opcode 9
  {
    "DIVIDE",
    2,             // fixedParameters
    0,             // repeatingParameters
    {
      REFERENCE,   // $operand_1
      MULTITYPE    // %operand_2
    }
  },

  // Opcode 10
  {
    "REMAINDER",
    2,             // fixedParameters
    0,             // repeatingParameters
    {
      REFERENCE,   // $operand_1
      MULTITYPE    // %operand_2
    }
  },

  // Opcode 11
  {
    "SORT-ASCENDING",
    3,             // fixedParameters
    0,             // repeatingParameters
    {
      MULTITYPE,   // %start
      MULTITYPE,   // %n
      MULTITYPE    // %k
    }
  },

  // Opcode 12
  {
    "SORT-DESCENDING",
    3,             // fixedParameters
    0,             // repeatingParameters
    {
      MULTITYPE,   // %start
      MULTITYPE,   // %n
      MULTITYPE    // %k
    }
  },

  // Opcode 13
  {
    "SHA-1",
    3,             // fixedParameters
    0,             // repeatingParameters
    {
      MULTITYPE,   // %position
      MULTITYPE,   // %length
      MULTITYPE    // %destination
    }
  },

  // Opcode 14
  {
    "LOAD",
    2,             // fixedParameters
    0,             // repeatingParameters
    {
      MULTITYPE,   // %address
      MULTITYPE    // %value
    }
  },

  // Opcode 15
  {
    "MULTILOAD",
    2,             // fixedParameters
    1,             // repeatingParameters
    {
      MULTITYPE,   // %address
      LITERAL,     // #n
      MULTITYPE    // %value_0
                   //...   
                   // %value_n-1
    }
  },

  // Opcode 16
  {
    "PUSH",
    1,             // fixedParameters
    0,             // repeatingParameters
    {
      MULTITYPE    // %value
    }
  },

  // Opcode 17
  {
    "POP",
    1,             // fixedParameters
    0,             // repeatingParameters
    {
      MULTITYPE    // %address
    }
  },

  // Opcode 18
  {
    "COPY",
    3,             // fixedParameters
    0,             // repeatingParameters
    {
      MULTITYPE,   // %position
      MULTITYPE,   // %length
      MULTITYPE    // %destination
    }
  },

  // Opcode 19
  {
    "COPY-LITERAL",
    3,             // fixedParameters
    0,             // repeatingParameters
    {
      MULTITYPE,   // %position
      MULTITYPE,   // %length
      REFERENCE    // $destination
    }
  },

  // Opcode 20
  {
    "COPY-OFFSET",
    3,             // fixedParameters
    0,             // repeatingParameters
    {
      MULTITYPE,   // %offset
      MULTITYPE,   // %length
      REFERENCE    // $destination
    }
  },

  // Opcode 21
  {
    "MEMSET",
    4,             // fixedParameters
    0,             // repeatingParameters
    {
      MULTITYPE,   // %address
      MULTITYPE,   // %length
      MULTITYPE,   // %start_value
      MULTITYPE    // %offset
    }
  },

  // Opcode 22
  {
    "JUMP",
    1,             // fixedParameters
    0,             // repeatingParameters
    {
      ADDRESS      // @address
    }
  },

  // Opcode 23
  {
    "COMPARE",
    5,             // fixedParameters
    0,             // repeatingParameters
    {
      MULTITYPE,   // %value_1
      MULTITYPE,   // %value_2
      ADDRESS,     // @address_1
      ADDRESS,     // @address_2
      ADDRESS      // @address_3
    }
  },

  // Opcode 24
  {
    "CALL",
    1,             // fixedParameters
    0,             // repeatingParameters
    {
      ADDRESS      // @address
    }
  },

  // Opcode 25
  {
    "RETURN",
    0,             // fixedParameters
    0,             // repeatingParameters
    {
    }
  },

  // Opcode 26
  {
    "SWITCH",
    2,             // fixedParameters
    1,             // repeatingParameters
    {
      LITERAL,     // #n
      MULTITYPE,   // %j
      ADDRESS      // @address_0
                   // @address_1
                   // ...
                   // @address_n-1
    }
  },

  // Opcode 27
  {
    "CRC",
    4,             // fixedParameters
    0,             // repeatingParameters
    {
      MULTITYPE,   // %value
      MULTITYPE,   // %position
      MULTITYPE,   // %length
      ADDRESS      // @address
    }
  },

  // Opcode 28
  {
    "INPUT-BYTES",
    3,             // fixedParameters
    0,             // repeatingParameters
    {
      MULTITYPE,   // %length
      MULTITYPE,   // %destination
      ADDRESS      // @address
    }
  },

  // Opcode 29
  {
    "INPUT-BITS",
    3,             // fixedParameters
    0,             // repeatingParameters
    {
      MULTITYPE,   // %length
      MULTITYPE,   // %destination
      ADDRESS      // @address
    }
  },

  // Opcode 30
  {
    "INPUT-HUFFMAN",
    3,             // fixedParameters
    4,             // repeatingParameters
    {
      MULTITYPE,   // %destination
      ADDRESS,     // @address
      LITERAL,     // #n
      MULTITYPE,   // %bits_1
      MULTITYPE,   // %lower_bound_1
      MULTITYPE,   // %upper_bound_1
      MULTITYPE    // %uncompressed_1
                   // ...
                   // %bits_n
                   // %lower_bound_n
                   // %upper_bound_n
                   // %uncompressed_n
    }
  },

  // Opcode 31
  {
    "STATE-ACCESS",
    6,             // fixedParameters
    0,             // repeatingParameters
    {
      MULTITYPE,   // %partial_identifier_start
      MULTITYPE,   // %partial_identifier_length
      MULTITYPE,   // %state_begin
      MULTITYPE,   // %state_length
      MULTITYPE,   // %state_address
      MULTITYPE    // %state_instruction
    }
  },

  // Opcode 32
  {
    "STATE-CREATE",
    5,             // fixedParameters
    0,             // repeatingParameters
    {
      MULTITYPE,   // %state_length
      MULTITYPE,   // %state_address
      MULTITYPE,   // %state_instruction
      MULTITYPE,   // %minimum_access_length
      MULTITYPE    // %state_retention_priority
    }
  },

  // Opcode 33
  {
    "STATE-FREE",
    2,             // fixedParameters
    0,             // repeatingParameters
    {
      MULTITYPE,   // %partial_identifier_start
      MULTITYPE    // %partial_identifier_length
    }
  },

  // Opcode 34
  {
    "OUTPUT",
    2,             // fixedParameters
    0,             // repeatingParameters
    {
      MULTITYPE,   // %output_start
      MULTITYPE    // %output_length
    }
  },

  // Opcode 35
  {
    "END-MESSAGE",
    7,             // fixedParameters
    0,             // repeatingParameters
    {
      MULTITYPE,   // %requested_feedback_location
      MULTITYPE,   // %returned_parameters_location
      MULTITYPE,   // %state_length
      MULTITYPE,   // %state_address
      MULTITYPE,   // %state_instruction
      MULTITYPE,   // %minimum_access_length
      MULTITYPE    // %state_retention_priority
    }
  }
};


/**
  Constructor for osc::Disassembler.
 */
osc::Disassembler::Disassembler(byte_t *buffer)
  : m_sh(), m_udvm(m_sh, 65536)
{
  m_codeLen = (buffer[0] << 4) | ((buffer[1] >> 4) & 0x0f);
  m_startAddress = ((buffer[1] & 0x0f) + 1) * 64;
  memmove(m_udvm.m_memory + m_startAddress, buffer+2, m_codeLen);
}

/**
  Copy constructor for osc::Disassembler.
 */
osc::Disassembler::Disassembler(Disassembler const &r)
  : m_sh(), m_udvm(m_sh)
{
  /* Assign attributes */
  assert(0);
}

/**
  Destructor for osc::Disassembler.
 */
osc::Disassembler::~Disassembler()
{
}

/**
  Assignment operator for osc::Disassembler.
 */
osc::Disassembler &
osc::Disassembler::operator=(Disassembler const &r)
{
  if (&r == this)
  {
    return *this;
  }
  /* Assign attributes */
  assert(0);
  return *this;
}


osc::u16
osc::Disassembler::streamParameter(std::ostream &os, parameter_t parameterType)
{
  switch(parameterType)
  {
    case LITERAL:
       {
         int val;
         val = m_udvm.getLiteralParameter();
         os << val;
         return val;
         break;
       }

    case REFERENCE:
       os << "$";
       insertSymbol(os, m_udvm.getReferenceParameter());
       break;

    case MULTITYPE:
    case ADDRESS:
       {
         bool isPointer;
         osc::u16 pointer;
         osc::u16 val;
         val = m_udvm.getMultitypeParameter(&pointer, &isPointer);
         if (parameterType == ADDRESS)
         {
           val += m_udvm.m_pc;
         }
         if (isPointer)
         {
           os << "$";
           insertSymbol(os, pointer);
         }
         else
         {
           if (parameterType == ADDRESS)
           {
             insertSymbol(os, val);
           }
           else
           {
             os << val;
           }
         }
       }
       break;
  }
  return 0;
}

void
osc::Disassembler::registerSymbol(osc::u16 address)
{
  symbols.insert(address);
}

osc::u16
osc::Disassembler::extractSymbol(parameter_t parameterType)
{
  switch(parameterType)
  {
    case LITERAL:
       {
         int val;
         val = m_udvm.getLiteralParameter();
         return val;
         break;
       }

    case REFERENCE:
       registerSymbol(m_udvm.getReferenceParameter());
       break;

    case MULTITYPE:
    case ADDRESS:
       {
         bool isPointer;
         osc::u16 pointer;
         osc::u16 val;
         val = m_udvm.getMultitypeParameter(&pointer, &isPointer);
         if (parameterType == ADDRESS)
         {
           val += m_udvm.m_pc;
         }
         if (isPointer)
         {
           registerSymbol(pointer);
         }
         else if (parameterType == ADDRESS)
         {
           registerSymbol(val);
         }
       }
       break;
  }
  return 0;
}

void
osc::Disassembler::disassemble(std::ostream &os)
{
  findSymbols();
  generateOutput(os);
}

void
osc::Disassembler::findSymbols()
{
  opcode_data_t *data;
  int i;
  m_udvm.m_pc = m_startAddress;
  while (m_udvm.m_pc < m_codeLen + m_startAddress)
  {
    int repeatingParameterCount = 0;
    if (m_udvm.m_memory[m_udvm.m_pc] < OPCODE_COUNT)
    {
      data = &opcodeData[m_udvm.m_memory[m_udvm.m_pc]];
      m_udvm.m_nextParameter = m_udvm.m_pc+1;
      if (data->fixedParameters)
      {
        for (i = 0; i < data->fixedParameters; i++)
        {
          int tmp = extractSymbol(data->types[i]);
          if (data->types[i] == LITERAL)
          {
            repeatingParameterCount = tmp;
          }
        }

        while (repeatingParameterCount--)
        {
          for (i = 0; i < data->repeatingParameters; i++)
          {
            extractSymbol(data->types[data->fixedParameters+i]);
          }
        }
      }
      m_udvm.m_pc = m_udvm.m_nextParameter;
    }
    else
    {
      m_udvm.m_pc++;
    }
  }
}

void
osc::Disassembler::generateOutput(std::ostream &os)
{
  opcode_data_t *data;
  int i;
  m_udvm.m_pc = m_startAddress;
  int lastParam = -1;

  for (i = 0; i < m_startAddress; i++)
  {
    if (symbols.count(i))
    {
      if (lastParam >= 0)
      {
        os << "pad(" << (i - lastParam) << ")" << std::endl;
      }
      else
      {
        os << "at(" << (i) << ")" << std::endl << std::endl;
      }
      os << ":";
      insertSymbol(os, i);
      os << "  ";
      symbols.erase(i);
      lastParam = i;
    }
  }

  os << std::endl << std::endl 
     << "at(" << m_startAddress << ")" << std::endl << std::endl;
  while (m_udvm.m_pc < m_codeLen + m_startAddress)
  {
    int repeatingParameterCount = 0;
    /*
    os << "0x" << std::hex << std::setw(4) << std::setfill('0') << m_udvm.m_pc 
       << " [" << std::dec << std::setw(5) << std::setfill(' ') << m_udvm.m_pc 
       << "]  ";
    */
    if (symbols.count(m_udvm.m_pc))
    {
      os << std::endl << ":";
      insertSymbol(os, m_udvm.m_pc);
      os << std::endl;

      symbols.erase(m_udvm.m_pc);
    }
    if (m_udvm.m_memory[m_udvm.m_pc] < OPCODE_COUNT)
    {
      data = &opcodeData[m_udvm.m_memory[m_udvm.m_pc]];
      os << data->name;
      m_udvm.m_nextParameter = m_udvm.m_pc+1;
      if (data->fixedParameters)
      {
        os << " (";
        for (i = 0; i < data->fixedParameters; i++)
        {
          os << " ";
          int tmp = streamParameter(os, data->types[i]);
          if (data->types[i] == LITERAL)
          {
            repeatingParameterCount = tmp;
          }
          if (!(i == data->fixedParameters - 1) 
              || data->repeatingParameters)
          {
            os << ",";
          }
        }

        while (repeatingParameterCount--)
        {
          for (i = 0; i < data->repeatingParameters; i++)
          {
            os << " ";
            streamParameter(os, data->types[data->fixedParameters+i]);

            if (!(i == data->repeatingParameters - 1) 
                || repeatingParameterCount)
            {
              os << ",";
            }

          }
        }

        os << " )";
      }
      m_udvm.m_pc = m_udvm.m_nextParameter;
      
    }
    else
    {
      os << "byte(0x" << std::hex << std::setw(2) << std::setfill('0')
         << static_cast<int>(m_udvm.m_memory[m_udvm.m_pc]) << std::dec << ")";
      m_udvm.m_pc++;
    }
    os << std::endl;
  }

  os << std::endl;
  for(std::set<osc::u16>::iterator k = symbols.begin();
      k != symbols.end(); k++)
  {
    os << "set(";
    insertSymbol(os, *k);
    os << ", "<< (*k) << ")" << std::endl;
  }

  os << std::endl << "; End address = " << m_udvm.m_pc << std::endl;
}

void
osc::Disassembler::insertSymbol(std::ostream &os, osc::u16 address)
{
  switch (address)
  {
    case 0: os << "udvm_memory_size"; break;
    case 2: os << "cycles_per_bit"; break;
    case 4: os << "sigcomp_version"; break;
    case 6: os << "partial_id_state_length"; break;
    case 8: os << "state_length"; break;
    case 64: os << "byte_copy_left"; break;
    case 66: os << "byte_copy_right"; break;
    case 68: os << "input_bit_order"; break;
    case 70: os << "stack_location"; break;
    default: os << "label" << address; break;
  }
}
