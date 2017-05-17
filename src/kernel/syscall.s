@ Arm syntax highlighting for vim (ARM9/arm-syntax-vim)
@ vim:ft=armv5

.global kernel_entry
.global kernel_exit

.section .text
kernel_entry:
    @ Store arguments
    stmfd sp!, {r0-r3}
    @ Read SWI instruction into r0, lr is SWI + 4
    ldr r0, [lr, #-4]
    @ SWI immediate value is the last 24 bits of the instruction.
    and r0, r0, #0x00ffffff
    @ Move pc for user
    mov r2, lr
    @ Switch to system mode to access cpsr,sp,lr
    msr cpsr, #0xDF
    @ Store user values into their sp
    stmfd sp!, {r2}
    stmfd sp!, {lr}
    stmfd sp!, {r4-r12}
    @ Store user sp
    mov r2, sp
    @ Move into supervisor mode
    msr cpsr, #0xD3
    @ Load stored args
    ldmfd sp!, {r4-r7}
    @ Load user sp, request
    ldmfd sp!, {r1,r3}
    @ Store request
    stmea r3!, {r4-r7}
    @ Load kernel registers
    ldmfd sp!, {r4-r12,lr}
    @ Store user sp into td
    str r2, [r1]
    mov pc, lr

kernel_exit:
    @ Store regs onto kernel stack
    @ r0 holds the current task's stack pointer
    @ r1 holds pointer to request
    @ lr saved because it is overwritten after swi
    stmfd sp!, {r0,r1,r4-r12,lr}
    @ Load the user sp
    ldr r1, [r0]
    sub r1, r1, #48
    str r1, [r0]
    add r0, r1, #48
    @ Load user saved values from their sp, also ret value
    ldmfd r0!, {r3-r12}
    @ Load the user's lr and pc
    ldmfd r0!, {r1,r2}
    @ Switch to system mode
    msr cpsr, #0xDF
    @ Put the user's sp and lr
    mov sp, r0
    mov lr, r1
    @ Switch back to supervisor mode
    msr cpsr, #0xD3
    @ Set spsr to user mode with interrupts
    msr spsr, #0x10
    @ Set return value
    mov r0, r3
    @ Go back to user land
    movs pc, r2
