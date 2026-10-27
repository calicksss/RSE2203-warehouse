# Experiments

A single program provides six experiments, each owned by one work package. Together they support the assessment component "experiment, analysis and insight discussion", which carries 20% of the team-project mark [B]. Citation keys refer to the *References* section of the root `README.md`.

```bash
./build/experiments/experiments all            # or: sorting | search | pathfinding | routing | traffic | policies
python experiments/plot_results.py             # requires: pip install matplotlib
```

Results are written to `experiments/results/*.csv` and charts to `experiments/results/plots/*.png`. The files in the repository are a **reference run**; each team should repeat the experiments on its own machines before quoting any figures. Counts (comparisons, cells, moves, ticks) are reproducible for a given seed and compiler. The Visual Studio standard library derives different random numbers from the same seed than GCC, Clang or MinGW do, so its warehouse figures differ slightly, although the conclusions are unaffected. Execution times depend on the machine, which is one of the limitations of empirical analysis noted in Lecture 3 [L3].

| # | Owner | Research question | Set-up | Reference result |
|---|-------|-------------------|--------|------------------|
| E1 | WP-A | Do the sorting algorithms grow as predicted by theory [L3], [L4]? | 6 algorithms × 4 input types × n = 100 to 200,000; comparisons and the median time of three runs | n² growth for bubble, selection and insertion sort; n log n for merge and quick sort. Insertion sort on sorted input: n − 1 comparisons. Quick sort on "few-unique" input: about 94 × n log n comparisons |
| E2 | WP-B | How much work does binary search save [L5a]? | Sorted arrays with n = 91 to 10⁶; 2,000 look-ups | Maximum comparisons = ⌊log₂ n⌋ + 1 (20 for 10⁶), compared with about n/2 for linear search |
| E3 | WP-C | How much work does the A\* heuristic save [S]? | 1,000 random start–goal pairs on the warehouse map, with 0 or 20 obstacles | Identical path lengths; A\* expands about 40 cells compared with about 250 for breadth-first search (16%) |
| E4 | WP-C | How far from optimal is greedy routing, and what is the cost of an exact search [L5b]? | k = 1 to 10 shelves, 100 random trips each | Greedy routes are 6–26% longer than optimal for k ≥ 2; pruning reduces the search to about 7.4 thousand nodes, compared with about 2 million, at k = 7 |
| E5 | WP-D | Does throughput increase linearly with the number of robots? | 80 orders, one to eight robots, ten seeds | Eight robots achieve a speed-up of 6.6 rather than 8; waiting increases with fleet size |
| E6 | WP-E | Which scheduling decision has the greatest effect? | 80 orders, eight robots, 3 routing × 2 robot-selection methods, ten seeds | Backtracking with nearest-robot selection: 25% less distance than the baseline; robot selection alone: 0.4–3% less |

## Recommended structure for each experiment in the report

1. **Research question and hypothesis.** The behaviour predicted by theory (time complexity; optimal versus greedy methods).
2. **Experimental set-up.** Inputs, sizes, seeds and the quantities measured, noting that execution times depend on the machine.
3. **Results.** One chart or table, described in one or two sentences.
4. **Analysis.** Agreement with theory and an explanation of every unexpected result, for example the equal comparison counts of bubble and selection sort, the behaviour of quick sort with duplicate keys, or a sub-linear speed-up.
5. **Insight.** The implication for the warehouse; for example, routing quality matters more than robot selection, so further effort is best directed at routing.

## Adding an experiment

Add a function `experimentX()` to `experiments.cpp`, call it from `main`, write a CSV file with `openCsv(...)`, and add a corresponding plotting function to `plot_results.py`. Fixed seeds must be used so that the results are reproducible.
