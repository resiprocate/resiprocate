#ifndef __OSC__DISASSEMBLER
#define __OSC__DISASSEMBLER 1

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

#include "Udvm.h"
#include "StateHandler.h"
#include <ostream>
#include <set>

namespace osc
{
  /**
    @todo Fill in description of Disassembler

    @todo Ideally, the opcodeData map should include
          a boolean for each parameter that indicates
          whether the value should be interpreted as 
          a literal or an address. So, for example,
          the first parameter of INPUT-HUFFMAN
          would be marked in such a way that it is
          disassembled as an address instead of an
          integer.
  */

  class Disassembler
  {
    public:
      Disassembler(byte_t *buffer);
      virtual ~Disassembler();

      void disassemble (std::ostream &os);
 
      Disassembler * operator &(){ return this; }
      Disassembler const * operator &() const { return this; }

    protected:

    private:
      /* if you define these, move them to public */
      Disassembler(Disassembler const &);
      Disassembler& operator=(Disassembler const &);

      enum parameter_t
      {
        LITERAL, REFERENCE, MULTITYPE, ADDRESS
      };

      enum {OPCODE_COUNT = 36};

      typedef struct
      {
        char *name;
        int fixedParameters;
        int repeatingParameters;
        parameter_t types[7];
      } opcode_data_t;

      static opcode_data_t opcodeData[OPCODE_COUNT];

      StateHandler m_sh;
      Udvm m_udvm;
      size_t m_codeLen;
      u16 m_startAddress;

      u16 streamParameter(std::ostream &os, parameter_t);
      u16 extractSymbol(parameter_t);
      void registerSymbol(u16 address);
      void findSymbols ();
      void generateOutput (std::ostream &os);
      void insertSymbol(std::ostream &, osc::u16);

      std::set<osc::u16> symbols;
  };
}

#endif
