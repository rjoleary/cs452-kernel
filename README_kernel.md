# CS 452 Kernel 4

Name: The Coldwell Kernel

Students: Ryan O'Leary and Elnar Dakeshov

Date: June 14, 2017


## Overview

The Coldwell Kernel implements preemptive multitasking for the ARM 920T CPU.
Tasks may be created, run and exited from. Inter-task communication uses is
based on three synchornous primitives: send, receive and reply. Tasks may await
a periodic 10ms timer event or block on UART events. Additionally, a constant
time scheduler is implemented with support for up to 32 distinct priorities.

A variety of servers are provided including serial IO, a timer and a train
manager. Upon starting the kernel, the user is prompted with a user interface
for controlling the trains. The kernel is stable and is tested to run for over
40 minutes.


## Logo


            _\/    \/_
             _\/__\/_
              /\__/\          ___ ___      _          __
         _\_\/_/  \_\/_/_    /   /  / /   / \  /   / /_   /   /
          / /\ \__/ /\ \    /__ /__/ /__ /__/ /_/_/ /___ /__ /__
             _\/__\/_
             _/\  /\_       For stickers and glory!
             /\    /\


## Checksums

Git hash: f09f101470cbd9eeb045df448e87411852d8c0e3


## Running

In RedBoot, run the following:

    > load -b 0x00218000 -h 10.15.167.5 "ARM/rj2olear/k4.elf"
    > go


## Building

    $ make <ARGUMENTS>
    $ stat build/kernel.elf

Where `<ARGUMENTS>` may be any combination of the following:

- `CACHE_ENABLED=1`: Enables the instruction and data caches
- `OPT_ENABLED=1`: Enables optimizations (`-O2` and `-flto`)


## Interface

The following convenient features can be found on the user interface:

- Clock display in tenths of a second
- GO/STOP, toggle with Tab key
- Percent user time spent in the idle task compared to other tasks
- Top 10 most recent sensor triggers
- 22 switch states
- Command prompt


## Commands

Enter instructions at the `%` prompt. The available instructions are:

- `com i [BYTE...]` - Send arbitrary bytes over COMi.
- `help` - Display this help information.
- `li NUMBER` - Toggle train lights.
- `q` - Quit and return to RedBoot.
- `rv NUMBER` - Reverse the direction of the train.
- `sw NUMBER DIR` - Set switch direction ('S' or 'C').
- `task (TID|NAME)` - Return info about a task.
- `taskall` - Return info about all tasks.
- `tr NUMBER SPEED` - Set train speed (0 for stop).

Additionally, the Tab will stop all trains in case of emergency.


## Kernel Structure

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

- (0) `Error::Ok`: no error
- (1) `Error::BadArg`: bad argument
- (2) `Error::NoRes`: no resource available
- (3) `Error::Trunc`: truncated
- (4) `Error::InvId`: invalid id
- (5) `Error::BadItc`: could not complete inter-task communication
- (6) `Error::Corrupt`: corrupted volatile data
- (7) `Error::Unkn`: unknown error

The `const char *errorToString(Error err)` function converts these error codes to human
readable strings.

We use a special error handling class called `ErrorOr` that holds a value or an
error. This way, we can easily check that the returned value is valid by
overloading the `~` binary operator. Internally, `ErrorOr` holds an integer. We
cast whatever value it holds (as long as it can fit) into this integer. Errors
are negative while valid values are positive. This way, we can cast to the
stored value or to the stored error, but not both at any given time. We assert
if we try to cast into a value if there is an error stored, letting us easily
check all returned function calls.


### Task Descriptor and States

The definition of the task descriptor can be found in `include/task.h`. Each
task descriptor contains the following fields:

- `Tid tid`: task id (-1 if td is unallocated)
- `Tid ptid`: parent's task id
- `Priority pri`: priority, 0 to 31
- `struct Td *nextReady, sendEnd`: next TD in the ready queue, or NULL
- `struct Td *sendReady`: next TD in the send queue, or NULL. This pointer is
  reused for the interrupt queue.
- `Runstate state`: current run state
- `unsigned *sp`: current stack pointer
- `unsigned long long userTime`: number of 508 kHz ticks spent in this task
- `unsigned long long sysTime`: number of 508 kHz ticks spent in the kernel

These are the possible states of a task:

- `Active`: The task has just run, is running, or is about to run. Only one
  task may be in the active state.
- `Ready`:  The task is ready to be scheduled and activated.
- `Zombie`: The task has exited and will never again run, but still retains its
  memory resources.
- `SendBlocked`: The task has executed `receive()`, and is waiting for a task
  to send to it.
- `ReceiveBlocked`: The task has executed `send()`, and is waiting for the
  message to be received.
- `ReplyBlocked`: The task have executed `send()` and its message has been
  received, but it has not received a reply.
