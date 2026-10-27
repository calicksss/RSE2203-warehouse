# sim: simulation loop (WP-F)

**Owner:** integrator (WP-F) · **Depends on:** all model modules · **Used by:** ui, app, experiments

`Simulation` constructs the warehouse from the two data files and advances it one tick at a time. It owns every robot through `std::unique_ptr`, provides the other classes with non-owning pointers, and verifies the safety properties after every tick. Citation keys refer to the *References* section of the root `README.md`.

```
step():  obstacles.update → jobs.update → traffic.step → robot->tick() for every robot → checkInvariants
```

`checkInvariants` records a **collision** if two picking robots occupy the same cell, if a robot stands on a wall, shelf or obstacle, or if two robots exchange cells within one tick. The count must remain zero, and every system test checks this.

## Tests (`tests/test_sim.cpp`)

| Test | Property established |
|------|----------------------|
| 80 orders with frequent obstacles | All orders complete with no collisions, and detours do occur |
| every picker works | Robots operate in parallel (all eight are busy at some point) |
| express faster on average | The priority rule has a measurable effect |
| fast battery drain | Charging and the resumption of suspended trips work under stress |
| same seed, same run | Runs are reproducible, as the experiments require |
| stress test (48 runs) | Every routing method, one to eight robots and obstacles: all runs complete without collisions |
| over-weight product | Invalid data is rejected at start-up |

## Validation

- **Verification of the verifier.** If `checkInvariants` were defective, the system tests would still report zero collisions. Which other tests would still detect a collision defect? (`test_picking` checks robot positions independently; try the mutant "robots drive into occupied cells" with `checkInvariants` disabled.)
- Change the order of operations in `step()`, for example by letting robots tick before the traffic controller moves them. Which tests fail, and is the new order incorrect or merely different?
- The stress test runs in under one second. Are 48 runs sufficient evidence? `docs/DESIGN_DECISIONS.md` cites 360 runs; how would more be performed without adding a slow test to the repository?
