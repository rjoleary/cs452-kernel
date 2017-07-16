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

Git hash: bb0036ebb6f19682060ed73e56c3adb2f28abfa0


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

### New servers

Features:
- multiple trains without collisions
- routing supports reversing a train
- short stops
- free running mode

TrackMan - Delete it

Reservation Server
- Stores reservations
  - Reservation: list of track nodes reserved by a train
    - All trains have reservations regardless of whether they are being routed.
    - For each track piece, it stores the absolute time the trains will be
      expected to enter and leave it.
    - The reservation:
      - Starts from the train's location
      - For each next node starting from the trains current node:
        - If the node is a branch/merge, call "queryExpectedSwitchState" on the
          routing layer.
        - If the node is the first sensor, mark this as the start of the
          stopping distance.
        - Break once the total distance has exceeded the stopping distance.
    - The reservation starts from where you are to the stopping distance from
      the next sensor.
    - If the train is anywhere on a track node within its stopping distance,
      that track node is considered reserved.
    - When accelerating/deceleration from speed X to Y, the reservation is as
      if the reservation is max(X, Y). Acceleration is assumed to be linear.
  - No two reservations may contain the same track node
- Provides function for getting the reservations for all trains
  - MAX_CONCURRENT_TRAINS * 15 2D array
- Provides function to switch a switch
  - If the switch is reserved, the switch is switched.
- Listens to switches
  - Update train reservations
- Listens to sensors
  - Update train reservations
- Timeout
  - If train is blocked on another train's reservation for too long, signal to
    routing server to reroute. The routing server is given the offending
    reservation.

Routing Server
- Exposes function for creating routes
  - Query the state of the train
  - Creates a path from start to end
  - Tell reservation server to change train's speed
- Rerouting due to offending reservation
  - Query the state of the train
  - Temporarily remove offending nodes from the graph
  - Creates a path from start to end
  - Tell reservation server to change train's speed
- Exposes function "queryExpectedSwitchState", function of the train number and
  switch state.
  - Four outcomes: don't care, straight/curved (only for branches), reverse
    (only for merges)

Model Server
- Exposes function for getting train state
- Exposes function for listening on sensors for a given train
- Exposes function for listening on hazards for a given train


Solutions:
- One train following another: Train
- Two trains on a collision course:
- Short routes with much switching
- Single failure of sensors
- Single failure of switch
- There are one or more switches in the path


### Layers of Abstraction

Each layer is built from multiple classes and/or tasks:

| Layer       | Description |
| ----------- | ----------- |
| Routing     | Routes one or more trains between two graph points; oblivious of sensors |
| Reservation |
| Model       | Models all trains locations/velocities, performs sensor attribution, extrapolates based on sensors and reports on hazards |
| Device      | Handles primitive commands (setSpeed, waitSensors, ...) |

Why?

- Software abstraction: Most of the model layer will likely stay the same
  regardless of which project we choose. However, the routing layer may change
  from performing path finding to something else.
- Failure isolation: For example, if the routing algorithm performs a bad route, the
  model layer will protect the trains against collision. This allows us to
  experiment with more sophisticated routing algorithms.


### Path finding

Path finding finds the shortest path between the start and end nodes. For train
milestone 1, the start and end nodes are always sensors.

Path finding uses Dijkstra's algorithm where the weight is the distance of each
node. Our implementation of Dijkstra's has a memory usage proportional to the
number of nodes in the graph.

The graph used is the one supplied on the course website. Occasionally, when a
sensor or switch is deemed broken, the node's type is modified to relay this
information. This way, a broken sensor/switch does not incur any additional
cost. Also, the complexity of the graph is not altered.

### Rerouting and Broken Tracks

Occasionally, sections of the track break down. This is especially important
when the broken sections are sensors or switches, as they impact pathfinding and
other train operations. To circumvent this, the train manager attempts to detect
and circumvent such breakdowns. If an expected sensor fails to report, it
becomes marked as broken. Similarly, if the train goes on an unexpected path,
the train manager assumes that the switch that connects the past location with
the current is broken. Note that it only assumes a switch is broken if there are
no broken sensors before it (as a broken sensor can impact the control of a
switch). If anything fails, the train attempts to reroute (if it was on a route).

### Track Manager

While the train is moving, the track manager attempts to predict the next sensor
to be hit. This way, the manager can know at any point in time where a train is
(to the accuracy of being between two sensors). It also keeps track of the
velocity of the last sensor segment covered, which is used for stopping and
other operations. The track manager also maintains the routing information.

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
