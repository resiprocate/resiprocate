at (32)

:index                          pad (2)
:extra_length_bits              pad (2)
:length_value                   pad (2)
:extra_distance_bits            pad (2)
:distance_value                 pad (2)

at (42)

set (requested_feedback_location, 0)

at (64)

:byte_copy_left                 pad (2)
:byte_copy_right                pad (2)
:input_bit_order                pad (2)

:decompressed_pointer           pad (2)

:length_table                   pad (116)
:distance_table                 pad (120)

set (returned_parameters_location, 0)

align (64)

:initialize_memory

set (udvm_memory_size, 8192)
set (state_length, (udvm_memory_size - 64))
set (length_table_start, (((length_table - 4) + 65536) / 4))
set (length_table_mid, (length_table_start + 24))
set (distance_table_start, (distance_table / 4))

MULTILOAD (64, 122, circular_buffer, udvm_memory_size, 5,
circular_buffer,

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

0,       1,       0,       2,       0,       3,
0,       4,       1,       5,       1,       7,
2,       9,       2,       13,      3,       17,
3,       25,      4,       33,      4,       49,
5,       65,      5,       97,      6,       129,
6,       193,     7,       257,     7,       385,
8,       513,     8,       769,     9,       1025,
9,       1537,    10,      2049,    10,      3073,
11,      4097,    11,      6145,    12,      8193,
12,      12289,   13,      16385,   13,      24577)

:decompress_sigcomp_message

INPUT-BITS (3, extra_length_bits, !)

:next_character

INPUT-HUFFMAN (index, end_of_message, 4,
7, 0, 23, length_table_start,
1, 48, 191, 0,
0, 192, 199, length_table_mid,
1, 400, 511, 144)
COMPARE ($index, length_table_start, literal, end_of_message,
length_distance)

:literal

set (index_lsb, (index + 1))

OUTPUT (index_lsb, 1)
COPY-LITERAL (index_lsb, 1, $decompressed_pointer)
JUMP (next_character)

:length_distance

; this is the length part

MULTIPLY ($index, 4)
COPY ($index, 4, extra_length_bits)
INPUT-BITS ($extra_length_bits, extra_length_bits, !)
ADD ($length_value, $extra_length_bits)

; this is the distance part

INPUT-HUFFMAN (index, !, 1, 5, 0, 31, distance_table_start)
MULTIPLY ($index, 4)
COPY ($index, 4, extra_distance_bits)

INPUT-BITS ($extra_distance_bits, extra_distance_bits, !)
ADD ($distance_value, $extra_distance_bits)
LOAD (index, $decompressed_pointer)
COPY-OFFSET ($distance_value, $length_value, $decompressed_pointer)
OUTPUT ($index, $length_value)
JUMP (next_character)

:end_of_message

END-MESSAGE (requested_feedback_location,
returned_parameters_location, state_length, 64,
decompress_sigcomp_message, 6, 0)

:circular_buffer
