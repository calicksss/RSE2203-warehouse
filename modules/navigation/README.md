# navigation: map, breadth-first search, A\*, distances and route planning (WP-C)

**Owner:** _name_ · **Depends on:** core, ds · **Used by:** picking, fulfilment, sim, ui

This module covers the spatial aspects of the warehouse: the floor plan, shortest paths, the order in which a robot visits its shelves, and temporary obstacles. Citation keys refer to the *References* section of the root `README.md`.

| File | Content | Algorithm / data structure | Complexity |
|------|---------|----------------------------|------------|
| `GridMap.hpp/.cpp` | Loading and validation of `Map.txt`; neighbours of a cell | Two-dimensional grid stored as a one-dimensional `vector` (`cell = row·cols + col`) [L2]; reachability check by breadth-first search | parsing O(cells) |
| `Pathfinding.hpp/.cpp` | `bfsDistances`, `bfsPath`, `aStarPath` | **Breadth-first search** with `ds::LinkedQueue`; **A\*** with `ds::MinHeap` and the Manhattan heuristic [S] | BFS O(V + E); A\* O(E log V) in the worst case |
| `DistanceTable.hpp/.cpp` | Cached distance between any two cells | One breadth-first search per source cell, stored for reuse (**memoisation** [L5a]) | O(V + E) on first use, O(1) thereafter |
| `RoutePlanner.hpp/.cpp` | Order of shelf visits within a trip | **as given** (baseline), **greedy** nearest neighbour, recursive **backtracking with pruning** [L5a], [L5b] | O(k), O(k²) and O(k!·2ᵏ) in the worst case |
| `ObstacleManager.hpp/.cpp` | Random temporary obstacles | `vector<char>` mask with one entry per cell | O(cells) per obstacle created |

## How it works

- **Graph model.** Walkable cells (floor and docks) are vertices; cells adjacent horizontally or vertically are joined by edges of cost 1. Shelves, sorters and walls are not vertices.
- **Breadth-first search** visits cells in order of increasing distance (0, 1, 2, …) using a first-in-first-out queue. Because every edge has the same cost, a cell is first reached along a shortest path.
- **A\*** always expands the open cell with the smallest `f = g + h`, where `g` is the number of moves so far and `h = |Δrow| + |Δcol|`. On a four-connected grid, `h` never overestimates the remaining distance (it is *admissible*) and changes by at most 1 per move (it is *consistent*). Consequently, the first time the goal is removed from the heap, its path is shortest. Outdated copies of a cell may remain in the heap and are skipped (lazy deletion).
- **Route planning** is a small instance of the travelling-salesman problem: start at the robot, visit k shelves (each from one of its accessible sides) and finish at the sorter.
  - The *greedy* method [L5b] always proceeds to the nearest unvisited shelf.
  - The *backtracking* method [L5b] explores every order depth-first by choosing, recursing and undoing each choice. It is initialised with the greedy result as the best route found so far and abandons a branch when `length so far + distance(current, end) ≥ best`. This bound is safe because, by the triangle inequality, the remainder of any route cannot be shorter than the direct distance to the end.

## Building and testing this module only

```bash
cmake --build build --target test_navigation
ctest --test-dir build -R navigation --output-on-failure
```

## Validation

### Hand trace 1: breadth-first search on the small test map

```
      col 0123456
row 0     WWWWWWW
row 1     WT...CW      T = start (1,1), C = dock (1,5)
row 2     W.WSW.W      S = shelf (2,3)
row 3     W.....W
row 4     WR....W      R = sorter
row 5     WWWWWWW
```

Write the breadth-first-search distance from T in every floor cell and compare the result with the test "BFS: distances on the tiny map"; for example, (3,3) = 4 and (4,5) = 7. Then block cell (1,3). What is the new distance to the dock, and which path does A\* take? (Answer: 8, along row 3.)

### Hand trace 2: backtracking with pruning (on paper)

The route starts at S, ends at E and visits shelves A, B and C. The distances below satisfy the triangle inequality, as grid distances do:

