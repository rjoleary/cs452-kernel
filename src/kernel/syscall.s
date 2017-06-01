@ vim:ft=armv5

.global irqEntry
.global kernelEntry
.global kernelExit
.global isIrq

.section .data
.align 4
isIrq:
    .word 0

.section .text
irqEntry:
    @ Switch to system mode to push user registers.
    msr cpsr_c, #0xdf
    stmfd sp, {r0-r15}
    @ r0: holds the user's stack pointer
    mov r0, sp
    @ Switch back to irq mode.
    msr cpsr, #0xd2
    @ Store lr as pc in user's stack.
    stmfd r0, {lr}
    @ Store the stored PSR in the user's stack.
    @ r1: holds the task's PSR
    mrs r1, spsr
    str r1, [r0, #-0x44]
    @ Switch back to supervisor mode.
    msr cpsr, #0xd3
    @ Set the irq flag.
    ldr r1, =isIrq
    str r1, [r1]
    @ Restore kernel registers and return.
    ldmea sp, {r4-r12,sp,pc}
    
kernelEntry:
    @ Switch to system mode to push user registers.
    msr cpsr_c, #0xdf
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
    ldr lr, [r0, #-4]!
    @ Load the rest of the user registers.
    ldmea r0, {r0-r14}^
    @ Return from the exception (copies SPSR to CPSR).
    movs pc, lr
