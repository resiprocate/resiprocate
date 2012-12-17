#ifndef __OSC__NACK_CODES
#define __OSC__NACK_CODES 1

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
  @file NackCodes.h
  @brief Deinition of NACK error codes defined in RFC 4077.
*/


namespace osc
{
  enum nack_code_t
  {
    OK                        =  0,
    STATE_NOT_FOUND           =  1,
    CYCLES_EXHAUSTED          =  2,
    USER_REQUESTED            =  3,
    SEGFAULT                  =  4,
    TOO_MANY_STATE_REQUESTS   =  5,
    INVALID_STATE_ID_LENGTH   =  6,
    INVALID_STATE_PRIORITY    =  7,
    OUTPUT_OVERFLOW           =  8,
    STACK_UNDERFLOW           =  9,
    BAD_INPUT_BITORDER        = 10,
    DIV_BY_ZERO               = 11,
    SWITCH_VALUE_TOO_HIGH     = 12,
    TOO_MANY_BITS_REQUESTED   = 13,
    INVALID_OPERAND           = 14,
    HUFFMAN_NO_MATCH          = 15,
    MESSAGE_TOO_SHORT         = 16,
    INVALID_CODE_LOCATION     = 17,
    BYTECODES_TOO_LARGE       = 18,
    INVALID_OPCODE            = 19,
    INVALID_STATE_PROBE       = 20,
    ID_NOT_UNIQUE             = 21,
    MULTILOAD_OVERWRITTEN     = 22,
    STATE_TOO_SHORT           = 23,
    INTERNAL_ERROR            = 24,
    FRAMING_ERROR             = 25,

    END_OF_NACK_CODE_LIST
  };

#ifdef DEBUG
  extern char *s_nackCode[END_OF_NACK_CODE_LIST];
#endif
}

#endif
