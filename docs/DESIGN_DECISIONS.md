# Design decisions and review for over-engineering

This document records the rationale for the present design and the features deliberately excluded from it. It is to be updated whenever the team revises a decision, and it provides material for the *Method* and *Discussion* sections of the report. Citation keys in square brackets refer to the course materials listed in the root `README.md`; numbered references [1]–[5] are listed at the end of this document.

## 1. Simplification of the previous version

The previous version of the software (v3) employed algorithms drawn from research literature and postgraduate curricula. Although they performed well, they could not reasonably be validated, explained in the technical interviews, or modified by the team, which is inconsistent with the aims of the course. Lecture 1a states that "data structure and algorithms are meant to 'simplify' problems, not to complicate things" [L1a, slide 6]. The present version (v4) therefore replaces these algorithms with methods covered in the course.

| Problem | v3 (previous) | v4 (current) | Rationale |
|---------|---------------|--------------|-----------|
| Multi-robot motion | Priority Inheritance with Backtracking (PIBT) [1] and windowed cooperative space-time A\* with a reservation table [2] | A\* for each robot, combined with traffic rules: move only into empty cells, ask parked robots to move aside, detour after three waits, random side-step when two robots block each other | The research algorithms are difficult to validate at this level. A\* falls within the graph-algorithms topic [S]; the traffic rules are greedy decisions [L5b]. |
| Assignment of robots to trips | Hungarian algorithm, O(n³) optimal assignment [3] | Greedy selection of the nearest idle robot [L5b] | Outside the syllabus. Experiment E6 shows that the assignment method affects the distance travelled by only 0.4–3%. |
| Order of shelf visits within a trip | Held–Karp dynamic programming [4], plus nearest-neighbour with 2-opt | Exact backtracking with pruning, compared with a greedy nearest-neighbour heuristic [L5a], [L5b] | Produces the same optimal result for up to eight shelves, using recursion and backtracking as taught in the course. |
| Trip formation | Proximity-based "seed-and-extend" batching | Greedy first-fit in order of urgency [L5b] | Simpler; shelf order is already optimised by the route planner. |
| Data structures | Standard Template Library throughout | Purpose-built linked-list queue, binary heap, sorting algorithms and binary search where they are significant [L2], [L4], [L5a] | A data structures and algorithms project should demonstrate the construction and analysis of these structures. |
| One-way aisles in the map | Supported (`> < ^ v`) | Removed | Not used by the data; an unused feature adds cost without benefit. |
| In-application benchmark | Background thread with cancellation flags | Separate `experiments` program producing CSV files | Fewer threads in the interface; the CSV files feed directly into the report. |
| Planner strategy and factory classes | `MotionPlanner` interface with two implementations | A single `TrafficController` | Only one strategy is now required. |

**Measured cost of the simplification** (80 orders, eight robots, mean over seeds 1–10, `Warehouse --headless 80 --seed N`):

| | Time to complete all orders | Mean lead time | Distance travelled |
|---|---:|---:|---:|
| v3 (PIBT, Hungarian, Held–Karp, proximity batching) | 671 ticks | 324 ticks | 3,988 cells |
| v4 (A\*, traffic rules, greedy methods, backtracking) | 720 ticks (+7%) | 347 ticks (+7%) | 4,222 cells (+6%) |

The advanced algorithms therefore improved performance by approximately 6–7%, at the cost of roughly 1,000 additional lines of code that the team could not have explained or maintained. For a course project, v4 represents the more appropriate trade-off. This comparison may be discussed in the *Discussion* section of the report.

## 2. Open decisions (to be resolved by the team)

The following design choices may or may not constitute over-engineering. Each module README repeats the items relevant to that module.

