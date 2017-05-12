.global kernel_entry

kernel_entry:
    ldr r0, [lr, #-4]
    b svcHandle
