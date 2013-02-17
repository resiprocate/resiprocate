; ***********************************************************************
;  Open SigComp -- Implementation of RFC 3320 Signaling Compression
; 
;  Copyright 2005 Estacado Systems, LLC
; 
;  Your use of this code is governed by the license under which it
;  has been provided to you. Unless you have a written and signed
;  document from Estacado Systems, LLC stating otherwise, your license
;  is as provided by the GNU General Public License version 2, a copy
;  of which is available in this project in the file named "LICENSE."
;  Alternately, a copy of the licence is available by writing to
;  the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
;  Boston, MA 02110-1307 USA
; 
;  Unless your use of this code is goverened by a written and signed
;  contract containing provisions to the contrary, this program is
;  distributed WITHOUT ANY WARRANTY; without even the implied warranty
;  of MERCHANTABILITY of FITNESS FOR A PARTICULAR PURPOSE. See the
;  license for additional details.
; 
;  To discuss alternate licensing terms, contact info@estacado.net
; ***********************************************************************

; Useful values
:uv_memory_size                 pad (2)
:uv_cycles_per_bit              pad (2)
:uv_sigcomp_version             pad (2)
:uv_parial_state_id_len         pad (2)
:uv_state_length                pad (2)

at (32)

:index                          pad (2)
:extra_length_bits              pad (2)
:length_value                   pad (2)
:extra_distance_bits            pad (2)
:distance_value                 pad (2)
:temp                           pad (2)
:state_length                   pad (2)

at (64)

set(state_address, byte_copy_left)

:byte_copy_left                 pad (2)
:byte_copy_right                pad (2)
:input_bit_order                pad (2)

:decompressed_pointer           pad (2)

:length_table                   pad (116)
:distance_table                 pad (120)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Requested feedback
; The requested feedback conveys information back about
; which messages have been sucessfully decompressed.

:requested_feedback_location    pad (2)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Returned feedback
; The returned parameters tell what our capabilities are

set (returned_parameters_location, cpb_dms_sms)

:cpb_dms_sms                    pad (1)
:sigcomp_version                pad (1)
:sip_state_id_length            pad (1)
:sip_state_id                   pad (6)

align (64)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Set up initial values

:initialize_memory

set (length_table_start, (((length_table - 4) / 4) + 16384))
set (length_table_mid, (length_table_start + 24))
set (distance_table_start, (distance_table / 4))

MULTILOAD (64, 124, circular_buffer, 0, 0,
           circular_buffer,

; Length Table
  0,       3,       0,       4,       0,       5,
  0,       6,       0,       7,       0,       8,
  0,       9,       0,       10,      1,       11,
  1,       13,      1,       15,      1,       17,
  2,       19,      2,       23,      2,       27,
  2,       31,      3,       35,      3,       43,
  3,       51,      3,       59,      4,       67,
  4,       83,      4,       99,      4,       115,
  5,       131,     5,       163,     5,       195,
  5,       227,     0,       258,

; Distance Table  
  0,       1,       0,       2,       0,       3,
  0,       4,       1,       5,       1,       7,
  2,       9,       2,       13,      3,       17,
  3,       25,      4,       33,      4,       49,
  5,       65,      5,       97,      6,       129,
  6,       193,     7,       257,     7,       385,
  8,       513,     8,       769,     9,       1025,
  9,       1537,    10,      2049,    10,      3073,
  11,      4097,    11,      6145,    12,      8193,
  12,      12289,   13,      16385,   13,      24577,

; Requested Feedback
  0x0400,

; Sigcomp Version
  2
)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Load in capabilities from input stream

INPUT-BYTES(1, cpb_dms_sms, !)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Read flag: is SIP SigComp dictionary being advertized?

:decompress_sigcomp_message
INPUT-BITS(1, sip_state_id_length, !)
COMPARE($sip_state_id_length, 0, !, set_buffer_size, load_sip_dict_id)

:load_sip_dict_id
MULTILOAD(sip_state_id_length, 4, 0x06FB, 0xE507, 0xDFE5, 0xE600)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Read in buffer size and set up circular buffer

:set_buffer_size
LOAD(byte_copy_right, 512)
INPUT-BITS(3, temp, !)
LSHIFT($byte_copy_right, $temp)
ADD($byte_copy_right, circular_buffer)
LOAD(state_length, $byte_copy_right)
SUBTRACT($state_length, state_address)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Read in serial number
INPUT-BITS (4, requested_feedback_location, !)
OR ($requested_feedback_location, 0x0400)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Here starts the loop to read in deflated data

:next_character
INPUT-HUFFMAN (index, end_of_message, 4,
               7, 0, 23, length_table_start,
               1, 48, 191, 0,
               0, 192, 199, length_table_mid,
               1, 400, 511, 144)

COMPARE ($index, length_table_start, literal, end_of_message,
         length_distance)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Output a literal byte value

:literal

set (index_lsb, (index + 1))

OUTPUT (index_lsb, 1)
COPY-LITERAL (index_lsb, 1, $decompressed_pointer)
JUMP (next_character)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Output the result of a length/value pair

:length_distance

; this is the length part

MULTIPLY ($index, 4)
COPY ($index, 4, extra_length_bits)
INPUT-BITS ($extra_length_bits, extra_length_bits, !)
ADD ($length_value, $extra_length_bits)

; this is the distance part

INPUT-BITS(5, index, !)
ADD($index, distance_table_start)
MULTIPLY ($index, 4)
COPY ($index, 4, extra_distance_bits)

INPUT-BITS ($extra_distance_bits, extra_distance_bits, !)
ADD ($distance_value, $extra_distance_bits)
LOAD (index, $decompressed_pointer)
COPY-OFFSET ($distance_value, $length_value, $decompressed_pointer)
OUTPUT ($index, $length_value)
JUMP (next_character)


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Indicate that message is complete

:end_of_message
END-MESSAGE (requested_feedback_location,
             returned_parameters_location,
             $state_length,
             state_address,
             decompress_sigcomp_message,
             6,
             0)

:circular_buffer