- `EventBlocked`: The task has executed `awaitEvent()`, but the event on which
  it is waiting has not occurred.

New tasks start in the `READY` state. The possible transitions are:

- `Ready -> Active`: The task has been scheduled.
- `Active -> Ready`: The task has been preempted.
- `Active -> Zombie`: The task has called `exeunt()`.
- `Active -> SendBlocked -> Ready`: The task called `receive()` and is blocked until a
  matching `send()`.
- `Active -> ReceiveBlocked -> Ready`: The task called `send()` and is blocked until a
  matching `receive()` and subsequent `reply()`.
- `Active -> EventBlocked -> Ready`: The task called `awaitEvent()` and is
  blocked on an event.


### Task List

The `task` command list information regarding a single task. For example:

    % task First
    TID     NAME    PTID    PRI     STATE   USER    SYS
    0       First   0       15      A       0%      0%
    % task 2
    TID     NAME    PTID    PRI     STATE   USER    SYS
    2       Idle    0       0       R       99%     0%

Similarily, the `taskall` command lists all the tasks.

Here is a list of all the tasks:

- `First`: The first task which initializes the system and displays user
  prompt.
- `NS`: The nameserver which provides the `registerAs`/`whoIs`/`reverseWhoIs`
  interface.
- `Idle`: Runs when no other task is ready.
- `Uart2Tx`/`Uart2Rx`/`Uart1Tx`/`Uart1Rx`: These are the serial servers. These
  handle the `putc`/`getc`/`flush` messages and buffer the data before sending
  the their respective notifiers.
- `Nart2Tx`/`Nart2Rx`/`Nart1Tx`/`Nart1Rx`: These are notifiers for their
  respective serial servers.
- `Clock` and `NTimer`: This is the clock server and notifier which provides the
  `delay`/`time`/`delayUntil` interface.
- `TrMan`: This is the train manager. It buffers commands to the trains and
  creates new tasks for trains when the train is new. It provides provides
  several functions: `void cmdToggleLight(int train)`,
  `void cmdSetSpeed(int train, int speed)`, `void cmdReverse(int train)`.
- `TrainXY`: One of the train tasks created by the train man. These tasks are
  created on the fly and block on the train man for new commands.
- `Timer` and `Counter`: These print the timer and idle time to the terminal.
- `Sensor`: Polls the sensors for new input and prints to the terminal.
- `SSwitch` and `NSwitch`: Server and notifier for the train switch controller.


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


### System Calls

The following are declared in `include/user/task.h` and can be included via
`#include <task.h>`:

- `ErrorOr<Tid> Create(Priority priority, void (*code)());` instantiates a task with
  the given priority and entry point. If the priority is higher than the active
  task's priority, the new task will begin execution immediately. If the
  priority is outside of the valid range, `-ERR_BADARG` is returned. If the
  kernel runs out of task descriptors, `-ERR_NORES` is returned. Currently,
  task descriptors are not recycled, so a maximum of 32 tasks may be created
  ever including the idle and first tasks.
- `Tid MyTid();` returns the task id of the calling task.
- `Tid myParentTid();` returns the task if of the calling task's parent.
- `Error taskInfo(Tid tid, TaskInfo *taskInfo);` fills in a `TaskInfo` struct
  for the given tid. The struct contains the tid, parent tid, priority, user
  time percentage, system time percentage and task state. This is used by the
  `task` and `taskall` commands
- `void pass();` ceases execution of the calling task, but leaves it in the
  ready state. This syscall enables cooperative multitasking.
- `void exeunt();` terminates execution of the task forever. This syscall
  does not return. Currently, task descriptors are not recycled.

The following are declared in `include/user/itc.h` and can be included via
`#include <itc.h>`:

- `ErrorOr<void> send(Tid tid, const T &msg, U &reply)` send an arbitrary
  message to a tid and get an arbitrary reply.
- `ErrorOr<void> receive(Tid *tid, T &msg)` receive an arbitrary message from a
  task.
- `ErrorOr<void> reply(Tid tid, const T &msg)` reply to a message from the
  given task.

The following are declared in `include/user/event.h` and can be included via
`#include <event.h>`:

- `int awaitEvent(Event eventid)`: wait for an external event.
- The `eventid` may be any of the following: `PeriodicTimer` - fires every
  10ms, `Uart1Rx`/`Uart2Rx` - fires on UART receive, or `Uart1Tx`/`Uart1Rx` -
  fires on UART transmit.


### Memory layout

The memory layout is as follows:

    0x00218000 - 0x00218014: data (global variables)
    0x00218014 - 0x00238020: bss (data initialized to zero)
      0x00218020 - 0x00238020: 32 user stacks, each 4096 bytes
    0x00238020 - 0x00244c30: text (program code and readonly data)
    0x00244c30 - 0x01fdcdfc: kernel stack

