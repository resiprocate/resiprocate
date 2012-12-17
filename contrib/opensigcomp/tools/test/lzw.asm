at (32)

:length_value                   pad (2)
:position_value                 pad (2)
:index                          pad (2)

at (42)

set (requested_feedback_location, 0)

at (64)

:byte_copy_left                 pad (2)
:byte_copy_right                pad (2)
:input_bit_order                pad (2)

:codebook_next                  pad (2)
:current_length                 pad (2)
:decompressed_pointer           pad (2)

set (returned_parameters_location, 0)

align (64)

:initialize_memory

set (udvm_memory_size, 8192)
set (state_length, (udvm_memory_size - 64))

MULTILOAD (64, 6, circular_buffer, udvm_memory_size, 0, codebook, 1,
static_dictionary)

:initialize_codebook

; The following instructions are used to initialize the first 256
; entries in the LZW codebook with single ASCII characters:


set (index_lsb, (index + 1))
set (current_length_lsb, (current_length + 1))

COPY-LITERAL (current_length_lsb, 3, $codebook_next)
COPY-LITERAL (index_lsb, 1, $decompressed_pointer)
ADD ($index, 1)
COMPARE ($index, 256, initialize_codebook, next_character, 0)

:decompress_sigcomp_message

:next_character

; The following INPUT-BITS instruction extracts 10 bits from the
; compressed message:

INPUT-BITS (10, index, end_of_message)

; The following instructions interpret the received bits as an index
; into the LZW codebook, and extract the corresponding
; position/length pair:

set (length_value_lsb, (length_value + 1))

MULTIPLY ($index, 3)
ADD ($index, codebook)
COPY ($index, 3, length_value_lsb)

; The following instructions append the selected text string to the
; circular buffer and create a new codebook entry pointing to this
; text string:

LOAD (current_length, 1)
ADD ($current_length, $length_value)
COPY-LITERAL (current_length_lsb, 3, $codebook_next)
COPY-LITERAL ($position_value, $length_value, $decompressed_pointer)

; The following instruction outputs the text string specified by the
; position/length pair:

OUTPUT ($position_value, $length_value)
JUMP (next_character)

:end_of_message

END-MESSAGE (requested_feedback_location,
returned_parameters_location, state_length, 64,
decompress_sigcomp_message, 6, 0)

:static_dictionary              pad (256)
:circular_buffer

at (4492)

:codebook
