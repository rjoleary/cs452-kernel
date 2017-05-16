# Kernel

## Building

    $ make
    $ stat build/kernel.elf

## Directory structure

- `include/*.h`         - kernel header files
- `include/user/*.h`    - usermode API
- `src/kernel/*{.c,.s}` - kernel source code
- `src/*/*{.c}`         - source code for various tasks

When the kernel is built, the compiler's source directory is set to
`src/kernel` and the include directory is `include/`.

When a user task is built, for example `test`, the source directory is
`src/test/` and the include directory is `include/user/`.

## Style Guide

- Defines / macros: `UPPER_SNAKE_CASE`
- Enum values: `UPPER_SNAKE_CASE`
- Enums: `UpperCamelCase`
- Functions: `lowerCamelCase`
- Types: `UpperCamelCase`
- Variables: `lowerCamelCase`

## Context Switching

### Software Interrupt

Page 58 of the ARM manual. The parts important to us:

    R14_svc   = address of next instruction after the SWI instruction
    SPSR_svc  = CPSR
    CPSR[4:0] = 0b10011  /* Enter Supervisor mode */
    CPSR[7] = 1          /* Disable normal interrupts */

To return afterwards:

    MOVS pc, R14


The following 4 steps are performed atomically:

    lr_svc <- pc + 4        % like a bl instruction
    pc <- #0x08             % instruction in interrupt vector
    spsr <- cpsr            % store original value of cpsr
    cpsr_svc <- cpsr | 0x3  % put processor into supervisor mode

The instruction in address 0x08 is a branch to the address in 0x28:

    pc <- [#0x28]

Note that supervisor mode has distinct registers for the following:

- 


Page 45
