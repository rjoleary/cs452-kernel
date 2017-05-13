.global kernel_entry

kernel_entry:
    # Read SWI instruction into r0 for reading the immediate value.
    ldr r0, [lr, #-4]
    # SWI immediate value is the last 24 bits of the instruction.
    AND r0, r0, #0x00ffffff
    # Jump to C code.
    b svcHandle

kernel_exit:
    # TODO
