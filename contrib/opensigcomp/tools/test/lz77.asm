; Variables that do not need to be stored after decompressing each
; SigComp message are stored here:

at (32)

:index                          pad (2)
:length_value                   pad (2)

at (42)

set (requested_feedback_location, 0)

; The UDVM registers must be stored beginning at Address 64:

at (64)

; Variables that should be stored after decompressing a message are
; stored here. These variables will form part of the SigComp state
; item created by the bytecode:

:byte_copy_left                 pad (2)
:byte_copy_right                pad (2)
:decompressed_pointer           pad (2)

set (returned_parameters_location, 0)

align (64)

:initialize_memory

set (udvm_memory_size, 8192)
set (state_length, (udvm_memory_size - 64))

; The UDVM registers byte_copy_left and byte_copy_right are set to
; indicate the bounds of the circular buffer in the UDVM memory. A
; variable decompressed_pointer is also created and set pointing to
; the start of the circular buffer:

MULTILOAD (64, 3, circular_buffer, udvm_memory_size, circular_buffer)

; The "dictionary" area of the UDVM memory is initialized to contain
; the values 0 to 255 inclusive:

MEMSET (static_dictionary, 256, 0, 1)

:decompress_sigcomp_message

:next_character

; The next character in the compressed message is read by the UDVM
; and the position and length integers are stored in the variables
; position_value and length_value respectively. If no more
; compressed data is available the decompressor jumps to the
; "end_of_message" subroutine:

INPUT-BYTES (4, index, end_of_message)

; The position_value and length_value point to a byte string in the
; UDVM memory, which is copied into the circular buffer at the
; position specified by decompressed_pointer. This allows the string
; to be referenced by later characters in the compressed message:

COPY-LITERAL ($index, $length_value, $decompressed_pointer)

; The byte string is also outputted onto the end of the decompressed
; message:

OUTPUT ($index, $length_value)

; The decompressor jumps back to consider the next character in the
; compressed message:

JUMP (next_character)

:end_of_message

; The decompressor saves the UDVM memory and halts:

END-MESSAGE (requested_feedback_location,
returned_parameters_location, state_length, 64,
decompress_sigcomp_message, 6, 0)

at (256)

; Memory for the dictionary and the circular buffer are reserved by
; the following statements:

:static_dictionary              pad (256)
:circular_buffer