Most of the memory right now is allocated to the kernel. In the future, we hope
on making more and larger user stacks.

To prevent stack overflow, the source code is compiled with the
`-Wstack-usage=4096`. This generates a compile-time warning if the stack
starting at any function can ever exceed 4096 bytes.


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


### Priorities

The priorities of all tasks can be conveniently found by entering the command
`taskall` and reading the `PRI` column. For reference, here is the output
organized by priority:

- (0) Idle task
- (1) Name server
- (15) First task
- (22) Train manager
- (28) Timer, Counter
- (29) Uart2Tx Server, Uart2Rx Server, Uart1Tx Server, Uart1Rx Server, Sensor,
  Switch Server
- (30) Uart2Tx Notifier, Uart2Rx Notifier, Uart1Tx Notifier, Uart1Rx Notifier,
  Clock Notifier, Switch Notifier

Here is the general pattern on how priorities are organized:

- All the notifier have a higher priority (30) than all else because they
  interact with the hardware and thus have stricter real-time requirements.
- The servers for these notifiers are at the next priority (29) because they
  directly interact with the notifiers.
- The first task (contains the terminal), train manager, timer (prints the
  timer) and counter (prints the idle time) all perform actions which interact
  with the user. The user has slow response time, so these these priorities are
  lower.
- Most tasks will cache the result from the nameserver, thus it is only a
  concern at statup.
- Finally, the idle task must be the lowest priority task because it never
  blocks.


### Interrupts

We use non-vectored interrupts to handle events. Events are an abstraction layer
over interrupts that let tasks listen to specific sequences of interrupts. For
example, the UART 1 Tx event only fires when a proper high-low-high CTS bit
sequence has been achieved. Similarly, since there is only one interrupt per
UART the event abstraction lets us have multiple tasks awaiting on in essence
one interrupts.

To support multiple tasks per event, we use an intrusive linked list. This way,
multiple tasks can await on the same event if necessary.


### IO

There are three messages for serial IO (they can be found in
`include/user/io.h`). The `tid` corresponds to the serial serial server which can
be obtained by querying the nameserver.

- `ErrorOr<int> getc(Tid tid)`: Returns a character from the UART interface or
  an error if there is corrupt data.
- `ErrorOr<void> putc(Tid tid, char c)`: Print a character to the UART interface.
- `ErrorOr<void> flush(Tid)`: Force a flush of the putc buffer.

`putc` does not write to the UART controller immediately. To ensure atomicity,
the io server implements per-task buffers. The `flush` function flushes the
buffer for the current task. Buffers are also flushed for two other reasons:

1. The buffer becomes full. This is 8 bytes for COM1 and 64 bytes for COM2.
2. The newline character is printed. This only applies to COM2.


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


### Circular Buffer

A circular buffer is implemented for Plain Old Data types (in reality trivial
types, less restrictive). It is implemented as a template so that it can be
reused for the many buffering needs of the user/kernel tasks. The size is also
dynamic in this sense, so the buffers are not restricted to a certain size
(the sizes are still static however).


### Parsing

The parsing code is found in `src/first/parse.cpp`. Parsing involves applying
the following stages on each command entered in the prompt:

1. Tokenize the next token in the input string.
2. If the token is expected to be a decimal number, parse to an `int`.
3. Apply domain constraints such as train numbers must be between 1 and 80.

Token information is accessible throughout stage 2 and 3 to give context arrows
for error messages. For example:

    % tr 45 3 nonesense
              ^^^^^^^^^
    Error: too many arguments
    % tr 101 4
         ^^^
    Error: invalid train number
    % nonesense 45 3
      ^^^^^^^^^
    Error: invalid command name


### Fast memcpy

To minimize latency for copying data we implemented a nontrivial memcpy
function. The main difference compared to the original "copy one byte at a
time" is that it tries to copy as many bytes in a single instruction as
possible. To accomplish this, we first assume all copied data is 4 byte
aligned. This is possible through the templated `send`/`receive`/`reply`
functions, as now the compiler can enforce proper alignment of the data types
used. Next, we use a custom memcpy that copies up to 32 bytes at a time through
`ldm` and `stm` commands. The instructions stand for "load multiple" and "store
multiple". We use them to copy up to 8 registers in one go (32 bytes). This
way, there is barely any differences between the 4 byte and 64 byte performance
tests.


## Bugs

- If you enter too many commands, the terminal scrolls out and makes the
  interface look like a mess. It helps if you make the terminal window very
  tall.
- There is a relatively long pause before entering and exiting the prompt. This
  ensures the switches have powered off, but can probably be more streamlined.
- Corrupt data from the UART is treated like regular data.
