@ vim:ft=armv5

.global irqTrampoline

@ Jump to the correct ISR.
.section .text
irqTrampoline:
    ldr sp, =0x800b0030
    ldr sp, [sp]
    bx sp
