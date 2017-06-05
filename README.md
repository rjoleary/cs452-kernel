# CS 452 Kernel 3

Name: The Coldwell Kernel

Students: Ryan O'Leary (rj2olear) and Elnar Dakeshov (edakesho)

Student Ids: 20509502 and 20577578

Date: June 5, 2017


## Overview

This program implements multitasking for the ARM 920T CPU. Tasks may be
created, run and exited from. Tasks may await a timer event which periodically
fires every 10ms. Additionally, a constant time scheduler is implemented with
support for up to 32 distinct priorities. Example tasks have been included to
demonstrate the functionality of the clock (see the demo section).


## Download Path

    git clone https://git.uwaterloo.ca/coldwell/kernel.git


## Checksums

Git hash: bd3f30b67349214c7da428b1ca05de34af07cc71


## Running

In RedBoot, run the following:

    > load -b 0x00218000 -h 10.15.167.5 "ARM/rj2olear/k3.elf"
    > go


## Building

    $ make <ARGUMENTS>
    $ stat build/kernel.elf

Where `<ARGUMENTS>` may be any combination of the following:

- `CACHE_ENABLED=1`: Enables the instruction and data caches
- `OPT_ENABLED=1`: Enables optimizations (`-O2` and `-flto`)
- `STRACE_ENABLED=1`: Enables printing debug information from the kernel


## Description

### Interrupts

We are using vectored interrupts. During kernel initialization, we enable and
prioritize all the interrupts we care about (`TC1UI`). When a task syscalls
`awaitEvent()`, the pointer to the task descriptor is stored in the vectored
interrupt address for the interrupt source.

When the interrupt occurs, we retrieve the task pointer from the interrupt
vector. To determine the interrupt source (type of interrupt), it is stored as
the first argument, `r0` of the blocked task (which has been pushed to the
stack).

We make the assumption that only one task will register to an event at a time,
which is fine if we use the notifier-server-consumer pattern. We currently only
support one interrupt type right now, but the system is expandable for multiple
interrupt sources. For the next kernel, we will need to implement the UART
interrupt source.


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


## Demo

Executing the kernel produces the following output:

    Build string: '21:27:13 Jun  4 2017 CACHE=1 OPT=1'
    Tid: 5, MyDelay: 10, DelayNum: 1
    Tid: 5, MyDelay: 10, DelayNum: 2
    Tid: 6, MyDelay: 23, DelayNum: 1
    Tid: 5, MyDelay: 10, DelayNum: 3
    Tid: 7, MyDelay: 33, DelayNum: 1
    Tid: 5, MyDelay: 10, DelayNum: 4
    Tid: 6, MyDelay: 23, DelayNum: 2
    Tid: 5, MyDelay: 10, DelayNum: 5
    Tid: 5, MyDelay: 10, DelayNum: 6
    Tid: 7, MyDelay: 33, DelayNum: 2
    Tid: 6, MyDelay: 23, DelayNum: 3
    Tid: 5, MyDelay: 10, DelayNum: 7
    Tid: 8, MyDelay: 71, DelayNum: 1
    Tid: 5, MyDelay: 10, DelayNum: 8
    Tid: 5, MyDelay: 10, DelayNum: 9
    Tid: 6, MyDelay: 23, DelayNum: 4
    Tid: 7, MyDelay: 33, DelayNum: 3
    Tid: 5, MyDelay: 10, DelayNum: 10
    Tid: 5, MyDelay: 10, DelayNum: 11
    Tid: 6, MyDelay: 23, DelayNum: 5
    Tid: 5, MyDelay: 10, DelayNum: 12
    Tid: 5, MyDelay: 10, DelayNum: 13
    Tid: 7, MyDelay: 33, DelayNum: 4
    Tid: 6, MyDelay: 23, DelayNum: 6
    Tid: 5, MyDelay: 10, DelayNum: 14
    Tid: 8, MyDelay: 71, DelayNum: 2
    Tid: 5, MyDelay: 10, DelayNum: 15
    Tid: 5, MyDelay: 10, DelayNum: 16
    Tid: 6, MyDelay: 23, DelayNum: 7
    Tid: 7, MyDelay: 33, DelayNum: 5
    Tid: 5, MyDelay: 10, DelayNum: 17
    Tid: 5, MyDelay: 10, DelayNum: 18
    Tid: 6, MyDelay: 23, DelayNum: 8
    Tid: 5, MyDelay: 10, DelayNum: 19
    Tid: 7, MyDelay: 33, DelayNum: 6
    Tid: 5, MyDelay: 10, DelayNum: 20
    Tid: 6, MyDelay: 23, DelayNum: 9
    Tid: 8, MyDelay: 71, DelayNum: 3

    Time usage (measured in 508 kHz ticks):
      TID   PTID    User    Sys
      0     0       21      25
      1     0       463471  448928
      2     0       8       17
      3     0       402     376
      4     0       223     385
      5     0       31353   31
      6     0       14111   18
      7     0       9394    12
      8     0       4721    7

### Description

Since each number is coprime, the lowest common multiple of all 4 is tick 230,
which is never reached so now two tasks will be awoken on the same tick will
occur on the same tick. This means the client task's priorities are irrelevant
in our analysis.

The resulting output then is quite obvious, each task prints whenever its delay
has been reached. If we multiply any `MyDelay` with `DelayNum` and compare it
to another such multiple, we will find that the results are strictly
increasing, as they should be.

At the end of the output, the user time and system time used by each TID is
printed. This measurement uses timer 2 where each tick is 508 kHz. System time
is the total time spent servicing a syscall for the given task. It is important
to measure both times because some tasks spend a lot of their time making
syscalls.


## Bugs

- The kernel successfully returns to RedBoot after completing. However, if you
  attempt to rerun the kernel from RedBoot, it will instantly crash.
- The systime metric in the time usage is not entirely accurate because a task
  is billed for an interrupt awaited on by another task.
