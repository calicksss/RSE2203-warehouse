# core: shared definitions (WP-F, reviewed by all members)

**Owner:** integrator (WP-F) · **Depends on:** none · **Used by:** every module except `ds`

This module contains small headers required by every other module. **Because a change here affects all six work packages, every member reviews it** (see `.github/CODEOWNERS`). Citation keys refer to the *References* section of the root `README.md`.

| File | Content |
|------|---------|
| `Types.hpp` | `Tick`, `CellIndex` (= row · cols + col), `GridPos`, and sentinel values such as `kNoCell` and `kUnreachable` |
| `Config.hpp/.cpp` | `SimConfig`: every adjustable parameter (fleet size, capacities, battery, timings, obstacle rates, traffic thresholds), together with the `RoutingMethod` and `AssignmentMethod` options used by the experiments |
| `Errors.hpp` | `DataError`, raised for invalid input files and reporting the file name and line number |
| `EventLog.hpp` | Rolling record of the 400 most recent events, stored in a `std::deque` (insertion at the back and removal from the front are both O(1)) |
| `Metrics.hpp` | Counters displayed by the interface and used by the experiments (moves, waits, collisions, lead time, …) |
| `WorkItem.hpp` | `PickUnit` (one item to be collected) and `PickStop` (one shelf visit) |
| `Robot.hpp` | Abstract base class `Robot` (pure virtual `tick`, `stateLabel`, …), the `FulfilmentListener` interface and `TickContext` |

## Validation

- **Object-oriented concepts [L1b].** Identify abstraction, encapsulation, inheritance and polymorphism in `Robot.hpp`, `PickingRobot` and `SortingRobot`. Where does a call through a `Robot*` invoke the derived class's `tick()`?
- Why is the destructor of `Robot` declared `virtual`? What would go wrong without it? (Destruction through `unique_ptr<PickingRobot>` is unaffected, but `delete` through a `Robot*` would not be.)
- Why are robots made non-copyable (`= delete`)?
- **Review for over-engineering.** Is `FulfilmentListener` (an application of the observer pattern) justified? Draw the include graph that would result if robots called `JobManager` directly.
