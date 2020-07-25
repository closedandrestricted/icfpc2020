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

Each game we particiapte in we have either the role of attacker or defender. Attacker needs to destroy defender's fleet within T time ticks, defender needs to either destroy attacker within this time frame or just not be fully destroyed in that time. Attacker has a budget of 512, while defender's budget is 448 where budget can be spent on one of:

- fuel (cost 1 point of budget each)
- laser (cost 4 points of budget each)
- radiators (cost 12 points of budget each) - we called them "regen" in the code
- engines (cost 2 points of budget each) - we called them "lives" in the code

# High level idea of the algorithm

Main approach is to construct a swarm of small ships with 1 engine and possibly a little bit of fuel in each which don't need fuel to orbit the sun. The idea behind it is that it will be difficult for the opponent to hunt down many small ships + small ships are great to be used to destroy enemy fleet with self destruction. More specifically:

- As a defender the strategy spends all of 448 budget on swarm ships requesting 300 fuel and 74 engines (engines := budget // 6, all that left is fuel) with no lasers or radiators.
- As an attacker the strategy splits the budget into 2 pieces:
  - 246 points of budget is spent on a separate ship with lasers with [20, 32, 8, 1] configuration. The idea behind this was that such a ship with lasers might be useful to destroy the last few enemy ships if they turn out to be on the trajectories different from our surviving swarm ships.
  - 266 points of budget is spent on swarm ships

# Navigation

orbit_util.py provides main functionality used in navigation of the ships. When ran as a separate program it generates dist_to_good.npz binary data file which contains precomputed 4d integer array which for every tuple (x, y, vx, vy) specifies how many engine thrusts are needed to achieve "good" trajectory from position (x, y) if current velocity is (vx, vy). A tuple (x, y, vx, vy) is considered "good" if a ship won't go closer than 100 points to sun within 512 time ticks without spending any fuel.

# Enemy movement prediction

