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
