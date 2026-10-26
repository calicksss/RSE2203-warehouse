# core: shared vocabulary (WP-F, reviewed by everyone)

**Owner:** integrator (WP-F) · **Depends on:** nothing · **Used by:** every module except `ds`

Small headers that every module needs. **A change here affects all six work packages, so every member reviews it** (see `.github/CODEOWNERS`).

| File | What |
|------|------|
| `Types.hpp` | `Tick`, `CellIndex` (= row·cols + col), `GridPos`, sentinels such as `kNoCell` and `kUnreachable` |
| `Config.hpp/.cpp` | `SimConfig`: every tunable number (fleet size, capacities, battery, timings, obstacle rates, traffic thresholds), plus the `RoutingMethod` / `AssignmentMethod` switches used by the experiments |
| `Errors.hpp` | `DataError`: thrown for bad input files, with file and line number |
| `EventLog.hpp` | Rolling list of the last 400 events (`std::deque`: add at the back and drop at the front, both O(1)) |
| `Metrics.hpp` | Counters shown by the UI and used by the experiments (moves, waits, collisions, lead time, …) |
| `WorkItem.hpp` | `PickUnit` (one item to fetch) and `PickStop` (one shelf visit) |
| `Robot.hpp` | Abstract base class `Robot` (pure virtual `tick`, `stateLabel`, …); `FulfilmentListener` interface; `TickContext` |

## Validate it

- **Lecture 1b check:** find abstraction, encapsulation, inheritance and polymorphism in `Robot.hpp`, `PickingRobot` and `SortingRobot`. Where does a `Robot*` call the derived class's `tick()`?
- Why is `Robot`'s destructor `virtual`? What would go wrong without it? (`unique_ptr<PickingRobot>` is fine, but `delete` through a `Robot*` would not be.)
- Why are robots non-copyable (`= delete`)?
- **Over-engineering question:** is `FulfilmentListener` (an "Observer") worth it? What would the include graph look like if robots called `JobManager` directly? Draw it.
