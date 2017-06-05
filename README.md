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

Git hash: TODO


## Running

In RedBoot, run the following:

    > load -b 0x00218000 -h 10.15.167.5 "ARM/TODO"
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

TODO


### Clock

TODO


### Min Heap

TODO


## Demo

Executing the kernel produces the following output:

    Build string: '21:27:13 Jun  4 2017 CACHE=1 OPT=1'
    Tid: 5, MyDelay: 10, TotalDelay: 10
    Tid: 5, MyDelay: 10, TotalDelay: 20
    Tid: 6, MyDelay: 23, TotalDelay: 23
    Tid: 5, MyDelay: 10, TotalDelay: 30
    Tid: 7, MyDelay: 33, TotalDelay: 33
    Tid: 5, MyDelay: 10, TotalDelay: 40
    Tid: 6, MyDelay: 23, TotalDelay: 46
    Tid: 5, MyDelay: 10, TotalDelay: 50
    Tid: 5, MyDelay: 10, TotalDelay: 60
    Tid: 7, MyDelay: 33, TotalDelay: 66
    Tid: 6, MyDelay: 23, TotalDelay: 69
    Tid: 5, MyDelay: 10, TotalDelay: 70
    Tid: 8, MyDelay: 71, TotalDelay: 71
    Tid: 5, MyDelay: 10, TotalDelay: 80
    Tid: 5, MyDelay: 10, TotalDelay: 90
    Tid: 6, MyDelay: 23, TotalDelay: 92
    Tid: 7, MyDelay: 33, TotalDelay: 99
    Tid: 5, MyDelay: 10, TotalDelay: 100
    Tid: 5, MyDelay: 10, TotalDelay: 110
    Tid: 6, MyDelay: 23, TotalDelay: 115
    Tid: 5, MyDelay: 10, TotalDelay: 120
    Tid: 6, MyDelay: 23, TotalDelay: 115
    Tid: 5, MyDelay: 10, TotalDelay: 120
    Tid: 5, MyDelay: 10, TotalDelay: 130
    Tid: 7, MyDelay: 33, TotalDelay: 132
    Tid: 6, MyDelay: 23, TotalDelay: 138
    Tid: 5, MyDelay: 10, TotalDelay: 150
    Tid: 5, MyDelay: 10, TotalDelay: 160
    Tid: 6, MyDelay: 23, TotalDelay: 161
    Tid: 7, MyDelay: 33, TotalDelay: 165
    Tid: 5, MyDelay: 10, TotalDelay: 170
    Tid: 5, MyDelay: 10, TotalDelay: 180
    Tid: 6, MyDelay: 23, TotalDelay: 184
    Tid: 5, MyDelay: 10, TotalDelay: 190
    Tid: 7, MyDelay: 33, TotalDelay: 198
    Tid: 5, MyDelay: 10, TotalDelay: 200
    Tid: 6, MyDelay: 23, TotalDelay: 207
    Tid: 8, MyDelay: 71, TotalDelay: 213

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

Description: TODO


## Bugs

- The kernel successfully returns to RedBoot after completing. However, if you
  attempt to rerun the kernel from RedBoot, it will instantly crash.
