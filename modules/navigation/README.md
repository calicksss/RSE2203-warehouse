# navigation: map, BFS, A*, distances and route planning (WP-C)

**Owner:** _name_ · **Depends on:** core, ds · **Used by:** picking, fulfilment, sim, ui

Everything about *where* things are and *how far* apart they are: the floor plan, shortest paths, the order in which a robot visits its shelves, and temporary obstacles.

| File | What | Algorithm / data structure | Big-O |
|------|------|----------------------------|-------|
| `GridMap.hpp/.cpp` | Load and validate `Map.txt`; neighbours of a cell | 2D grid flattened into a 1D `vector` (`cell = row*cols + col`); BFS reachability check | parse O(cells) |
| `Pathfinding.hpp/.cpp` | `bfsDistances`, `bfsPath`, `aStarPath` | **BFS** with `ds::LinkedQueue`; **A\*** with `ds::MinHeap`, Manhattan heuristic | BFS O(V+E); A\* O(E log V) worst case |
| `DistanceTable.hpp/.cpp` | Distance between any two cells, cached | one BFS per source cell, kept for reuse (**memoisation**) | O(V+E) first time, O(1) after |
| `RoutePlanner.hpp/.cpp` | Order of shelves on a trip | **as given** (baseline), **greedy** nearest-neighbour, recursive **backtracking with pruning** | O(k), O(k²), O(k!·2ᵏ) worst |
| `ObstacleManager.hpp/.cpp` | Random temporary obstacles | `vector<char>` mask, one entry per cell | O(cells) per spawn |

## How it works (short)

- **Graph view.** Walkable cells (floor, docks) are vertices; up/down/left/right neighbours are edges of cost 1. Shelves, sorters and walls are not vertices.
- **BFS** visits cells in rings of distance 0, 1, 2, … using a FIFO queue, so the first time it reaches a cell is via a shortest path (every edge costs the same).
- **A\*** always expands the open cell with the smallest `f = g + h`. Here `g` is the number of moves so far and `h = |Δrow| + |Δcol|`. On a 4-connected grid `h` never over-estimates (*admissible*) and changes by at most 1 per move (*consistent*). So the first time the goal leaves the heap, its path is shortest. The heap may hold old copies of a cell, which are skipped ("lazy deletion").
- **Route planning** (a small travelling-salesman problem). Start at the robot, visit k shelves (each from one of its sides), end at the sorter.
  - *Greedy* (L5b) goes to the closest unvisited shelf each time.
  - *Backtracking* (L5b) tries every order depth-first: choose, recurse, undo. It starts with the greedy answer as the "best so far" and prunes a branch when `length so far + distance(here, end) ≥ best`. That bound is safe because the rest of the route can never be shorter than going straight to the end (triangle inequality).

## Build and test only this module

```bash
cmake --build build --target test_navigation
ctest --test-dir build -R navigation --output-on-failure
```

## Validate it

### Hand-trace 1: BFS on the tiny map (used by the tests)

```
      col 0123456
row 0     WWWWWWW
row 1     WT...CW      T = start (1,1), C = dock (1,5)
row 2     W.WSW.W      S = shelf (2,3)
row 3     W.....W
row 4     WR....W      R = sorter
row 5     WWWWWWW
```

Write the BFS distance from T into every floor cell. Then check against the test "BFS: distances on the tiny map". For example, (3,3) = 4 and (4,5) = 7. Next, block cell (1,3): what is the new distance to the dock, and which path does A\* take? (Answer: 8, along row 3.)

### Hand-trace 2: backtracking with pruning (paper only)

Start S, end E, shelves A, B and C. The distances (they obey the triangle inequality, like real grid distances) are:

|   | A | B | C | E |
|---|---|---|---|---|
| **S** | 2 | 3 | 4 | 3 |
| **A** | – | 2 | 4 | 1 |
| **B** | 2 | – | 2 | 3 |
| **C** | 4 | 2 | – | 5 |

a) What route does greedy choose, and how long is it? b) Draw the backtracking tree, starting with best = the greedy length. Mark every branch the bound `length + d(here, E) ≥ best` cuts off. c) What is the optimal route?

<details><summary>Answer</summary>

a) Greedy: S→A (2) → B (2) → C (2) → E (5) = **11**.

b) With best = 11:
- **S→A (2):**
  - A→B (4): then C, 6 + 5 = 11 ≥ 11, cut.
  - A→C: 6 + 5 = 11, cut.
- **S→B (3):**
  - B→A (5): then C, 9 + 5 = 14, cut.
  - B→C (5): then A, 9 + 1 = 10 < 11 → leaf at 10 → **best = 10**.
- **S→C (4):**
  - C→A (8): then B, 10 + 3 = 13, cut.
  - C→B (6): then A, 8 + 1 = 9 → leaf at 9 → **best = 9**.

c) **S→C→B→A→E = 9.** Greedy was 22% longer, because it ignored where the trip ends.
</details>

### Break it on purpose

`python tools/mutation_test.py navigation` plants four bugs:
- a heuristic 3× too big (not admissible): caught, because path lengths no longer match BFS;
- backtracking that forgets to undo `visited`: caught by the brute-force comparison;
- `neighbours()` that ignores walls: caught;
- a weaker pruning bound: the answers stay correct, so only the "far fewer nodes" test notices. Discuss what that says about testing *performance* claims.

### Questions you should be able to answer

- Why does BFS give shortest paths here, but would not if some moves cost 2? What would you use then?
- Prove (informally) that Manhattan distance never over-estimates on this grid. Would it still be admissible with diagonal moves?
- Why is `DistanceTable::distance` only valid for walkable cells? (Look at the comment: what breaks the symmetry?)
- Why is the backtracking result exact even though it prunes? Why is greedy not?
- How many leaves does the full search tree have for k shelves with 2 sides each? (k!·2ᵏ, see E4.)

### Is this over-engineered?

- Do we need *both* BFS and A\*? BFS gives distances to *all* cells at once (needed for the distance table); A\* is faster for *one* goal (driving). Agree or disagree?
- `DistanceTable` can hold up to 693 × 693 ints (≈ 2 MB). Is caching worth it? Count BFS runs with and without the cache.
- The route planner also chooses the *side* of each shelf, which doubles the branching. Is that worth it? Measure the change in route length.

### Cross-check with a second GenAI tool

Prompt idea: *"Implement A\* on a 4-connected grid in C++ with a binary heap. Explain why the first time the goal is popped the path is optimal."* Check the explanation: does it mention *consistency* and stale heap entries? Run its code against `bfsPath` on random pairs, as our test does.

## Modify it (starter tasks)

1. **Dijkstra with costs.** Give some floor cells cost 3 ("slow zone", new map symbol) and make A\* use costs. BFS is then wrong; show that with a test.
2. **2-opt improvement.** After greedy, repeatedly reverse a segment of the route if that makes it shorter. Compare with greedy and backtracking in E4.
3. **Tie-breaking experiment.** Change `OpenEntry::operator<` to prefer *larger* h on equal f and measure the cells expanded in E3.

## Experiments you own

- **E3** `experiments pathfinding`: cells expanded and time, BFS vs A\*, with and without obstacles (A\* does about 16% of BFS's work).
- **E4** `experiments routing`: as-given vs greedy vs exact route lengths for k = 1…10 shelves; search-tree nodes with and without pruning. Discuss exponential growth (Lecture 3, 5b) and why we cap exact search at 8 shelves.
