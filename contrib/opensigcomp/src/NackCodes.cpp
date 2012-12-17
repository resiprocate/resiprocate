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
  @file NackCodes.cpp
  @brief Declaration of strings corresponding to various NACK codes.

  These are available only when the library is compiled in a
  debugging configuration.
*/

#include "NackCodes.h"

#ifdef DEBUG
char *osc::s_nackCode[] =
{
   "OK",
   "STATE_NOT_FOUND",
   "CYCLES_EXHAUSTED",
   "USER_REQUESTED",
   "SEGFAULT",
   "TOO_MANY_STATE_REQUESTS",
   "INVALID_STATE_ID_LENGTH",
   "INVALID_STATE_PRIORITY",
   "OUTPUT_OVERFLOW",
   "STACK_UNDERFLOW",
   "BAD_INPUT_BITORDER",
   "DIV_BY_ZERO",
   "SWITCH_VALUE_TOO_HIGH",
   "TOO_MANY_BITS_REQUESTED",
   "INVALID_OPERAND",
   "HUFFMAN_NO_MATCH",
   "MESSAGE_TOO_SHORT",
   "INVALID_CODE_LOCATION",
   "BYTECODES_TOO_LARGE",
   "INVALID_OPCODE",
   "INVALID_STATE_PROBE",
   "ID_NOT_UNIQUE",
   "MULTILOAD_OVERWRITTEN",
   "STATE_TOO_SHORT",
   "INTERNAL_ERROR",
   "FRAMING_ERROR"
};
#endif
