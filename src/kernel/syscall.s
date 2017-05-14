@ Arm syntax highlighting for vim (ARM9/arm-syntax-vim)
@ vim:ft=armv5

.global kernel_entry
.global kernel_exit
.global user_sp

.section .data
kernel_sp:
    @ Storage for kernel's stack pointer while we're off in user land.
    .word 0x0

.section .data
user_sp:
    @ TODO: this is a hack to access user stack pointer from C.
    .word 0x0

.section .text
kernel_entry:
    @ Read SWI instruction into r0 for reading the immediate value.
    ldr r0, [lr, #-4]
    @ SWI immediate value is the last 24 bits of the instruction.
    and r0, r0, #0x00ffffff
    @ Push registers onto user stack and make accessible from C.
    stmfd sp!, {r4-r11,lr}
    ldr r1, =user_sp
    str sp, [r1]
    @ Restore the kernel's state and stack pointer. r1 is used as a scratch register.
    mov r1, #kernel_sp
    ldr sp, [r1]
    ldmfd sp!, {r4-r11,pc}

kernel_exit:
    @ Store kernel's state and stack pointer. r1 is used as a scratch register.
    mov r1, #kernel_sp
    stmfd sp!, {r4-r11,lr}
    str sp, [r1]
    @ Copy stack pointer from active->sp to sp register.
    mov sp, r0
    @ Pop the rest of the registers off of the stack, including pc.
    @ LDMEA = LoaD Multiple from Empty Ascending stack
    ldmfd sp!, {r4-r11,pc}
