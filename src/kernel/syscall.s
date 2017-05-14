@ Arm syntax highlighting for vim (ARM9/arm-syntax-vim)
@ vim:ft=armv5

.global kernel_entry
.global kernel_exit

kernel_sp:
    @ Storage for kernel's stack pointer while we're off in user land.
    @ TODO: this is BAAAAD because we're writing to the text segment
    .word 0x0

kernel_entry:
    @ Read SWI instruction into r0 for reading the immediate value.
    ldr r0, [lr, #-4]
    @ SWI immediate value is the last 24 bits of the instruction.
    and r0, r0, #0x00ffffff
    @ Restore the kernel's state and stack pointer. r1 is used as a scratch register.
    mov r1, #kernel_sp
    ldr sp, [r1]
    ldmfa sp!, {r4-r11,r13,pc}

kernel_exit:
    @ Store kernel's state and stack pointer. r1 is used as a scratch register.
    mov r1, #kernel_sp
    stmfa sp!, {r4-r11,r13,lr}
    str sp, [r1]
    @ Copy stack pointer from active->sp to sp register.
    mov sp, r0
    @ Pop the rest of the registers off of the stack, including pc.
    @ LDMEA = LoaD Multiple from Empty Ascending stack
    @ TODO: works, but might not conform to gcc abi
    ldmea sp!, {r0-r12,lr,pc}
