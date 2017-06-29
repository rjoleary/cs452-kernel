# CS 452 Train Milestone 1

Name: The Coldwell Kernel

Students: Ryan O'Leary (rj2olear) and Elnar Dakeshov (edakesho)

Student Ids: 20509502 and 20577578

Date: June 29, 2017


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


## Download Path

    git clone https://git.uwaterloo.ca/coldwell/kernel.git


## Checksums

Git hash: 4a126afb65cc395d5031602d5537f26bf7b71529


## Running

In RedBoot, run the following:

    > load -b 0x00218000 -h 10.15.167.5 "ARM/edakesho/tm1.elf"
    > go


## Building

    $ make <ARGUMENTS>
    $ stat build/kernel.elf

Where `<ARGUMENTS>` may be any combination of the following:

- `CACHE_ENABLED=0`: Disables the instruction and data caches
- `OPT_ENABLED=0`: Disables optimizations


## Interface

The following convenient features can be found on the user interface:

- Clock display in tenths of a second
- GO/STOP, toggle with Tab key
- Percent user time spent in the idle task compared to other tasks
- Top 10 most recent sensor triggers
- Track A layout including 22 switch states and broken sensors
- Command prompt


## Commands

Enter instructions at the `%` prompt. The available instructions are:

- `cal TR SP SEN` - stop train on the next trigger.
- `com i [BYTE...]` - Send arbitrary bytes over COMi.
- `help` - Display this help information.
- `li NUMBER` - Toggle train lights.
- `q` - Quit and return to RedBoot.
- `route TR SP SEN` - Route train to sensor at given speed.
- `rv NUMBER` - Reverse the direction of the train.
- `ssss NUMBER` - Set the stopping distance in mm.
- `sw NUMBER DIR` - Set switch direction ('S' or 'C').
- `task (TID|NAME)` - Return info about a task.
- `taskall` - Return info about all tasks.
- `tr NUMBER SPEED` - Set train speed (0 for stop).
- `unbreak` - Mark all sensors as unbroken.
 
Additionally, the Tab key will stop all trains in case of emergency.

## Description

### Path finding

Path finding finds the shortest path between the start and end nodes. For train
milestone 1, the start and end nodes are always sensors. 

Path finding uses Dijkstra's algorithm where the weight is the distance of each
node. Our implementation of Dijkstra's has a memory usage proportional to the
number of nodes in the graph.

### Calibration

Calibration is used to determine the stopping distance for each train at a
given speed. The `cal` runs a train at the given speed and sends the stop
command at a sensor. The distance is measured between these two reference
points:

- The center of the switch track.
- The front of the train in the direction the train is driving.

For this reason, we must also measure the stopping distance of each train in
reverse.

For accurate results, the average of three trials is taken.


### Stopping Distance

The results from calibration provided these stopping distances:

- Train 63:
    - Speed: 10
        - Forward: 807 mm
        - Reverse: 899 mm
    - Speed: 12
        - Forward: 974 mm
        - Reverse: 1049 mm
- Train 71
    - Speed: 10
        - Forward: 494 mm
        - Reverse: 625 mm
    - Speed: 12
        - Forward: 807 mm
        - Reverse: 953 mm

At each sensor, the distance to the end of the route is calculated. With the
velocity, it is possible to estimate the stopping time. If the stopping time
will pass before the next sensor, a delay is entered to stop the train at the
given stopping time.


## Bugs

- Extra debug information is printed all over the terminal.
- Sensor modules D and E are printed to the wrong location on the GUI.
- There is a relatively long pause before entering and exiting the prompt. This
  ensures the switches have powered off, but can probably be more streamlined.
- Corrupt data from the UART is treated like regular data.
- Trains can only stop properly if they are driving forwards.
