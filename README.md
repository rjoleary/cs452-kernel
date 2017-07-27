# CS 452 Final Project

Name: The Coldwell Kernel

Students: Ryan O'Leary (rj2olear) and Elnar Dakeshov (edakesho)

Student Ids: 20509502 and 20577578

Date: July 27, 2017


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

Git hash: 5e000a6addf96186e4c1aa0c6f1a46a68bc8bdab


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
- List of reservations for each train
- Command prompt


## Commands

Enter instructions at the `%` prompt. The available instructions are:

- `cal TR SP SEN` - stop train on the next trigger.
- `com i [BYTE...]` - Send arbitrary bytes over COMi.
- `help` - Display this help information.
- `q` - Quit and return to RedBoot.
- `route TR SP SEN [OFF]` - Route train to sensor at given speed with optional offset.
- `rv NUMBER` - Reverse the direction of the train.
- `ssd TR SP NUMBER` - Set the stopping distance in mm.
- `sw NUMBER DIR [TR]` - Set switch direction ('S' or 'C').
- `task (TID|NAME)` - Return info about a task.
- `taskall` - Return info about all tasks.
- `tr NUMBER SPEED` - Set train speed (0 for stop).

Additionally, the Tab key will stop all trains in case of emergency.

## Description

### Features

- multiple trains without collisions
- reverse trains during routing
- free running mode
- set switch with respect to a train

### Layers of Abstraction

Each layer is built from multiple classes and/or tasks:

| Layer       | Description |
| ----------- | ----------- |
| Application | Routes one or more trains between two graph points; oblivious of sensors |
| Safety      | Models all trains locations/velocities, performs sensor attribution, performs reservations |
| Device      | Handles primitive commands (setSpeed, waitSensors, ...) |

For our project, there are three applications:

| Application | Description |
| ----------- | ----------- |
| Terminal    | User types in commands such as `tr 24 10` |
| Calibration | Used as a tool to find a train's stopping distance at various calibrations |
| Routing     | Routes a train from to the given destination and offset |

Why have multiple layers?

- Software abstraction: New applications can be written without changing the
  bottom layers.
- Failure isolation: For example, if the routing algorithm performs a bad route, the
  safety layer will protect the trains against collision. This allows us to
  experiment with more sophisticated routing algorithms.

### Application Layer

Terminal:

Calibration:

Routing:

Path finding finds the shortest path between the start and end nodes. For train
milestone 1, the start and end nodes are always sensors.

Path finding uses Dijkstra's algorithm where the weight is the distance of each
node. Our implementation of Dijkstra's has a memory usage proportional to the
number of nodes in the graph.

The graph used is the one supplied on the course website. Occasionally, when a
sensor or switch is deemed broken, the node's type is modified to relay this
information. This way, a broken sensor/switch does not incur any additional
cost. Also, the complexity of the graph is not altered.

- Exposes a function for routing a train from the terminal.
  - First, the routing layer may query the current state of the train. Not only
    is the optional, but it is not required for milestone 2.
  - Then, using Dijkstra's algorithm, a shortest path tree is generated in
    reverse, rooted on the end node.
  - A Gradient Absolute Switch Profile (GASP) is created from the tree. For
    every switch, the desired direction for passing through the switch is
    stored. Addtionally, the minimal point (the end switch and offset) is
    stored.
  - The GASP is passed to the safety server.
- After initial routing, there are two reasons for rerouting:
  - Contention: The train has stopped to avoid collision. To resolve this
    issue, the routing layer generates a new GASP from a graph with the
    offending node removed.
  - Broken switch: A switch is considered broken if the train diverges from the
    expected path. To resolve this issue, the routing layer generates a new
    GASP from a graph with the offending node removed.

### Safety Layer

- Exposes a function for getting train state. The state contains:
  - current speed of the train
  - estimated velocity of the train
  - estimated stopping distance of the train
  - estimated location of the train
- Sensor attribution
- Models all trains locations/velocities
- Train state extrapolation

### Reservation System (part of the safety layer)

- Stores reservations: a list of track nodes reserved by each train
  - All trains have reservations regardless of whether they are being routed.
    This is required to support free-running mode.
  - The reservation starts from the node immediately behind the train to the
    stopping distance from the next sensor.
  - Whenever the reservation is calculated, the previous reservation is thrown
    out and recreated with
    - Starts from the train's location
    - For each next node starting from the trains current node:
      - Reserve each node and its reverse node.
      - If the node is a branch/merge, lookup in the GASP to determine in which
        direction to route the train.
      - If the node is the first sensor, mark this as the start of the
        stopping distance.
      - Break once the total distance has exceeded the stopping distance.
  - If the train is anywhere on a track node within its stopping distance,
    that track node is considered reserved.
  - When accelerating/deceleration from speed X to Y, the reservation is as
    if the reservation is max(X, Y). Acceleration is assumed to be linear.
  - No two reservations may contain the same track node
- Listens to sensors
  - Update train reservations
- Timeout to prevent contention
  - If a train is blocked on another train's reservation for too long, signal
    to routing server to reroute. The routing server is given the offending
    node.

### Attribution System (part of the safety layer)

### Hazards

Some of these have meaning in the code, the rest are conceptual.

- Train Red Zone Hazard: The system has gotten to a state where trains will
  collide.
- Train Yellow Zone Hazard: The system has gotten to a state where, if no
  action is taken, it will enter the red zone imminently. The safety layer will
  perform an action, such as stopping and/or reversing the train.
- Switch Red Zone Hazard: A train is over a switch and changing the switch's
  state may cause the train the derail.
- Switch Yellow Zone Hazard: A train will soon be over a switch. If a specific
  direction is desired for the switch, it much be changes imminently before the
  switch enters the red zone.

### Solutions

This section outlines several challenges and how the provided system solves
them:

- One train following another: The leader behaves normally. The follower is
  controlled via negative feedback to stay at the cusp of the reserved zone.
  That is, when a train receives a sensor notification and the offending
  train's velocity is in the same direction, the follower matches the leader's
  velocity. To prevent unstable velocities, the following train's speed must
  always decrease.
- Two trains on a collision course: There are three generalized situations in
  which trains may collide:
  - Two trains on a straight: There reserved zones collide and both trains are
    stopped.
  - Two trains on the branches of a switch: One reserved zone is laid down
    first, the second is blocked. One train will wait for the other.
  - One train on the branch, the other on the merge of a switch: This situation
    is graphically identical to the straight case.
- Single failure of sensors: Our reservations are large enough such that they
  contain the next two sensors. When a sensor fails, the next sensor can be
  used for attribution.
- Single failure of a switch: Not solved.

- Rerouting due to contention
  - Query the state of the train
  - Temporarily remove offending nodes from the graph
  - Creates a path from start to end
  - Tell reservation server to change train's speed

Sensor attribution: Whenever the safety layer gets sensor notified, the
following steps are taken:

1. Extrapolate the position of all trains based on time (position & velocity)
2. Figure out which train caused the sensor (the sensor should be on a train's
   reservation)
3. Update that train's position to be exactly over the switch. Also, update the
   velocity of the train to be more exact.
4. Inform the reservation class to update reservations.

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

### Calibrating Velocity

### Calibrating Stopping Distance

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
