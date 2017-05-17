# CS 452 Kernel 1

Name: The Coldwell Kernel

Students: Ryan O'Leary (rj2oleary) and Elnar Dakeshov (edakesho)

Student Ids: 20509502 and 20577578

Date: May 26, 2017


## Overview

This program implements non-preemptive multitasking for the ARM 920T CPU.
Tasks may be created, run and exited from. Tasks must yield occasionally to
allow other task of the same priority to execute. Additionally, a constant time
scheduler is implemented with support for up to 32 distinct priorities. Example
tasks have been included to demonstrate these functionalities.


## Download Path

    git clone https://git.uwaterloo.ca/coldwell/kernel.git


## Running

TODO: we have to upload a kernel we won't accidentally overwrite

    > load -b 0x00218000 -h 10.15.167.5 "ARM/rj2olear/coldwell.elf"
    > go


## Building

    $ make
    $ stat build/kernel.elf


## Demo

### Output

    TODO


### Explanation

    TODO


## Description

### Directory structure

Source files are sorted as follows:

- `include/*.h`         : kernel header files
- `include/user/*.h`    : user API and header files
- `src/kernel/*{.c,.s}` : kernel source code
- `src/user/*.c`        : shared user source code
- `src/*/*{.c}`         : source code for various tasks

This organization prevents user source files from including kernel header files
and calling kernel functions. However, kernel sources can still include user
headers. Specifically, this is accomplished with the following rules in the
makefile:

- When the kernel is built, the compiler's source directory is set to
  `src/kernel` and the include directory is `include`.
- When a user task is built, for example `first`, the source directory is
  `src/first` and the include directory is `include/user`.


### Style Guide

Naming conventions:

- Defines / macros: `UPPER_SNAKE_CASE`
- Enum values: `UPPER_SNAKE_CASE`
- Enums: `UpperCamelCase`
- Functions: `lowerCamelCase`
- Types: `UpperCamelCase`
- Variables: `lowerCamelCase`

Consequentially, the `exit` syscall interferes with the GCC `exit` builtin. For
this reason, the exit syscall has been renamed to `exeunt`.


### Error Codes

All error codes are specified by the `Error` enumeration. By convention,
syscalls return a non-negative value for success, and a negative error code to
indicate failure. The error codes are:

- (0) `ERR_OK`: no error
- (1) `ERR_BADARG`: bad argument
- (2) `ERR_NORES`: no resource available
- (3) `ERR_TRUNC`: truncated
- (4) `ERR_INVID`: invalid id
- (5) `ERR_BADITC`: could not complete inter-task communication
- (6) `ERR_CORRUPT`: corrupted volatile data
- (7) `ERR_UNKN`: unknown error

The `const char *err2str(int err)` function converts these error codes to human
readable strings.


### Priorities




### System Calls

The following are declared in `include/user/task.h` and can be included via
`#include <task.h>`:

- `Tid Create(Priority priority, void (*code)(void));` instantiates a task with
  the given priority and entry point. If the priority is higher than the active
  task's priority, the new task will begin execution immediately. If the
  priority is outside of the valid range, `-ERR_BADARG` is returned. If the
  kernel runs out of task descriptors, `-ERR_NORES` is returned. Currently,
  task descriptors are not recycled, so a maximum of 32 tasks may be created
  ever including the idle and first tasks.
- `Tid MyTid(void);` returns the task id of the calling task.
- `Tid myParentTid(void);` returns the task if of the calling task's parent.
- `void pass(void);` ceases execution of the calling task, but leaves it in the
  ready state. This syscall enables cooperative multitasking.
- `void exeunt(void);` terminates execution of the task forever. This syscall
  does not return. Currently, task descriptors are not recycled.


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


## Bugs

- No known bugs.

## Checksums

Run `md5sum $(git ls-files)`:

    TODO

Git hash: TODO