|   | A | B | C | E |
|---|---|---|---|---|
| **S** | 2 | 3 | 4 | 3 |
| **A** | – | 2 | 4 | 1 |
| **B** | 2 | – | 2 | 3 |
| **C** | 4 | 2 | – | 5 |

(a) Which route does the greedy method choose, and what is its length? (b) Draw the backtracking tree, starting with the greedy length as the best value, and mark every branch removed by the bound `length + d(current, E) ≥ best`. (c) What is the optimal route?

<details><summary>Answer</summary>

(a) Greedy: S→A (2) → B (2) → C (2) → E (5) = **11**.

(b) With best = 11:
- **S→A (2):**
  - A→B (4): then C gives 6 + 5 = 11 ≥ 11, pruned.
  - A→C gives 6 + 5 = 11, pruned.
- **S→B (3):**
  - B→A (5): then C gives 9 + 5 = 14, pruned.
  - B→C (5): then A gives 9 + 1 = 10 < 11, a complete route of length 10, so **best = 10**.
- **S→C (4):**
  - C→A (8): then B gives 10 + 3 = 13, pruned.
  - C→B (6): then A gives 8 + 1 = 9, a complete route of length 9, so **best = 9**.

(c) **S→C→B→A→E, of length 9.** The greedy route is 22% longer because it does not consider where the trip ends.
</details>

### Mutation testing

`python tools/mutation_test.py navigation` introduces four defects:
- a heuristic three times too large (not admissible): detected, because path lengths no longer match breadth-first search;
- backtracking that does not undo the `visited` mark: detected by the comparison with exhaustive search;
- `neighbours()` that ignores walls: detected;
- a weaker pruning bound: results remain correct, so the defect is detected only by the test asserting far fewer search nodes. The report should discuss what this implies for the testing of *performance* claims.

### Review questions

- Why does breadth-first search produce shortest paths here, and why would it not if some moves cost 2? Which algorithm would then be appropriate?
- Show informally that the Manhattan distance never overestimates on this grid. Would it remain admissible if diagonal moves were allowed?
- Why is `DistanceTable::distance` valid only for walkable cells? (Refer to the comment: what would break the symmetry?)
- Why is the backtracking result exact despite pruning, and why is the greedy result not [L5b]?
- How many leaves does the complete search tree have for k shelves with two sides each? (k!·2ᵏ; see E4.)

### Review for over-engineering

- Are *both* breadth-first search and A\* required? Breadth-first search provides distances to *all* cells at once (needed for the distance table), whereas A\* is faster for a *single* goal (robot motion). The team should state whether it agrees.
- `DistanceTable` can hold up to 693 × 693 integers (about 2 MB). Is caching worthwhile? Count the searches performed with and without the cache.
- The route planner also selects the *side* from which each shelf is approached, which doubles the branching factor. Is this justified? Measure the effect on route length.

### Cross-check with a second generative AI tool

Suggested prompt: *"Implement A\* on a 4-connected grid in C++ with a binary heap. Explain why the first time the goal is popped the path is optimal."* Assess whether the explanation addresses *consistency* and outdated heap entries, and test the generated code against `bfsPath` on random pairs, as the existing test does.

## Suggested modifications

1. **Dijkstra's algorithm with costs.** Assign cost 3 to some floor cells (a "slow zone" with a new map symbol) and make A\* use these costs. Show with a test that breadth-first search then gives incorrect results.
2. **2-opt improvement.** After the greedy construction, repeatedly reverse a segment of the route whenever this shortens it, and compare the result with the greedy and backtracking methods in E4.
3. **Tie-breaking.** Change `OpenEntry::operator<` to prefer the *larger* h when f values are equal, and measure the number of cells expanded in E3.

## Experiments owned by this work package

- **E3** `experiments pathfinding`: cells expanded and execution time for breadth-first search and A\*, with and without obstacles. A\* performs about 16% of the work of breadth-first search.
- **E4** `experiments routing`: route lengths for the as-given, greedy and exact methods for k = 1 to 10 shelves, and the number of search-tree nodes with and without pruning. The discussion should address exponential growth [L3], [L5b] and justify limiting the exact search to eight shelves.
