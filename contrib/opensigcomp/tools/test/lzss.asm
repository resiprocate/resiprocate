at (32)

:index                          pad (2)
:length_value                   pad (2)
:old_pointer                    pad (2)

at (42)

set (requested_feedback_location, 0)

at (64)

:byte_copy_left                 pad (2)
:byte_copy_right                pad (2)
:input_bit_order                pad (2)
:decompressed_pointer           pad (2)

set (returned_parameters_location, 0)

align (64)

:initialize_memory

set (udvm_memory_size, 8192)
set (state_length, (udvm_memory_size - 64))

MULTILOAD (64, 4, circular_buffer, udvm_memory_size, 0,
circular_buffer)

:decompress_sigcomp_message

:next_character

INPUT-HUFFMAN (index, end_of_message, 2, 9, 0, 255, 16384, 4, 4096,
8191, 1)
COMPARE ($index, 8192, length, end_of_message, literal)

:literal

set (index_lsb, (index + 1))

OUTPUT (index_lsb, 1)
COPY-LITERAL (index_lsb, 1, $decompressed_pointer)
JUMP (next_character)

:length

INPUT-BITS (4, length_value, !)
ADD ($length_value, 3)
LOAD (old_pointer, $decompressed_pointer)
COPY-OFFSET ($index, $length_value, $decompressed_pointer)
OUTPUT ($old_pointer, $length_value)
JUMP (next_character)

:end_of_message

END-MESSAGE (requested_feedback_location,
returned_parameters_location, state_length, 64,
decompress_sigcomp_message, 6, 0)

:circular_buffer

