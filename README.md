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
- `PERF_TEST=1`: Enables performance testing, prints context switch latency
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

    TODO

## Bugs

- No known bugs
