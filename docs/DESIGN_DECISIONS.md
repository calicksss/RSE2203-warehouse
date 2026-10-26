# Design decisions and the over-engineering review

This file records **why** the code looks the way it does, and what was deliberately left out. Add to it whenever your team changes a decision; it becomes the "Method" and "Discussion" material for the report.

## 1. The previous version was over-engineered for this course

The earlier version (v3) used algorithms from research papers and graduate courses. They worked well, but nobody on the team could reasonably validate them, defend them in an interview, or modify them. The brief and Lecture 1a ask for the opposite: *"Data structure and algorithms are meant to 'simplify' problems, not to complicate things!"*

| Problem | v3 (previous) | v4 (this version) | Why the change |
|---------|---------------|-------------------|----------------|
| Multi-robot movement | PIBT (Okumura 2019) and windowed cooperative space-time A* with a reservation table | A\* per robot plus traffic rules: move only into empty cells, nudge parked robots, detour after 3 waits, random sidestep in a mutual jam | Research algorithms; hard to validate. A\* and greedy rules are in scope (graphs, wk 10–11; greedy, L5b). |
| Robot ↔ trip assignment | Hungarian algorithm (O(n³) optimal matching) | Greedy: the nearest idle robot | Not in the syllabus. E6 shows the robot-choice method matters little (0.4–3% distance). |
| Visiting order on a trip | Held-Karp dynamic programming plus nearest-neighbour + 2-opt | Backtracking with pruning (exact, L5b) vs greedy nearest-neighbour (L5b) | Same optimal answer for ≤ 8 shelves, using a course technique (recursion + backtracking). |
| Forming trips | "Seed-and-extend" proximity batching | Greedy first-fit in urgency order | Simpler; the order of shelves is already optimised by the route planner. |
| Data structures | Everything from the STL | Our own linked-list queue, binary heap, sorting algorithms and binary search where they matter | A DS&A project should show we can build and analyse them (L2, L4, L5a). |
| One-way aisles in the map | Supported (`> < ^ v`) | Removed | The data never used them. Unused features are a cost with no benefit. |
| In-app benchmark tab | Background thread, cancellation flags | Separate `experiments` program writing CSVs | Fewer threads in the UI; the CSVs feed the report directly. |
| Strategy/Factory classes for planners | `MotionPlanner` interface with two implementations | One `TrafficController` | Only one strategy is needed now. |

**Measured cost of simplifying** (80 orders, 8 robots, average of seeds 1–10, `Warehouse --headless 80 --seed N`):

| | Time to finish all orders | Mean lead time | Distance driven |
|---|---:|---:|---:|
| v3 (PIBT + Hungarian + Held-Karp + proximity batching) | 671 ticks | 324 ticks | 3,988 cells |
| v4 (A\* + traffic rules + greedy + backtracking) | 720 ticks (+7%) | 347 ticks (+7%) | 4,222 cells (+6%) |

So the advanced algorithms bought about 6–7% performance, at the price of roughly 1,000 extra lines that the team could not have explained. For a course project, v4 is the better trade-off. This comparison is a good "insight" point for the report.

## 2. Decisions that are still open (decide as a team)

These are honest "maybe over-engineered, maybe not" spots. Each module README repeats the ones for that module.

| Where | Question | Arguments |
|-------|----------|-----------|
| `ds/LinkedQueue` | Why not `std::queue`? | The project is about data structures; ours is tested against `std::queue`. Using STL would be simpler, though. |
| `ds/Sorting` bubble / selection | Only used in experiments | Fine for E1. Should they stay in the library at all? |
| `shop/Catalogue` binary search | With only 91 products, linear search is just as fast in practice | Binary search demonstrates L5a and scales (E2). The lower-case conversion in the comparator could be precomputed. |
| `shop/Money` | Integer cents instead of `double` | Prevents rounding errors (a test shows 0.10 × 1000 ≠ 100.0 in `double`). Worth keeping. |
| `core/Robot` + `FulfilmentListener` | Abstract classes and the observer pattern | Avoids a circular dependency and shows L1b polymorphism; adds one level of indirection. |
| `navigation/DistanceTable` | Caching all BFS results | Saves repeated BFS runs (memoisation); about 2 MB of memory. |
| `picking/TrafficController` sidestep rule | Random sidestep when two robots block each other | Needed for the corridor test; random means runs are only repeatable with the same seed. |
| `fulfilment/JobManager` | One large class (≈ 500 lines with comments) | Could be split into Backlog / Dispatcher / ChargingManager. |
| `ui` | FTXUI library, ≈ 1,500 lines | Not part of the DSA marks, but the demo video benefits. Could be reduced to a text dashboard. |

## 3. Known limitations (be upfront about these in the report)

1. **The traffic rules have no proof that every jam ends.** They worked in all stress runs (360 runs, 0 unfinished, 0 collisions), but a proof would need algorithms such as CBS or PIBT. *Safety* (no collisions) is guaranteed by construction; *liveness* (no permanent jams) is only tested.
2. **The route planner uses static distances.** It ignores other robots and temporary obstacles; A\* handles those while driving.
3. **Express priority is a fixed head start** (120 ticks), not a deadline.
4. **Robots move one cell per tick.** There is no acceleration, turning time or battery model beyond "per move".
5. **Backtracking is exponential.** Above 8 shelves it falls back to greedy; `tripCapacityUnits = 8` keeps trips at or below this limit.
6. **Quick sort degrades with many equal keys** (E1, "few-unique"): this partition scheme puts all equal keys on one side. A 3-way partition would fix it.

## 4. Ideas for extensions (if time allows; pick at most one per WP)

- WP-A: 3-way partition quick sort; heap sort; compare with E1.
- WP-B: product recommendations ("customers also bought"), using a hash map of co-purchase counts.
- WP-C: Dijkstra with different floor costs (e.g. slow zones); compare with A\* in E3.
- WP-D: reservation of the *next two* cells to reduce waiting; measure in E5.
- WP-E: deadline-based scheduling for express orders (earliest-deadline-first).
- WP-F: CSV export of a live run; replay mode.
