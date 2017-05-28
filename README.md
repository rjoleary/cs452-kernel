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

    > load -b 0x00218000 -h 10.15.167.5 "ARM/edakesho/k2.elf"
    > go

For the fastest benchmarks:

    > load -b 0x00218000 -h 10.15.167.5 "ARM/edakesho/k2_bench.elf"
    > go

## Building

    $ make
    $ stat build/kernel.elf


## Description

### Language
The largest change between K1 and K2 is that we changed languages from C to C++,
as well as upgraded our compiler to GCC 6. We are disabling all unnecessary
features and instead using the richer type system as well as other safety
guarantees. This means we have no RTTI, exceptions, or STL, as well as other
things such as unwind tables and threadsafe statics. We are also using C++14 for
various language features.

### Intertask communication
The other change to the kernel structure is the addition of `send`, `receive`,
and `reply`. All of these are implemented quite simply. Each task has a state
that is modified whenever it calls a syscall. In the usage of the ITC functions,
the states that are used specifically are `(Send/Receive/Reply)Blocked`. This
lets us know what state a task is in, so that task which is not anticipating a
reply doesn't receive one and other such things. The only new data structure used
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
Then, the nameserver is created with a priority of 31. The reason the nameserver
has such a high priority is because we don't anticipate that it will be used too
often, so having it respond quickly is not an issue. On top of this, we didn't
want to have a high priority task being blocked by a lower one just because the
nameserver is taking a long time to respond. Therefore, we went to the logical
extreme of setting it to the highest priority, so that no task will have to wait
on the nameserver.

The RPS server has a priority of 5. This number is chosen basically arbitrarily,
as it should just be lower than the first user task but higher than the clients.
Similarly, the clients have a priority of 2.

### memcpy
To minimize time copying data we implemented a custom memcpy function. The main
difference is that it tries to copy as many bytes in a signle instruction as
possible. For this to work properly, we first assume all copied data is 4 byte
aligned. This is possible through the templated `send/receive/reply` functions,
as now the compiler can enforce proper alignment of the datatypes used. Next,
we use a custom memcpy that copies up to 32 bytes at a time through `ldm` and
`stm` commands. This way, there is barely and differences between the 4 byte
and 64 byte tests.

## Measurements

TODO

## Demo
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

R = Rock
P = Paper
S = Scissors
O = Opponent Quit
Q = Quit
Round 1 shows a typical RPS game where someone wins and someone loses.
Round 2 is a tie.
Round 3 has both clients quitting instead of throwing, and so they both leave
the queue.
Round 4 has a rock being thrown by TID 5, but TID 6 decides to quit right away.
TID 5 is informed of this.
Round 5 is a normal round again. TID 5 is still playing because it hasn't quit.
Round 6 is a tie.
Round 7 has TID 7 quit. Now both TID 5 and the server are waiting. TID 5 sent a
play request to the server, which the server acknowledges but needs another
client to start a game. Since there are no more clients, the scheduler does not
have anything to schedule and we return to redboot.

The reason why previous clients get to keep playing until they quit is TODO

## Bugs

- No known bugs.
