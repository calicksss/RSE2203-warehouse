# sim: the simulation loop (WP-F)

**Owner:** integrator (WP-F) · **Depends on:** all model modules · **Used by:** ui, app, experiments

`Simulation` builds the warehouse from the two data files and advances it one tick at a time. It owns every robot (`std::unique_ptr`), gives the other classes plain pointers, and checks the safety rules after every tick.

```
step():  obstacles.update → jobs.update → traffic.step → robot->tick() for every robot → checkInvariants
```

`checkInvariants` counts a **collision** if two pickers share a cell, if a picker is on a wall, shelf or obstacle, or if two pickers swapped cells. The count must stay 0, and every system test checks it.

## Tests (`tests/test_sim.cpp`)

| Test | What it proves |
|------|----------------|
| 80 orders + heavy obstacles | Everything completes, 0 collisions, detours actually happen |
| every picker works | Robots work in parallel (all 8 busy at some point) |
| express faster on average | The priority rule has a visible effect |
| fast battery drain | Charging and resuming trips works under stress |
| same seed, same run | Experiments are reproducible |
| stress (48 runs) | All routing methods, 1–8 robots and obstacles: always finishes, 0 collisions |
| too-heavy product | Bad data is rejected at start-up |

## Validate it

- "Who tests the tester?" If `checkInvariants` were broken, the system tests would still report 0 collisions. Which *other* tests would still catch a collision bug? (Hint: `test_picking` checks positions itself. Try the mutant "robots drive into occupied cells" with `checkInvariants` emptied.)
- Change the order inside `step()`, for example robots tick *before* traffic moves them. Which tests fail, and is the new order actually wrong or just different?
- The stress test finishes in under a second. Is 48 runs enough evidence? How many runs did `docs/DESIGN_DECISIONS.md` cite, and how would you run more? (Change the loops; do not commit a 5-minute test.)
