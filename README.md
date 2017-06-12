Name: The Coldwell Kernel

Students: Ryan O'Leary (rj2olear) and Elnar Dakeshov (edakesho)

Student Ids: 20509502 and 20577578

Date: June 14, 2017


## Overview

The coldwell kernel implements multitasking for the ARM 920T CPU. Tasks may be
created, run and exited from. Tasks may await a timer event which periodically
fires every 10ms. Additionally, a constant time scheduler is implemented with
support for up to 32 distinct priorities. UART IO is provided, as well as a name
server and interrupt handling system.


## Download Path

    git clone https://git.uwaterloo.ca/coldwell/kernel.git


## Checksums

Git hash: TODO


## Running

In RedBoot, run the following:

    > load -b 0x00218000 -h 10.15.167.5 TODO
    > go


## Building

    $ make <ARGUMENTS>
    $ stat build/kernel.elf

Where `<ARGUMENTS>` may be any combination of the following:

- `CACHE_ENABLED=1`: Enables the instruction and data caches
- `OPT_ENABLED=1`: Enables optimizations (`-O2` and `-flto`)
TODO: Still works?
- `STRACE_ENABLED=1`: Enables printing debug information from the kernel


## Functionality

    tr train[0-80] speed[0-14]
    rv train[0-80]
    sw switch[0-255] direction [C,S]
    q

## Description

### Language

We are using C++14 with GCC 6. C++ allows the use of a richer type system and 
other safety guarantees. Additionally, we can shift some performance costs from
runtime to compile time. To safely use C++ in an embedded system, many features
must be disabled such as RTTI (Run-Time Type Information), exceptions, the STL
(Standard Template Library), exceptions, unwind tables and threadsafe static
variables.


### Directory structure

Source files are organized as follows:

- `include/*.h`           : kernel header files
- `include/user/*.h`      : user API and header files
- `src/kernel/*{.cpp,.s}` : kernel source code
- `src/user/*.cpp`        : shared user source code
- `src/*/*{.cpp}`         : source code for various tasks

This organization prevents user source files from including kernel header files
and calling kernel functions. However, kernel sources can still include user
headers. Specifically, this is accomplished with the following rules in the
Makefile:

- When the kernel is built, the compiler's source directory is set to
  `src/kernel` and the include directory is `include`.
- When a user task is built, for example `first`, the source directory is
  `src/first` and the include directory is `include/user`.


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

The task descriptor is quite self explanatory, it holds all the bookkeeping
information for any given task. There are pointers to other tasks which are used
for various intrusive lists. These lists are the ready queue and the sender
queue.

These are the possible states of a task:

- `Active`: The task has just run, is running, or is about to run. Only one
  task may be in the active state.
- `Ready`:  The task is ready to be scheduled and activated.
- `Zombie`: The task has exited and will never again run, but still retains its
  memory resources.
- `SendBlocked`: The task has executed Receive, and is waiting for a task to 
  send to it.
- `ReceiveBlocked`: The task has executed Send, and is waiting for the message
  to be received.
- `ReplyBlocked`: The task have executed Send and its message has been received,
  but it has not received a reply.
- `EventBlocked`: The task has executed `awaitEvent`, but the event on which it
  is waiting has not occurred.


### Scheduling

Priorities are numbered 0 (lowest) through 31 (highest). Tasks with a higher
value are scheduled first.

The scheduler (found in `src/kernel/scheduler.cpp`) maintains 32 queues, one for
each priority.

Since there are 32 priorities, each priority queue's state can be represented
with a single bit in a 32 bit integer. Additionally, querying the first bit that
is toggled can be done in constant time. We use `__builtin_clz` for this
purpose. If the bit is toggled on, that means there are ready tasks for the
given priority. Otherwise, the bit is toggled off.

The queues are represented using an intrusive singly linked list. This way,
insertion to the end and removal from the front are constant time operations.
Additionally, there is little size overhead as we only need to store an extra
pointer per task.

An idle task exists that runs at the lowest priority. Since we measure the time
spend in any given user task, we can use the idle task's running time as a
metric of how much time is spent waiting on events.


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

The syscall number is written to `r5` by the user task before performing the
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
    -0x34 |  r3  | <- argument 3
    -0x30 |  r4  | <- argument 4
    -0x2c |  r5  | <- syscall number
          |  .   |
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

TODO: How does the stack look usually (thing we lost marks on)

### Strace
TODO: Does it work?

Additional debug information can be enabled by using `make STRACE_ENABLED=1`
when building. This will enable a printout of some useful information such as
the program's memory layout, a build time, as well as additional trace output
for all system calls. The output is also in a darker color to distinguish it
from normal output.


