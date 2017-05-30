@ vim:ft=armv5

.global timer2Irq
.global irqTrampoline

.section .bss
.align 4
irqStack:
    .zero 128
irqStackEnd:

.section .text
irqTrampoline:
    ldr sp, =0x800b0030
    ldr sp, [sp]
    bx sp

timer2Irq:
    @ Push r1.
    ldr sp, =irqStackEnd
    stmfd sp!, {r0-r12, lr}
    @ Jump to the function.
    mov r0, lr
    bl profilerRecord
    @ Clear the interrupt.
    ldr r1, =0x8081002c
    str r1, [r1]
    @ Acknowledge VIRQ serviced.
    ldr r1, =0x800b0030
    str r0, [r1]
    @ Unstack registers.
    ldmfd sp!, {r0-r12, lr}
    @ Return.
    subs pc, lr, #4
