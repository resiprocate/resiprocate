at (32)

:index                          pad (2)
:distance_value                 pad (2)
:old_pointer                    pad (2)

at (42)

set (requested_feedback_location, 0)

at (64)

:byte_copy_left                 pad (2)
:byte_copy_right                pad (2)
:input_bit_order                pad (2)
:decompressed_pointer           pad (2)

set (returned_parameters_location, 0)

at (128)

:initialize_memory

set (udvm_memory_size, 8192)
set (state_length, (udvm_memory_size - 64))

MULTILOAD (64, 4, circular_buffer, udvm_memory_size, 0,
circular_buffer)

:decompress_sigcomp_message

:character_after_literal

INPUT-HUFFMAN (index, end_of_message, 16,
    5, 0, 11, 46,
    0, 12, 12, 256,
    1, 26, 32, 257,
    1, 66, 68, 32,
    0, 69, 94, 97,
    0, 95, 102, 264,
    0, 103, 103, 511,
    2, 416, 426, 35,
    0, 427, 465, 58,
    0, 466, 481, 272,
    1, 964, 995, 288,
    3, 7968, 7988, 123,
    0, 7989, 8115, 384,
    1, 16232, 16263, 0,
    0, 16264, 16327, 320,
    1, 32656, 32767, 144)

COMPARE ($index, 256, literal, distance, distance)

:character_after_match

INPUT-HUFFMAN (index, end_of_message, 16,
    4, 0, 0, 511,
    1, 2, 9, 256,
    1, 20, 22, 32,
    0, 23, 30, 264,
    1, 62, 73, 46,
    0, 74, 89, 272,
    2, 360, 385, 97,
    0, 386, 417, 288,
    1, 836, 874, 58,
    0, 875, 938, 320,
    1, 1878, 1888, 35,
    0, 1889, 2015, 384,
    1, 4032, 4052, 123,
    1, 8106, 8137, 0,
    1, 16276, 16379, 144,
    1, 32760, 32767, 248)

COMPARE ($index, 256, literal, distance, distance)

:literal

set (index_lsb, (index + 1))

OUTPUT (index_lsb, 1)
COPY-LITERAL (index_lsb, 1, $decompressed_pointer)
JUMP (character_after_literal)

:distance

SUBTRACT ($index, 253)
INPUT-HUFFMAN (distance_value, !, 9,
    9, 0, 7, 9,
    0, 8, 63, 129,
    1, 128, 135, 1,
    0, 136, 247, 17,
    0, 248, 319, 185,
    1, 640, 1407, 257,
    2, 5632, 6655, 1025,
    1, 13312, 15359, 2049,
    2, 61440, 65535, 4097)

LOAD (old_pointer, $decompressed_pointer)
COPY-OFFSET ($distance_value, $index, $decompressed_pointer)
OUTPUT ($old_pointer, $index)
JUMP (character_after_match)

:end_of_message

END-MESSAGE (requested_feedback_location,
returned_parameters_location, state_length, 64,
decompress_sigcomp_message, 6, 0)

:circular_buffer