### Task Manipulation

A new task can be created using `create`. There is a current hard limit of 32
tasks, and since there is currently no task reuse possible, `create` must not
be called more than 32 times. A task can voluntarily give up its running time by
calling `pass`, and there is a scheduling quantum of 10 ms, meaning that any
task will be preempted in at most 10 ms (but this task may rerun again
immediately after its preemption). A task can quit running at any time by
calling `exeunt`. A task that returns from its main function will automatically
call `exeunt`.

### Inter-Task Communication (ITC)

There are three functions for inter-task communication, `send`, `receive`, and
`reply`. A send queue is implemented as an intrusive list to save on space. The
task's state is modified so that the kernel can keep track of which of these
functions the task has called.

The three functions are written using C++ templates to ensure that there
are no issues with size misalignment. This way, any structure can be passed and
the size and alignment are checked at compile time. This enables us to make
a fast memcpy that expects certain alignments/sizes.

The nameserver uses an array to internally hold all registered names. This is
sort of a lie however, because there are no registered names at all. Instead, an
enum holds all the symbolic names for the registered program. This way, we know
at compile time the size of the array needed to hold all program registrations
and it makes lookup and insertion constant time.


### memcpy

To minimize latency for copying data we implemented a nontrivial memcpy
function. The main difference compared to the original "copy one byte at a
time" is that it tries to copy as many bytes in a single instruction as
possible. To accomplish this, we first assume all copied data is 4 byte
aligned. This is possible through the templated `send/receive/reply` functions,
as now the compiler can enforce proper alignment of the data types used. Next,
we use a custom memcpy that copies up to 32 bytes at a time through `ldm` and
`stm` commands. The instructions stand for "load multiple" and "store
multiple". We use them to copy up to 8 registers in one go (32 bytes). This
way, there is barely any differences between the 4 byte and 64 byte performance
tests.


### Priorities

The first user task that bootstraps all others starts with a priority of 15,
which is in the middle of all priorities (0 being lowest, 31 being highest).
Then, the nameserver is created with a priority of 31. The reason the
nameserver has such a high priority is we do not anticipate that it will be
used too often, so having it respond quickly is not an issue. On top of this,
we do not want to have a high priority task being blocked by a lower one
(priority inversion) just because the nameserver is taking a long time to
respond. Therefore, we went to the logical extreme of setting it to the highest
priority, so that no task (low or high) will have to wait on the nameserver.

TODO add pris of everything else


### Interrupts

We are using vectored interrupts. During kernel initialization, we enable and
prioritize all the interrupts we care about. When a task syscalls 
`awaitEvent()`, the pointer to the task descriptor is stored in the vectored
interrupt address for the interrupt source.

When the interrupt occurs, we retrieve the task pointer from the interrupt
vector. To determine the interrupt type, we retrieve the first argument, `r0`,
of the blocked task (which has been pushed to the stack).

We make the assumption that only one task will register to an event at a time,
which is fine because we use the notifier-server-consumer pattern.


### Clock

We use clock timer 1 for our tick measurements. Since this is a 2kHz clock, we
set the rollover time to 19, so that it will fire off an interrupt every 20
counts (which translates to once every 10ms, the length of a tick). A notifier
task listens to this event, and when it fires it sends an empty message to the
clock server. The clock server then increments its time and notifies any
waiting tasks if appropriate. 

The clock server has the following interface:

- `int delay(Tid tid, int ticks);`: wait for a given amount of time
- `int time(Tid tid);` give the time since clock server start up
- `int delayUntil(Tid tid, int ticks);` wait until a time

Where each tick is a 10 ms and the `tid` is the TID of the clock server. The
clock server's TID is registered to the nameserver.


### Min Heap

The min heap is implemented as an arbitrary heap with some comparator function.
This is done with the magic of C++ templates. The heap has 3 functions,
`peak()`, `push()`, and `pop()`. `peak()` checks the top of the heap, `push()`
adds an element and `pop()` removes the top of the heap. We use a minheap to
manage all the timing requirements of the kernel. When we get a call to
`delay()` we convert the ticks into an absolute time. Then, for both `delay()`
and `delayUntil()` we push the TID of the caller and the absolute time into the
minheap. Any time we increment the ticks, we check the top of the heap and if
the current tick is equal to the soonest waiting task we pop as many tasks off
as we can and notify them. We use a minheap as it has `O(log n)` push and pop
while maintaining a priority queue and due to ease of implementation.

TODO k4/ uart info


## Bugs

- No known bugs.
