# CS 452 Kernel 1

Name: The Coldwell Kernel

Students: Ryan O'Leary (rj2olear) and Elnar Dakeshov (edakesho)

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


## Checksums

Git hash: TODO


## Running

In RedBoot, run the following:

    > load -b 0x00218000 -h 10.15.167.5 "ARM/rj2olear/k1.elf"
    > go


## Building

    $ make
    $ stat build/kernel.elf


## Demo

### Output

    FirstUserTask: entering
    FirstUserTask: myTid()=0
    FirstUserTask: created tid 1
    FirstUserTask: created tid 2
    TestUserTask: myTid()=3, myParentTid()=0
    TestUserTask: myTid()=3, myParentTid()=0
    FirstUserTask: created tid 3
    TestUserTask: myTid()=4, myParentTid()=0
    TestUserTask: myTid()=4, myParentTid()=0
    FirstUserTask: created tid 4
    FirstUserTask: exiting
    TestUserTask: myTid()=1, myParentTid()=0
    TestUserTask: myTid()=2, myParentTid()=0
    TestUserTask: myTid()=1, myParentTid()=0
    TestUserTask: myTid()=2, myParentTid()=0


### Explanation

1. The first two lines are printed by the first task. The first task's id is
  always 0 and its priority is 3.
2. The first task creates two tasks in succession. However, since they have a
  lower priority (1), the first task takes precedence over them. The two
  created tasks (tid 1 and  remain in the ready state.
3. Next, the first task creates another user task (tid=3), but since it has a
  priority of 5, higher than the first task's priority, it preempts the first
  task. The first task must wait until task 3 exits before being able to print
  its task id.
4. The higher priority task prints its task id and parent task id then passes.
  However, it is the only high priority task and so it gets to run again and
  prints again. The high priority task exits and the first task gets to run
  again.
5. Since the first priority task was in the middle of printing, it finishes and
  prints the "created tid 3".
6. The first task creates another task with priority 5. The tid for this new
  task is 4. Steps 4 and 5 are repeated for this task.
7. The first task finishes by printing "exiting" and calling the `exeunt` syscall.
8. Now there are still 2 lower priority tasks in the ready state.
9. The first of the two prints and then calls pass. Since both tasks have the
  same priority, the second low priority task gets to run. It prints and
  passes to the first task, which prints and exits. Lastly, the second task
  prints and exits also.
10. Once all the tasks exits, the system returns to RedBoot.


## Description

### Directory structure

Source files are organized as follows:

- `include/*.h`         : kernel header files
- `include/user/*.h`    : user API and header files
- `src/kernel/*{.c,.s}` : kernel source code
- `src/user/*.c`        : shared user source code
- `src/*/*{.c}`         : source code for various tasks

This organization prevents user source files from including kernel header files
and calling kernel functions. However, kernel sources can still include user
headers. Specifically, this is accomplished with the following rules in the
Makefile:

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


### Task Descriptor and States

The definition of the task descriptor can be found in `include/task.h`. Each
task descriptor contains the following fields:

- `Tid tid`: task id (-1 if td is unallocated)
- `Tid ptid`: parent's task id
- `Priority pri`: priority 0 to 31
- `struct Td *nextReady`: next TD in the ready queue, or NULL
- `struct Td *sendReady`: next TD in the send queue, or NULL. For the purposes
  of kernel 1, this field is unused.
- `enum RunState state`: current run state
- `unsigned *sp`: current stack pointer

For kernel 1, only three states are needed for task creation. More states are
required for future kernels. The three current states are:

- `ACTIVE`: The task has just run, is running, or is about to run. Only one
  task may be in the active state.
- `READY`:  The task is ready to be scheduled and activated.
- `ZOMBIE`: The task has exited and will never again run, but still retains its
  memory resources.

New tasks start in the `READY` state. The possible transitions are:

- `READY -> ACTIVE`
- `ACTIVE -> READY`
- `ACTIVE -> ZOMBIE`


### Scheduling

Priorities are numbered 0 (lowest) through 31 (highest). Tasks with a higher
value are scheduled first.

The scheduler (found in `src/kernel/scheduler.c`) maintains 32 queues, one for
each priority.

If no tasks are available for scheduling, the kernel exits and returns to the
RedBoot prompt.

Since there are 32 priorities, each priority queue's state can be represented
with a single bit in a 32 bit integer. Additionally, querying the first bit that
is toggled can be done in constant time. We use `__builtin_clz` for this
purpose. If the bit is toggled on, that means there are ready tasks for the
given priority. Otherwise, the bit is toggled off.

The queues are represented using an intrusive singly linked list. This way,
insertion to the end and removal from the front are constant time operations.
Additionally, there is little size overhead as we only need to store an extra
pointer per task.


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


### Context Switching

Context switching is implement by two assembly functions: `kernelEntry` and
`kernelyExit` which can be found in `src/kernel/syscall.s`.

`kernelExit` exits kernel mode and enters user mode. It performs the following:

1. Push the kernel registers to the kernel stack (r4-r12, sp, lr).
2. Load the task's CPSR into SPSR.
3. Pop the task's registers from the user's stack (r0-r14).
4. `movs pc, lr` which restores the program counter and copies SPSR to CPSR.

`kernelEntry` exits user mode and enters kernel mode. It performs the following:

1. Push the task's registers to the user's stack (r0-r15).
2. Save `lr` as `pc` in the user's stack.
3. Save the task's CPSR.
4. Pop the kernel registers from the kernel stack (r4-r12, sp, lr).
5. Update the task's stack pointer in the task descriptor.

The task's arguments are read directly off the stack from the pushed registers
(r0-r4 in the diagram below). The return value is written to the location of r0
in the stack.

The syscall number is written to `r9` by the user task before performing the
SWI. This is somewhat more efficient than using the immediate value (see
discussions in the [ARM EABI](
http://www.arm.linux.org.uk/developer/patches/viewpatch.php?id=3105/4)).

User Stack Layout: (note that `sp_usr` remains unmodified)

        0 +------+
        ↑ |      |
          |      |
          |      |
          |      |
    -0x44 | cpsr |
    -0x40 |  r0  | <- argument 0 / return value
    -0x3c |  r1  | <- argument 1
    -0x38 |  r2  | <- argument 2
          |  .   |
          |  .   |
          |  .   |
    -0x1c |  r9  | <- syscall number
          |  .   |
    -0x10 | r12  |
    -0x0c |  sp  |
    -0x08 |  lr  |
    -0x04 |  pc  |
          |  X   | <- sp_usr
          |  X   |
          |  X   |
          |      |
          |      |
          |      |
        ↓ |      |
        ∞ +------+

Kernel stack layout: (note that `sp_svc` remains unmodified)

        0 +-----+
        ↑ |     |
          |     |
          |     |
          |     |
    -0x30 |  r4 |
    -0x2c |  r5 |
          |  .  |
          |  .  |
          |  .  |
    -0x0c | r12 |
    -0x08 |  sp |
    -0x04 |  pc |
          |  X  | <- sp_svc
          |  X  |
          |  X  |
          |     |
          |     |
          |     |
        ↓ |     |
        ∞ +-----+

### Strace

Additional debug information can be enabled by using `make STRACE_ENABLED=1`
when building. This will enable a printout of some useful information such as
the program's memory layout, a build time, as well as additional trace output
for all system calls. The output is also in a darker color to distinguish it
from normal output.

## Bugs

- No known bugs.
