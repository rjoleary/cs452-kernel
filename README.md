# CS 452 Kernel 2

Name: The Coldwell Kernel

Students: Ryan O'Leary (rj2olear) and Elnar Dakeshov (edakesho)

Student Ids: 20509502 and 20577578

Date: May 29, 2017


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

Git hash: TODO


## Running

In RedBoot, run the following:

    > load -b 0x00218000 -h 10.15.167.5 "ARM/edakesho/k2.elf"
    > go

For the fastest benchmarks:

    > load -b 0x00218000 -h 10.15.167.5 "ARM/edakesho/k2_bench.elf"
    > go


## Building

    $ make <ARGUMENTS>
    $ stat build/kernel.elf

Where `<ARGUMENTS>` may be any combination of the following:

- `CACHE_ENABLED=1`: Enables the instruction and data caches
- `OPT_ENABLED=1`: Enables optimizations (`-O2` and `-flto`)
- `PERF_TEST=1`: Enables performance testing, prints context switch latency
- `STRACE_ENABLED=1`: Enables printing debug information from the kernel


## Description

### Language

The largest changes between K1 and K2 is the change from C to C++ and upgrading
the compiler to GCC 6. C++ allows the use of a richer type system and other
safety guarantees. To safely use C++ in an embedded system, many features must
be disabled such as RTTI (Run-Time Type Information), exceptions, the STL
(Standard Template Library), exceptions, unwind tables and threadsafe static
variables. Specifically, language features from C++14 are being used.


### Inter-Task Communication (ITC)

The other change to the kernel structure is the addition of `send`, `receive`,
and `reply`. All of these are implemented quite simply. Each task has a state
that is modified whenever it calls a syscall. In the usage of the ITC functions,
the states that are used specifically are `(Send/Receive/Reply)Blocked`. This
lets us know what state a task is in, so that task which is not anticipating a
reply does not receive one and other such things. The only new data structure used
is the send queue, which is created using a two intrusive pointers in the task
descriptor. This way, when a task is send blocked it is put into the receiving
task's queue. This adds very little memory overhead and has constant time
complexity for insertion at the end and removing from the start.

The three ITC functions are also written using C++ templates to ensure that there
are no issues with size misalignment. This way, any structure can be passed and
the size will be computed at compile time.

The nameserver uses an array to internally hold all registered names. This is
sort of a lie however, because there are no registered names at all. Instead, an
enum holds all the symbolic names for the registered program. This way, we know
at compile time the size of the array needed to hold all program registrations
and it makes lookup and insertion constant time.


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

The RPS server has a priority of 5. This number is chosen basically
arbitrarily, as it should just be lower than the first user task but higher
than the clients.  Similarly, the clients have a priority of 2. This assignment
has the benefit of allowing the first task to run to completion before starting
a single game of rock-paper-scissors.

To summarize, the priorities are (lowest to highest):

- `2`: RPS client
- `5`: RPS server
- `15`: first user task
- `31`: name server


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


## Performance Results

		Message     Caches  Send before     Optimization    Time        Group
		length              Reply
		4 bytes     off     yes             off             388.863     Coldwell Group
		64 bytes    off     yes             off             404.531     Elnar Dakeshov
		4 bytes     on      yes             off             25.2        and Ryan O'Leary
		64 bytes    on      yes             off             26.459
		4 bytes     off     no              off             375.835
		64 bytes    off     no              off             391.497
		4 bytes     on      no              off             24.361
		64 bytes    on      no              off             25.638
		4 bytes     off     yes             on              92.501
		64 bytes    off     yes             on              96.033
		4 bytes     on      yes             on              7.414
		64 bytes    on      yes             on              7.723
		4 bytes     off     no              on              91.142
		64 bytes    off     no              on              94.520
		4 bytes     on      no              on              7.314
		64 bytes    on      no              on              7.612


## Demo

Executing the kernel produces the following output:

    FirstUserTask: myTid()=0
    FirstUserTask: created nameserver, tid 1

    ROCK PAPER SCISSORS ROUND 1
       Tid 3 played P
       Tid 4 played S
       Tid 3 standing L
       Tid 4 standing W
    Press any key to continue...
    Tid 3 received standing L
    Tid 4 received standing W

    ROCK PAPER SCISSORS ROUND 2
       Tid 3 played P
       Tid 4 played P
       Tid 3 standing T
       Tid 4 standing T
    Press any key to continue...
    Tid 3 received standing T
    Tid 4 received standing T

    ROCK PAPER SCISSORS ROUND 3
       Tid 3 played Q
       Tid 4 played Q
       Tid 3 standing Q
       Tid 4 standing Q
    Press any key to continue...

    ROCK PAPER SCISSORS ROUND 4
       Tid 5 played R
       Tid 6 played Q
       Tid 5 standing O
       Tid 6 standing Q
    Press any key to continue...
    Tid 5 received standing O

    ROCK PAPER SCISSORS ROUND 5
       Tid 5 played S
       Tid 7 played P
       Tid 5 standing W
       Tid 7 standing L
    Press any key to continue...
    Tid 5 received standing W
    Tid 7 received standing L

    ROCK PAPER SCISSORS ROUND 6
       Tid 5 played R
       Tid 7 played R
       Tid 5 standing T
       Tid 7 standing T
    Press any key to continue...
    Tid 5 received standing T
    Tid 7 received standing T

    ROCK PAPER SCISSORS ROUND 7
       Tid 5 played P
       Tid 7 played Q
       Tid 5 standing O
       Tid 7 standing Q
    Press any key to continue...
    Tid 5 received standing O

    Program completed with status 0

This legend describes the single character output:

- `R` = Rock
- `P` = Paper
- `S` = Scissors
- `W` = Won
- `L` = Lost
- `O` = Opponent Quit
- `Q` = Quit

- Round 1 shows a typical RPS game where someone wins and someone loses.
- Round 2 is a tie.
- Round 3 has both clients quitting instead of throwing, and so they both leave
  the queue.
- Round 4 has a rock being thrown by TID 5, but TID 6 decides to quit right
  away.
- TID 5 is informed of this.
- Round 5 is a normal round again. TID 5 is still playing because it has not
  yet quit.
- Round 6 is a tie.
- Round 7 has TID 7 quit. Now both TID 5 and the server are waiting. TID 5 sent
  a play request to the server, which the server acknowledges but needs another
  client to start a game. Since there are no more clients, the scheduler does
  not have anything to schedule and we return to redboot.

Internally, the rock-paper-scissors server queues client TIDs onto a circular
buffer as they are received. When there are two players to play a game, the
first two TIDs are popped from the queue and placed into another 2-element
queue of active players. A reply is sent to both players to active their tasks
and so they can make a choice. If either player quits, the round ends and
another TID is popped from the queue to replace it.


## Bugs

- The randomness for the Rock-Paper-Scissors game is pseudo-random. Each
  execution of the program will show the exact same players with the exact same
  winners and losers. The determinism is useful for debugging, but not much fun
  overall.


## Attribution

The implementation for the pseudo-random number generator `prng` in
`src/rps/main.c` is a derivate of that found on [Wikipedia
Xorshift](https://en.wikipedia.org/wiki/Xorshift).