| Location | Question | Considerations |
|----------|----------|----------------|
| `ds/LinkedQueue` | Should `std::queue` be used instead? | The project concerns data structures, and the implementation is tested against `std::queue`; the standard container would nevertheless be simpler. |
| `ds/Sorting`: bubble and selection sort | Used only in the experiments | Appropriate for E1; it is open whether they belong in the library. |
| `shop/Catalogue`: binary search | With 91 products, linear search is equally fast in practice | Binary search demonstrates [L5a] and scales (E2). The lower-case conversion in the comparator could be precomputed. |
| `shop/Money` | Integer cents instead of `double` | Prevents rounding errors (a test shows that summing 0.10 one thousand times in `double` does not give exactly 100.0). Recommended to retain. |
| `core/Robot` and `FulfilmentListener` | Abstract classes and the observer pattern | Avoids a circular dependency and demonstrates polymorphism [L1b], at the cost of one level of indirection. |
| `navigation/DistanceTable` | Caching every breadth-first search result | Avoids repeated searches (memoisation [L5a]); requires about 2 MB of memory. |
| `picking/TrafficController`: side-step rule | Random side-step when two robots block each other | Required by the corridor test; because the choice is random, runs are reproducible only for the same seed. |
| `fulfilment/JobManager` | A single large class (approximately 500 lines including comments) | Could be divided into Backlog, Dispatcher and ChargingManager classes. |
| `ui` | FTXUI interface of approximately 1,500 lines | Not assessed as part of the data structures and algorithms content, but beneficial for the demonstration video; could be reduced to a text dashboard. |

## 3. Known limitations (to be stated in the report)

1. **The traffic rules are not proven to resolve every congestion situation.** They succeeded in all stress tests (360 runs, none unfinished, no collisions), but a guarantee would require algorithms such as Conflict-Based Search [5] or PIBT [1]. *Safety* (absence of collisions) holds by construction; *liveness* (absence of permanent blockage) is only supported by testing.
2. **The route planner uses static distances.** It does not account for other robots or temporary obstacles; these are handled by A\* during motion.
3. **Express priority is a fixed head start** of 120 ticks rather than a deadline.
4. **Robots move one cell per tick.** Acceleration, turning time and load-dependent battery consumption are not modelled.
5. **Backtracking has exponential complexity.** Above eight shelves the planner falls back to the greedy method; `tripCapacityUnits = 8` keeps every trip within this limit.
6. **Quick sort degrades when many keys are equal** (E1, "few-unique" input), because the partition scheme of Lecture 4 [L4] places all keys equal to the pivot on one side. A three-way partition would remove this weakness.

## 4. Possible extensions (at most one per work package, if time permits)

- WP-A: three-way-partition quick sort and heap sort, compared in E1.
- WP-B: product recommendations ("customers also bought") based on a hash table of co-purchase counts.
- WP-C: Dijkstra's algorithm with varying floor costs (for example slow zones), compared with A\* in E3.
- WP-D: reservation of the next two cells to reduce waiting, measured in E5.
- WP-E: deadline-based scheduling of express orders (earliest deadline first).
- WP-F: CSV export of a live run and a replay mode.

## References

[1] K. Okumura, M. Machida, X. Défago and Y. Tamura, "Priority Inheritance with Backtracking for Iterative Multi-agent Path Finding," in *Proc. 28th Int. Joint Conf. Artificial Intelligence (IJCAI)*, 2019, pp. 535–542.

[2] D. Silver, "Cooperative Pathfinding," in *Proc. 1st AAAI Conf. Artificial Intelligence and Interactive Digital Entertainment (AIIDE)*, 2005, pp. 117–122.

[3] H. W. Kuhn, "The Hungarian method for the assignment problem," *Naval Research Logistics Quarterly*, vol. 2, no. 1–2, pp. 83–97, 1955.

[4] M. Held and R. M. Karp, "A dynamic programming approach to sequencing problems," *Journal of the Society for Industrial and Applied Mathematics*, vol. 10, no. 1, pp. 196–210, 1962.

[5] G. Sharon, R. Stern, A. Felner and N. R. Sturtevant, "Conflict-based search for optimal multi-agent pathfinding," *Artificial Intelligence*, vol. 219, pp. 40–66, 2015.
