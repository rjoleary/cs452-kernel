@ vim:ft=armv5

.global profilerFiq

.section .bss
.align 4
irqStack:
    .zero 4096
irqStackEnd:


.section .text
profilerFiq:
    @ Clear the interrupt.
    @ldr sp, =0x8081008c
    @str sp, [sp]
    @subs pc, lr, #4

    @ Push r1. TODO: push fewer registers because FIQ banking
    ldr sp, =irqStackEnd
    stmfd sp!, {r0-r12, lr}
    @ Jump to the function.
    sub r0, lr, #4
    bl profilerRecord
    @ Clear the interrupt.
    ldr r1, =0x8081008c
    str r1, [r1]
    @ Unstack registers.
    ldmfd sp!, {r0-r12, lr}
    @ Return.
    subs pc, lr, #4
