at (32)

; The following 2-byte variables are stored in the scratch-pad memory
; area because they do not need to be saved after decompressing a
; SigComp message:

:length_value                   pad (2)
:position_value                 pad (2)
:index                          pad (2)
:extra_extension_bits           pad (2)
:codebook_old                   pad (2)

at (42)

set (requested_feedback_location, 0)

at (64)

; UDVM_registers

:byte_copy_left                 pad (2)
:byte_copy_right                pad (2)

:input_bit_order                pad (2)

; The following 2-byte variables are saved as state after
; decompressing a SigComp message:

:current_length                 pad (2)
:decompressed_pointer           pad (2)
:ordinal_length                 pad (2)
:codeword_length                pad (2)
:codebook_next                  pad (2)

set (returned_parameters_location, 0)

align (64)

:initialize_memory

; The following constants can be adjusted to configure the LZJH
; decompressor. The current settings are as recommended in the V.44
; specification (given that a total of 8K UDVM memory is available):

set (udvm_memory_size, 8192)   ; sets the total memory for LZJH
set (max_extension_length, 8)  ; sets the maximum string extension
set (min_ordinal_length, 7)    ; sets the minimum ordinal length
set (min_codeword_length, 6)   ; sets the minimum codeword length

set (codebook_start, 4492)
set (first_codeword, (codebook_start - 12))
set (state_length, (udvm_memory_size - 64))

MULTILOAD (64, 8, circular_buffer, udvm_memory_size, 7, 0,
circular_buffer, min_ordinal_length, min_codeword_length,
codebook_start)

:decompress_sigcomp_message

:standard_prefix

; The following code decompresses the standard 1-bit LZJH prefix
; which specifies whether the next character is an ordinal or a
; codeword/control value:

INPUT-BITS (1, index, end_of_message)
COMPARE ($index, 1, ordinal, codeword_control, codeword_control)

:prefix_after_codeword

; The following code decompresses the special LZJH prefix that only
; occurs after a codeword. It specifies whether the next character is
; an ordinal, a codeword/control value, or a string extension:

INPUT-HUFFMAN (index, end_of_message, 2, 1, 1, 1, 2, 1, 0, 1, 0)
COMPARE ($index, 1, ordinal, string_extension, codeword_control)

:ordinal

; The following code decompresses an ordinal character, and creates
; a new codebook entry consisting of the ordinal character plus the
; next character to be decompressed:

set (index_lsb, (index + 1))
set (current_length_lsb, (current_length + 1))

INPUT-BITS ($ordinal_length, index, !)
OUTPUT (index_lsb, 1)
LOAD (current_length, 2)
COPY-LITERAL (current_length_lsb, 3, $codebook_next)
COPY-LITERAL (index_lsb, 1, $decompressed_pointer)
JUMP (standard_prefix)

:codeword_control

; The following code decompresses a codeword/control value:

INPUT-BITS ($codeword_length, index, !)
COMPARE ($index, 3, control_code, initialize_memory, codeword)

:codeword

; The following code interprets a codeword as an index into the LZJH
; codebook. It extracts the position/length pair from the specified
; codebook entry; the position/length pair points to a byte string
; in the circular buffer which is then copied to the end of the
; decompressed message. The code also creates a new codebook entry
; consisting of the byte string plus the next character to be
; decompressed:

set (length_value_lsb, (length_value + 1))

MULTIPLY ($index, 3)
ADD ($index, first_codeword)
COPY ($index, 3, length_value_lsb)
LOAD (current_length, 1)
ADD ($current_length, $length_value)
LOAD (codebook_old, $codebook_next)
COPY-LITERAL (current_length_lsb, 3, $codebook_next)
COPY-LITERAL ($position_value, $length_value, $decompressed_pointer)
OUTPUT ($position_value, $length_value)
JUMP (prefix_after_codeword)

:string_extension

; The following code decompresses a Huffman-encoded string extension:

INPUT-HUFFMAN (index, !, 4, 1, 1, 1, 1, 2, 1, 3, 2, 1, 1, 1, 13, 3,
0, 7, 5)
COMPARE ($index, 13, continue, extra_bits, extra_bits)

:extra_bits

INPUT-BITS (max_extension_length, extra_extension_bits, !)
ADD ($index, $extra_extension_bits)

:continue

; The following code extends the most recently created codebook entry
; by the number of bits specified in the string extension:

COPY-LITERAL ($position_value, $length_value, $position_value)
COPY-LITERAL ($position_value, $index, $decompressed_pointer)
OUTPUT ($position_value, $index)
ADD ($index, $length_value)
COPY (index_lsb, 1, $codebook_old)
JUMP (standard_prefix)

:control_code

; The code can handle all of the control characters in V.44 except
; for ETM (Enter Transparent Mode), which is not required for
; message-based protocols such as SigComp.

COMPARE ($index, 1, !, flush, stepup)

:flush

; The FLUSH control character jumps to the beginning of the next
; complete byte in the compressed message:

INPUT-BYTES (0, 0, 0)
JUMP (standard_prefix)

:stepup

; The STEPUP control character increases the number of bits used to
; encode an ordinal value or a codeword:

INPUT-BITS (1, index, !)
COMPARE ($index, 1, stepup_ordinal, stepup_codeword, 0)

:stepup_ordinal

ADD ($ordinal_length, 1)
JUMP (ordinal)

:stepup_codeword

ADD ($codeword_length, 1)
JUMP (codeword_control)

:end_of_message

END-MESSAGE (requested_feedback_location,
returned_parameters_location, state_length, 64,
decompress_sigcomp_message, 6, 0)

:circular_buffer
