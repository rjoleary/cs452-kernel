@ Arm syntax highlighting for vim (ARM9/arm-syntax-vim)
@ vim:ft=armv5

.global kernel_entry
.global kernel_exit

.section .text
kernel_entry:
    @ Read SWI instruction into r0, lr is SWI + 4
    ldr r0, [lr, #-4]
    @ SWI immediate value is the last 24 bits of the instruction.
    and r0, r0, #0x00ffffff
    @ Switch to system mode to access cpsr, sp,lr
    msr cpsr, #0xDF
    @ Store user values into their sp
    stmfd sp!, {r4-r11,lr}
    @ Store user sp
    mov r2, sp
    @ Move into supervisor mode
    msr cpsr, #0xD3
    @ Load kernel registers,
    ldmfd sp!, {r1,r4-r11,lr}
    @ Store user sp into td
    str r2, [r1]
    mov pc, lr

kernel_exit:
    @ Store regs onto kernel stack
    @ r0 holds the current task's stack pointer
    @ lr saved because it is overwritten after swi
    stmfd sp!, {r0,r4-r11,lr}
    @ Load the user sp
    ldr r1, [r0]
    @ Load user saved values from their sp
    ldmfd r1!, {r4-r11}
    @ Load the user's pc
    ldmfd r1!, {r2}
    @ Store the user's sp 
    str r1, [r0]
    @ Switch to system mode
    msr cpsr, #0xDF
    @ Put the user's sp into user's sp
    mov sp, r1
    @ Switch back to supervisor mode
    msr cpsr, #0xD3
    @ Set spsr to user mode with interrupts
    msr spsr, #0x10
    @ Go back to user land
    movs pc, r2
