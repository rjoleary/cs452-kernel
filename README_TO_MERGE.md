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
