# CS 452 Final Project

Name: The Coldwell Kernel

Students: Ryan O'Leary (rj2olear) and Elnar Dakeshov (edakesho)

Student Ids: 20509502 and 20577578

Date: July 27, 2017


## Overview

The Coldwell Kernel implements preemptive multitasking for the ARM 920T CPU.
Tasks may be created, run and exited from. Inter-task communication is based on
three synchronization primitives: send, receive and reply. Tasks may await a
periodic 10ms timer event or block on UART events. Additionally, a constant
time scheduler is implemented with support for up to 32 distinct priorities.

The application supports up to four trains and has been tested with three
trains. The trains may be routed to any node on the track without collision.
Routing supports reversals to provide the most optimal route. Additionally,
trains may enter a free running mode where they drive straight without flipping
switches.


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

Git hash: 73e0d74d0953ec920c169f62bb50c890f654d402


## Running

In RedBoot, run the following:

    > load -b 0x00218000 -h 10.15.167.5 "ARM/rj2olear/coldwell.elf"
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
- free running mode with exit reversal
- set switch with respect to a train

### Layers of Abstraction

Each layer is built from multiple classes and/or tasks:

| Layer       | Description |
| ----------- | ----------- |
| Application | There are 3 applications outlined below |
| Safety      | Models all trains locations/velocities, performs sensor attribution, performs reservations |
| Device      | Handles primitive commands (setSpeed, waitSensors, ...) |

For our project, there are three applications:

| Application | Description |
| ----------- | ----------- |
| Terminal    | User types in commands such as `tr 24 10` |
| Calibration | Used as a tool to find a train's stopping distance at various speeds |
| Routing     | Routes a train to the given destination and offset |

Why have multiple layers?

- Software abstraction: New applications can be written without changing the
  bottom layers.
- Failure isolation: For example, if the routing algorithm performs a bad route, the
  safety layer will protect the trains against collision. This allows us to
  experiment with more sophisticated routing algorithms.

### Application Layers

Terminal:

- Terminal commands are sent to the safety.
- If a terminal command is unsafe, it will be ignored and a error message is
  printed.
- For example: If setting a train's speed will cause a collision, the speed is
  not increased.

Routing:

- Routing finds the shortest path between every node and the end node. This is
  accomplished using Dijkstra's algorithm where the weight is the length of
  each track piece.
- The graph used is the one supplied on the course website. Occasionally, when
  a sensor or switch is deemed broken, the node's type is modified to relay
  this information. This way, a broken sensor/switch does not incur any
  additional cost. Also, the complexity of the graph is not altered.
- Exposes a function for routing a train from the terminal:
  - First, the routing layer may query the current state of the train. This can
    be useful for more sophisticated routing.
  - Then, using Dijkstra's algorithm, a shortest path tree is generated in
    reverse, rooted on the end node.
  - A Gradient Absolute Switch Profile (GASP) is created from the tree. For
    every switch, the desired direction for passing through the switch is
    stored. Addtionally, the minimal point (the end switch and offset) is
    stored.
  - The GASP is passed to the safety server for the given train.
- After initial routing, there are two reasons for rerouting:
  - Contention: The train has stopped to avoid collision. To resolve this
    issue, the routing layer generates a new GASP from a graph with the
    offending node removed.
  - Broken switch: A switch is considered broken if the train diverges from the
    expected path. To resolve this issue, the routing layer generates a new
    GASP from a graph with the offending node removed.

Calibration:

- Calibration is used to find the stopping distance of trains.
- During calibration, a specific switch is selected. When the train drives over
  the switch, a stop signal is sent to the train. The stopping distance is
  measured by hand from the switch's position to the front of the train.
- Since calibration is in the application layer, a train which is being
  calibrated will not collide with other trains.


### Safety Layer

- The safety layer exposes the following functions and ensures they are used in
  a safe manner which will not cause collision:
  - `setTrainSpeed(Train, Speed)`:
  - `reverseTrain(Train)`:
  - `setGasp(Train, Gasp)`: Set all switches with respect to the given train.
  - `setSwitch(Train, Switch, SwitchState)`: Set switch state with respect to
    the given train.
- Additionally, a `getTrainState` function is exposed for getting train state.
  The state contains:
  - last time a sensor was attributed to the train
  - current speed of the train
  - estimated velocity of the train
  - estimated stopping distance of the train
  - estimated position of the train (graph node and offset)
  - last known exact position of the train (graph node)
  - switch states with respect to the train (GASP)
- Sensor attribution (see below)
- Reservations (see below)
- Train discovery (see below)
- Models all trains locations/velocities
- Train state extrapolation

### Reservation System (part of the safety layer)

- Stores reservations: a list of track nodes reserved by each train
  - All trains have reservations regardless of whether they are being routed.
    This is required to support free-running mode.
  - The reservation starts from the node immediately behind the train to the
    stopping distance from the next sensor and is contiguous.
  - Whenever the reservation is calculated, the previous reservation is thrown
    out and recreated with:
    - Starts from the train's location
    - For each next node starting from the trains current node:
      - Reserve each node and its reverse node.
      - If the node is a branch/merge, lookup in the GASP to determine in which
        direction to route the train.
      - If the node is the first sensor, mark this as the start of the
        stopping distance.
      - Break once the total distance has exceeded the stopping distance.
    - The backwards reservation functions in the same manner, but the stopping
      distance is a small constant.
  - If the train is anywhere on a track node within its stopping distance,
    that track node is considered reserved.
  - No two reservations may contain the same track node
- Listens to sensors
  - Update train reservations
- Timeout to prevent contention
  - If a train is blocked on another train's reservation for too long (15s),
    reverse and signal to routing server to reroute. The routing server is
    given the offending node.


### Attribution System (part of the safety layer)

When a sensor is triggered, it is attributed to the train which has the that
sensor in its reservation. Since no two trains may reserve the same sensor,
this unambiguously attributes the sensor.

If the sensor is not reserved by any train, it is considered spurious and the
"unattributed sensor" error message is printed.

More specifically, the following steps are taken for each sensor notification:

1. Extrapolate the position of all trains based on time (position & velocity)
2. Figure out which train caused the sensor (the sensor should be on a train's
   reservation)
3. Update that train's position to be exactly over the switch. Also, update the
   velocity of the train to be more exact.
4. Inform the reservation class to update reservations.

### Train Discovery (part of the safety layer)

When the user sends a command to a train which has never been attributed, a new
state is created for the train. The next occurrence of an unattributed sensor
is attributed to this new train.

It is impossible to discover two trains at once because there would be
ambiguity in the attribution of sensors.

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
    stopped. After 15s the trains reverse to prevent a deadlock.
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

### Measuring Velocity

Initially, before two sensors have been attributed for the train, a rough
estimate is provided for the velocity.

Therein afterwards, the velocity is calculated as the average of the current
estimate and the previous estimate. This means that velocities from previous
track segments dimmish exponentially.

### Calibrating Stopping Distance

Calibration is used to determine the stopping distance for each train at a
given speed. The `cal` command runs a train at the given speed and sends the
stop command at a sensor. The distance is measured between these two reference
points:

- The center of the switch track.
- The front of the train in the direction the train is driving.

For this reason, we must also measure the stopping distance of each train in
reverse.

For accurate results, the average of three trials is taken.

At each sensor, the distance to the end of the route is calculated. With the
velocity, it is possible to estimate the stopping time. If the stopping time
will pass before the next sensor, a delay is entered to stop the train at the
given stopping time.

## Bugs

- Extra debug information is printed all over the terminal.
- Sensor modules D and E are printed to the wrong location on the GUI.
- Trains can only stop properly on Track B.
