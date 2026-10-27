# picking: picking robots, batteries, docks and traffic control (WP-D)

**Owner:** _name_ · **Depends on:** core, ds, navigation · **Used by:** fulfilment, sim, ui

This module contains the mobile picking robots and the rules that allow eight of them to share one floor without collisions and without being permanently blocked by obstacles. Citation keys refer to the *References* section of the root `README.md`.

| File | Content | Data structure / algorithm | Complexity |
|------|---------|----------------------------|------------|
| `PickingRobot.hpp/.cpp` | A single robot: **finite-state machine** with eight states, trip route, cargo and battery | The route is a `ds::LinkedQueue<PickStop>`, i.e. a first-in-first-out queue of shelves [L2]; the class derives from `Robot` [L1b] | O(1) per tick |
| `Battery.hpp` | Charge level in per cent, kept within [0, 100] | value type | O(1) |
| `DockingStation.hpp` | A charger that can be reserved by one robot | holder identifier | O(1) |
| `TrafficController.hpp/.cpp` | Moves every robot by at most one cell per tick while avoiding collisions and obstacles | A\* per robot (`navigation`), occupancy array, `ds::insertionSort` to order the robots [L4] | O(cells + R·(A\* + R)) per tick |

## How it works

**State machine** (see the diagram in `docs/ARCHITECTURE.md`): `IDLE → TO_SHELF → PICKING → (further shelves) → TO_SORTER → DROPPING → RETURNING → IDLE`, together with `TO_DOCK → CHARGING`, which suspends a trip and resumes it afterwards. Each state corresponds to one `case` in `PickingRobot::tick`. A robot never plans its own path: `goal()` states where it intends to go, and the `TrafficController` moves it.

**Traffic rules applied in each tick** (`TrafficController::step`):
1. Robots that need to move are ordered with the longest-waiting robot first, using insertion sort. With at most eight elements that are already nearly sorted from the previous tick, this is the best case of insertion sort [L4].
2. For each robot in turn:
   - (a) A path is planned with A\* if required. A **new obstacle on the planned path** triggers re-planning; this constitutes the obstacle-avoidance behaviour.
   - (b) If the next cell is empty, the robot moves into it.
   - (c) If a **parked** robot occupies the next cell, it is asked to move aside, and the robot then moves.
   - (d) Otherwise the robot waits. After three waits it plans a detour that treats other robots as obstacles. After six waits, if the blocking robot is *also* waiting (a mutual blockage), it steps aside to a free cell, preferably one not on the blocking robot's path.

**Absence of collisions.** A robot enters a cell only if that cell is empty at that moment, and the occupancy array is updated after every individual move. Two robots can therefore never occupy the same cell or exchange cells. `Simulation::checkInvariants` and the tests verify this property in every tick.

**What is not guaranteed.** It is not guaranteed that every blockage is eventually resolved: rule (d) is a heuristic. No blockage persisted in any of the 360 stress-test runs, but this is empirical evidence rather than proof, and the report should state it as such.

## Building and testing this module only

```bash
cmake --build build --target test_picking
ctest --test-dir build -R picking --output-on-failure
```

## Validation

### Hand trace: two robots meeting head-on (test "two robots meeting head-on")

```
      col 012345678
row 1     WT.....TW      A starts at (1,1) with home (1,7); B starts at (1,7) with home (1,1)
row 2     WWWW.WWWW      the only passing place is the side cell (2,4)
```

Predict, tick by tick, what happens under rules 1–2(d) with `replanAfterWaits = 3` and `sidestepAfterWaits = 6`. Then print the positions in the test and compare.

<details><summary>Observed behaviour of the program (seed 2026)</summary>

- Ticks 1–2: the robots move towards each other.
- Tick 3: A reaches (1,4) and B waits at (1,5).
- Ticks 3–7: both robots wait; no detour exists in a corridor one cell wide.
- Tick 8: B has waited six ticks and steps aside. Its only free neighbouring cell is (1,6), which lies on A's path, so the move is unavoidable but unhelpful.
- Further blockages follow.
- Tick 20: A steps aside into the side cell (2,4), and B passes.
- Tick 25: both robots have reached their homes.

The exchange takes 25 ticks, 33 waits and three side-steps for a six-cell journey. **Exercise:** propose a better rule (for example, the robot closer to a free side cell gives way), implement it, and show with this test and with E5 that it reduces waiting.
</details>

### Mutation testing

`python tools/mutation_test.py picking` introduces three defects:
- robots move into occupied cells: detected (collisions);
- a robot retains its cargo after delivery: detected;
- parked robots are never asked to move aside: detected.

Each owner adds further mutants, for example "obstacles on the remaining path are ignored" (rule a). Which test detects it?

### Review questions

- Describe the state transitions for a trip with two shelves in which the battery becomes low on the way to the sorter.
- Why is the route stored in a first-in-first-out queue rather than a stack [L2]?
- Explain the safety argument. Why can two robots never exchange cells?
- Why is insertion sort suitable for ordering the robots in each tick [L4]?
- Why is the side-step choice *random*? What could happen with a deterministic rule when two robots are in mirror-image positions?

### Review for over-engineering

- Four traffic rules and the request to parked robots: are all of them necessary? Remove one at a time and run the stress test (`test_sim`) to determine which ones matter.
- The rule that a technician replaces an empty battery after 30 ticks exists only to prevent the simulation from stalling. Is it realistic, and is it needed if charging works correctly?
- `PickingRobot` provides several query functions (`stopsLeft`, `unitsOnTrip`, `shelvesLeft`, …), mainly for the interface. Which of them could be removed?

### Cross-check with a second generative AI tool

Suggested prompt: *"Several robots move on a grid one step per time unit. Give simple rules that guarantee they never collide, and explain whether they can deadlock."* Compare the response with the rules above. Does the other tool claim freedom from deadlock without proof? This is relevant to the accuracy criterion [B].

## Suggested modifications

1. A better rule for resolving blockages (see the hand trace above), evaluated in E5.
2. Make battery consumption depend on load, `drainPerMove × (1 + cargo kg / 12)`, and update the energy check in `fulfilment/JobManager` accordingly.
3. Add an interface key that stops one robot manually, and observe the others re-planning around it.

## Experiment owned by this work package

- **E5** `experiments traffic`: one to eight robots processing the same 80 orders, measuring completion time, waiting and congestion, and comparing the speed-up with the ideal. The discussion should explain why eight robots achieve a speed-up of about 6.6 rather than 8 (congestion and shared sorters and chargers).
