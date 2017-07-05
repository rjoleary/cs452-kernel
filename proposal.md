# CS452 Project Proposal - Pokemon TRAINer

Students: Ryan O'Leary (rj2olear) and Elnar Dakeshov (edakesho)

Student Ids: 20509502 and 20577578

Date: July 5, 2017

Project Type: Game

## Overview

- Each Pokemon has a weight of how good that Pokemon is.
- The software randomly generates Pokemon on the graph. The Pokemon are virtual
  and their location is printed to the GUI as a colourful character. Their
  movement depends on the type of Pokemon:
    - Land Pokemon types must follow the graph at a set velocity and randomly
      branches.
    - Air Pokemon types can jump between graph segments which are geographically
      close, but not necessarily connected.
- Trains are Pokemon trainers. Their goal is to collect Pokemon.
- There are multiple trains on the track at once and they are adversarial. They
  race each other to collect the Pokemon as they appear. To collect a Pokemon,
  the train stops on top of it.
- Trains must not collide! However, in their attempt to reach the same Pokemon,
  they might have to stop to avoid a collision. When two trains stop near each
  other, a battle ensues!
- With the terminal is also possible to manually engage two trains in a battle.
- The train with the best total weight of its Pokemon wins the battle. The
  losing trainer must give a random Pokemon to the winning train and retreat.
- The GUI will display:
    - Pokemon locations
    - Pokemon collected by each train
    - Ongoing battles between the trains


## Technical Challenges

1. Multiple trains are on the track at once and often times will be rapidly
   pursuing the same Pokemon. We need some mechanism to identify hazards and
   stop trains before they collide.
2. The trainers are adversarial, so we need to write a decent AI to select
   which Pokemon to pursue. For example, should the train attempt to collect a
   Pokemon further out of its way because it is stronger? This is an
   optimization challenge. The Pokemon also follow branches unpredictably which
   adds to the excitement.
3. The trains are attempting to stop on a moving Pokemon while they are moving
   themselves.


## Technical Solutions

Each point corresponds to the same number in the previous section.

1. Hopefully, this is a challenge we complete with milestone 2.
2. This adds onto the existing path finding implementation. The weight of the
   Pokemon subtracts from the length of the path. As for a branch
   misprediction, we want to be on the path which leads to the next most
   optimal Pokemon.
3. If we measure stopping distance as a function of the train's velocity,
   `S(t_v)`, then the stopping time relative `t` to the moving Pokemon
   (velocity `p_v` and position `p_d`) becomes `t = (p_d-p_v*t-S(t_v))/t_v`
   (solve for `t`). The equation may need some tweaking, but is certainly
   possible when you take into account relative velocities.


## Appendix: What is Pokemon?

If the reader is unfamiliar with the enchanting game from Nintendo, here is a quick
rundown:

- `Pokemon` (plural `Pokemon`) are fantasy animals each with their own unique
  ability. Probably the most famous Pokemon is Pikachu, but there are many
  others such as Charmander, Bulbasaur and Mew.
- `Pokemon Trainers` travel the corners of the map in search of Pokemon. They
  collect the Pokemon in `Pokeballs` with the hope of one day "catching 'em
  all".
- `Pokemon Battles` take place between Pokemon trainers to determine who is the
  best Pokemon trainer. Their Pokemon aggressively fight each other; but
  thankfully, since it is a PG video game, the violence is fairly timid.
