# icfpc2020
Closed and Restricted Boltzmann Machine Code Repo for ICFP Contest 2020

## Participants (in alphabetinal order)

- Vladislav Isenbaev
- Evgeniy Krasko
- Alexander Pivovarov
- Alexey Poyarkov
- Den Raskovalov

## Algorithm description

# Short recap of the rules

Each game we particiapte in, we have the role of either an attacker or a defender. Attacker needs to destroy defender's fleet within T time ticks, defender needs to either destroy attacker within this time frame or just not be fully destroyed in that time. Attacker has a budget of 512, while defender's budget is 448. The budget can be spent on:

- fuel (cost 1 point of budget each)
- laser (cost 4 points of budget each)
- radiators (cost 12 points of budget each) - we called them "regen" in the code
- engines (cost 2 points of budget each) - we called them "lives" in the code

# High level idea of the algorithm

The main approach is to construct a swarm of small ships with 1 engine and possibly a little bit of fuel in each; they won't need fuel to orbit the sun. The idea behind it is that it will be difficult for the opponent to hunt down many small ships + small ships are great to be used to destroy enemy fleet with self destruction. More specifically:

- As a defender, the strategy spends all of 448 budget on swarm ships requesting 300 fuel and 74 engines (engines := budget // 6, all that left is fuel) with no lasers or radiators.
- As an attacker, the strategy splits the budget into 2 pieces:
  - 246 points of budget is spent on a separate ship with lasers with [20, 32, 8, 1] configuration. The idea behind this was that such a ship with lasers might be useful to destroy the last few enemy ships if they turn out to be on the trajectories different from our surviving swarm ships.
  - 266 points of budget is spent on swarm ships

# Navigation

The main functionality used in navigation of the ships is provided by orbit_util.py. When ran as a separate program it generates dist_to_good.npz binary data file which contains precomputed 4d integer array which for every tuple (x, y, vx, vy) specifies how many engine thrusts are needed to achieve "good" trajectory from position (x, y) if current velocity is (vx, vy). A tuple (x, y, vx, vy) is considered "good" if a ship won't go closer than 100 points to sun within 512 time ticks without spending any fuel.

Only tuples (x, y, vx, vy) with -128 <= x, y <= 128 and -10 <= vx, vy <= 10 are considered in dist_to_good.npz 

# Swarm strategy

Swarm strategy (swarmer.py) each turn splits all ships into groups described by their (x, y, vx, vy) tuples. If more than 1 ship shares same tuple, their first priority is to split trajectories. Otherwise, if a ship has more than 1 engine, it divides itself into 2 roughly equal parts. If both the trajectory is ok and dividing is not possible, then the ship just continues moving with no fuel being spent, checking if self destruction is reasonable + checking if they can shift their trajectory using a bit of fuel to come closer to an enemy ship and self-destruct in the future (aka "homing").

## Splitting trajectories

If more than 1 ship in the swarm shares an "orbit" (i.e. position + velocity (x, y, vx, vy) tuple), then the first priority for them is to split their trajectories. The algorithm for that is finding all possible engine thrusts from (x, y, vx, vy) which would lead to a reasonably good new (x, y, vx, vy) tuple (i.e. with the smallest possible "dist_to_good"). If that best distance is reasonably good (not more than 3 for larger ships in the beginning of the battle, or exactly 0 for smaller ships), then all possible thrusts leading to good orbits are shuffled and all ships except 1 apply one of such thrusts (if there are too many ships sharing the orbit, then some of them will have to apply the same thrust).

If only one ship is on an orbit, but the orbit is not "good", it will try to aply an engine thrust to get closer to a "good" orbit.

## Dividing the ship into smaller ships

If the ship is on a good orbit and doesn't share it with other swarm ships, and the number of engines/lives is more than 1, then the ship will divide into 2 roughly equal ships.

## Self destruction

For each small swarm ship, we pick the enemy which on the next turn is expected to be closest, and check whether it makes sense to self-destruct our swarmer ship: first, if it will damage the enemy at all, second if the "gain" from self-destruction is positive. We also then note all enemy ships which will be damaged by this and (greedily) take that into account when deciding if we should have other ships self-destruct as well. The "gain" is 2x of damage we expect to cause to the enemy minus the damage we expect to cause to our ships.

## Homing

After 64th time tick in the game, the swarmer ships with fuel left are considering "homing" - spending a bit of fuel to change their trajectory in order to get closer to enemy ships after a few time ticks. Every turn all enemy ships' trajectories are traced 16 time ticks into the future (assuming they don't use engines to change their trajectories). Using these trajectories, we consider all possible thrusts and trace the trajectories 16 ticks for them as well. If we can make minimum distance to some enemy not more than 3 (and that won't be the case without using the engines), then we pick one of such trajectory adjustment thrusts and apply it.

# Attacker's ship with the lasers substrategy

# Enemy movement prediction

A simple, but relatively effective algorithm is used for each enemy ship to predict its use of thrusters on the next turn using ThrustPredictor class (one predictor per enemy ship). Each turn it registers the ship's thrust (or (0, 0) if thrust was not used). It then adds a 1.0 point for each of 8 "predictors" saying "thrust is same as T-1", ..., "thrust is same as T-8" if that predictor is correct. Then all predictors are multiplied by 0.95 decay. Then we pick the highest predictor's score and use its prediction for the next turn engine thrust of the enemy ship. This way we can predict very well the cases of enemies using their engines in a cyclic way with a short period.
