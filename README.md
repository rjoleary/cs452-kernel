# CS 452 Kernel 3

Name: The Coldwell Kernel

Students: Ryan O'Leary (rj2olear) and Elnar Dakeshov (edakesho)

Student Ids: 20509502 and 20577578

Date: June 5, 2017


## Overview

This program implements non-preemptive multitasking for the ARM 920T CPU.
Tasks may be created, run and exited from. Tasks must yield occasionally to
allow other task of the same priority to execute. Additionally, a constant time
scheduler is implemented with support for up to 32 distinct priorities. Example
tasks have been included to demonstrate these functionalities.

In addition to the features from the previous kernel, support for inter-task
communication (ITC) has been added. A demo application which plays
rock-paper-scissors has been included to demonstrate this functionality.

Furthermore, the kernel supports compiler optimizations (`-O2` and `-flto`) and
memory caches. Profiles for the context switch has been included in this
readme.


## Download Path

    git clone https://git.uwaterloo.ca/coldwell/kernel.git


## Checksums

Git hash: 177faedb42a41904e731493f61eedb572b7e8baf


## Running

In RedBoot, run the following:

    > load -b 0x00218000 -h 10.15.167.5 "ARM/edakesho/k3.elf"
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

We are using vectored interrupts. At program start, we enable and prioritize all
the interrupts we care about. When a task syscalls `awaitEvent()`, that task's
descriptor is stored in the vectored interrupt of the task. Then, when the
interrupt occurs we retrieve the TID from the interrupt vector. We then retrieve
the first argument the task used (the one that was used to invoke
`awaitEvent()`) and notify the task accordingly. We make the assumption that
only one task will register to an event at a time, which is fine if we use the
notifier-server-consumer paradigm. We currently only support one interrupt right
now, but the system is expandable for multiple interrupt sources.


### Clock

We use clock timer 2 for our tick measurements. Since this is a 2kHz clock, we
set the rollover time to 19, so that it will fire off an interrupt every 20
counts (which translates to once every 10ms, the length of a tick). A notifier
task listens to this event, and when it occurs it sends an empty message to the
clock server. The clock server then increments its time and notifies any waiting
tasks if appropriate. 


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

Since each number is coprime, the lcm of all 4 is tick 230, which is never
reached so the priority is a non factor. The resulting output then is quite
obvious, each task prints whenever its delay has been reached. If we multiply
any `MyDelay` with `DelayNum` and compare it to another such multiple, we will
find that the results are strictly increasing, as they should be.


## Bugs

- The kernel successfully returns to RedBoot after completing. However, if you
  attempt to rerun the kernel from RedBoot, it will instantly crash.
- The systime metric in the time usage is not entirely accurate right now