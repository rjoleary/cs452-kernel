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
    @ Switch to system mode to access sp/lr 
    mrs r1, cpsr
    orr r1, r1, #0x1F
    msr cpsr_c, r1
    @ Store user values into their sp
    stmfd sp!, {r4-r11, lr}
    @ Store user sp
    mov r2, sp
    @ Move into supervisor mode
    mrs r1, cpsr
    bic r1, r0, #0x1F
    orr r1, r0, #0x13
    msr cpsr_c, r1
    @ Load kernel registers,
    ldmfd sp!, {r1,r4-r11,lr}
    @ Store user sp into td
    str r2, [r1]
    mov pc, lr

kernel_exit:
    @ Store regs onto kernel stack
    @ r0 holds the current task's stack pointer
    @ lr saved because it is overwritten after swi
    stmfd sp!, {r0,r4-r11, lr}
    @ Load the user sp
    ldr r1, [r0]
    @ Load user saved values from their sp
    ldmfd r1!, {r4-r11}
    @ Load the user's pc
    ldmfd r1!, {r2}
    @ Store the user's sp 
    str r1, [r0]
    @ Switch to user mode
    mrs r0, cpsr
    bic r0, r0, #0x1F
    orr r0, r0, #0x10
    msr cpsr_c, r0
    @ Put the user's sp into sp
    mov sp, r1
    mov pc, r2
