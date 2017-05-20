@ Arm syntax highlighting for vim (ARM9/arm-syntax-vim)
@ vim:ft=armv5

.global kernelEntry
.global kernelExit

.section .text
kernelEntry:
    @ Switch to system mode to push user registers.
    msr cpsr_c, #0x1f
    stmfd sp, {r0-r15}
    @ r0: holds the user's stack pointer
    mov r0, sp
    @ Switch back to supervisor mode.
    msr cpsr, #0xd3
    @ Store lr as pc in user's stack.
    stmfd r0, {lr}
    @ Store the stored PSR in the user's stack.
    @ r1: holds the task's PSR
    mrs r1, spsr
    str r1, [r0, #-0x44]
    @ Restore kernel registers and return.
    ldmea sp, {r4-r12,sp,pc}

kernelExit:
    @ r0: holds the task's stack pointer (passed as arg 0)
    @ Store kernel's registers onto kernel's stack.
    stmfd sp, {r4-r12,sp,lr}
    @ Restore the task's psr.
    @ r1: holds the task's PSR
    ldr r1, [r0, #-0x44]
    msr spsr, r1
    @ Load the task's pc into lr_svc.
    ldr lr, [r0, #-4]
    @ Load the rest of the user registers.
    sub r0, r0, #4
    ldmea r0, {r0-r14}^
    @ Return from the exception (copies SPSR to CPSR).
    movs pc, lr
