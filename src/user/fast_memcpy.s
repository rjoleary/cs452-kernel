.global fast_memcpy

.section .text
fast_memcpy:
    push {r4-r10}
    @ Divide bytes by 4 since we are 4 byte aligned
    lsr r2, r2, #2
loopStart:
    cmp r2, #0
    ble loopEnd
    @ Modulo r3 = r2 % 8
    and r3, r2, #7
    add pc, pc, r3, lsl #4
    nop

  	ldmia r1!, {r3-r10}
    stmia r0!, {r3-r10}
    sub r2, r2, #8
    b loopStart

  	ldmia r1!, {r3}
    stmia r0!, {r3}
    sub r2, r2, #1
    b loopStart

  	ldmia r1!, {r3-r4}
    stmia r0!, {r3-r4}
    sub r2, r2, #2
    b loopStart

  	ldmia r1!, {r3-r5}
    stmia r0!, {r3-r5}
    sub r2, r2, #3
    b loopStart

  	ldmia r1!, {r3-r6}
    stmia r0!, {r3-r6}
    sub r2, r2, #4
    b loopStart

  	ldmia r1!, {r3-r7}
    stmia r0!, {r3-r7}
    sub r2, r2, #5
    b loopStart

  	ldmia r1!, {r3-r8}
    stmia r0!, {r3-r8}
    sub r2, r2, #6
    b loopStart

  	ldmia r1!, {r3-r9}
    stmia r0!, {r3-r9}
    sub r2, r2, #7
    b loopStart

loopEnd:
    pop {r4-r10}
    bx lr
