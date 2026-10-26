# picking: picking robots, batteries, docks and traffic control (WP-D)

**Owner:** _name_ · **Depends on:** core, ds, navigation · **Used by:** fulfilment, sim, ui

The mobile robots and the rules that let eight of them share one floor without crashing or getting stuck behind obstacles.

| File | What | Data structure / algorithm | Big-O |
|------|------|----------------------------|-------|
| `PickingRobot.hpp/.cpp` | One robot: **finite-state machine** (8 states), trip route, cargo, battery | route is a `ds::LinkedQueue<PickStop>` (FIFO of shelves) | O(1) per tick |
| `Battery.hpp` | Level in % clamped to [0, 100] | value type | O(1) |
| `DockingStation.hpp` | A charger that one robot can reserve | holder id | O(1) |
| `TrafficController.hpp/.cpp` | Moves all robots ≤ 1 cell per tick, avoids collisions and obstacles | A\* per robot (`navigation`), occupancy array, `ds::insertionSort` for robot order | O(cells + R·(A\* + R)) per tick |

## How it works (short)

**State machine** (see the diagram in `docs/ARCHITECTURE.md`): `IDLE → TO_SHELF → PICKING → (more shelves) → TO_SORTER → DROPPING → RETURNING → IDLE`, plus `TO_DOCK → CHARGING`, which pauses and later resumes a trip. Each state is one `case` in `PickingRobot::tick`. The robot never plans its own path: `goal()` says where it wants to be, and the TrafficController moves it.

**Traffic rules, each tick** (`TrafficController::step`):
1. Robots that want to move are sorted, longest-waiting first (`insertionSort`: ≤ 8 items, almost sorted already).
2. For each robot:
   - (a) plan with A\* if needed. A **new obstacle on the path** triggers a re-plan; that is the obstacle avoidance.
   - (b) if the next cell is empty, move.
   - (c) if a **parked** robot is in the way, nudge it aside and move.
   - (d) otherwise wait. After 3 waits, plan a detour that treats the other robots as walls. After 6 waits, if the blocker is *also* waiting (a jam), step aside to a free cell, preferably one not on the blocker's path.

**Why there are no collisions:** a robot only enters a cell that is empty *at that moment*, and the occupancy array updates after every single move. So two robots can never share a cell or swap places. `Simulation::checkInvariants` and the tests check this every tick.

**What is *not* guaranteed:** that every jam ends. Rule (d) is a heuristic. The stress tests (360 runs) never got stuck, but that is evidence, not proof. Say so in the report.

## Build and test only this module

```bash
cmake --build build --target test_picking
ctest --test-dir build -R picking --output-on-failure
```

## Validate it

### Hand-trace: two robots meeting head-on (test "two robots meeting head-on")

```
      col 012345678
row 1     WT.....TW      A starts at (1,1) and lives at (1,7); B starts at (1,7) and lives at (1,1)
row 2     WWWW.WWWW      the only passing place is the pocket (2,4)
```

Predict tick by tick what happens (rules 1–2d, `replanAfterWaits = 3`, `sidestepAfterWaits = 6`). Then print the positions in the test and compare.

<details><summary>What the program actually does (seed 2026)</summary>

- t1–t2: both drive towards each other.
- t3: A reaches (1,4) and B waits at (1,5).
- t3–t7: both wait. A detour is impossible in a 1-wide corridor.
- t8: B has waited 6 ticks and sidesteps. Its only free cell is (1,6), which lies on A's path, so it is a bad but forced choice.
- …more jams…
- t20: A sidesteps into the pocket (2,4) and B passes.
- t25: both are home.

The result is 25 ticks, 33 waits and 3 sidesteps for a 6-cell trip. **Exercise:** propose a better rule (for example, "the robot nearer a free side cell yields"), implement it, and show in this test and in E5 that it reduces waits.
</details>

### Break it on purpose

`python tools/mutation_test.py picking` plants three bugs:
- robots drive into occupied cells: caught (collisions);
- a robot keeps its cargo after delivering: caught;
- parked robots are never nudged: caught.

Add your own, for example "ignore obstacles on the remaining path" (rule a). Which test catches it?

### Questions you should be able to answer

- Walk through the state machine for a trip with 2 shelves where the battery runs low on the way to the sorter.
- Why does the robot store its route in a FIFO queue and not a stack?
- Explain the safety argument. Why can two robots never swap cells?
- Why is insertion sort a good choice for ordering the robots each tick? (Lecture 4: best case.)
- Why is the sidestep *random*? What could happen with a fixed rule when two robots are in mirror-image positions?

### Is this over-engineered?

- Four traffic rules plus nudging: are all of them needed? Remove one at a time and run the stress test (`test_sim`) to see which ones matter.
- The "technician swaps an empty battery after 30 ticks" rule only exists so a simulation can never freeze. Is it realistic? Is it needed if charging works?
- `PickingRobot` has many query functions (`stopsLeft`, `unitsOnTrip`, `shelvesLeft`, …), mostly for the UI. Which ones could go?

### Cross-check with a second GenAI tool

Prompt idea: *"Several robots move on a grid one step per time unit. Give simple rules that guarantee they never collide, and explain whether they can deadlock."* Compare with our rules. Does the other tool claim deadlock-freedom without proof? (Accuracy!)

## Modify it (starter tasks)

1. A better jam-breaking rule (see the hand-trace above), measured in E5.
2. Make the battery drain depend on the load: `drainPerMove × (1 + cargo kg / 12)`. Update the energy check in `fulfilment/JobManager`.
3. Add a "manual stop" key in the UI that freezes one robot, and observe the others re-routing around it.

## Experiment you own

- **E5** `experiments traffic`: 1…8 robots on the same 80 orders. Makespan, waits and congestion; speed-up vs the ideal. Explain why 8 robots give about 6.6× and not 8× (congestion, shared sorters, chargers).
