.global kernel_entry
.global kernel_exit

kernel_entry:
    # Read SWI instruction into r0 for reading the immediate value.
    ldr r0, [lr, #-4]
    # SWI immediate value is the last 24 bits of the instruction.
    and r0, r0, #0x00ffffff
    # Jump to C code.
    b svcHandle

kernel_exit:
    # Copy stack pointer from active->sp to sp register.
    mov sp, r0
    # Pop the rest of the registers off of the stack, including pc.
    # LDMEA = LoaD Multiple from Empty Ascending stack
    # TODO: works, but might not conform to gcc abi
    ldmea sp!, {r0-r12,lr,pc}
