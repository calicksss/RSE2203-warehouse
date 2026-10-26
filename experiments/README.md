# Experiments

One program, six experiments, one owner each. They support the report's "Experiment, analysis and insight discussion" (20%).

```bash
./build/experiments/experiments all            # or: sorting | search | pathfinding | routing | traffic | policies
python experiments/plot_results.py             # needs: pip install matplotlib
```

Results go to `experiments/results/*.csv` and the charts to `experiments/results/plots/*.png`. The files in the repository are a **reference run**: re-run on your own machine before quoting numbers. Counts (comparisons, cells, moves, ticks) repeat exactly for the same seed and compiler. Visual Studio's standard library turns a seed into different random numbers than GCC/Clang/MinGW, so its warehouse numbers differ slightly (the conclusions do not). Times in ms or µs depend on the computer (Lecture 3: limits of empirical analysis).

| # | Owner | Question | Setup | Reference result |
|---|-------|----------|-------|------------------|
| E1 | WP-A | Do the sorting algorithms grow as the theory says? | 6 algorithms × 4 input types × n = 100 … 200,000; comparisons and median time of 3 | n² for bubble, selection and insertion; n log n for merge and quick. Insertion on sorted input: n−1 comparisons. Quick sort on "few-unique" input: about 94 × n log n comparisons |
| E2 | WP-B | How much does binary search save? | Sorted arrays n = 91 … 10⁶, 2,000 lookups | Max comparisons = ⌊log₂ n⌋ + 1 (20 for 10⁶) vs about n/2 for linear search |
| E3 | WP-C | How much work does A\*'s heuristic save? | 1,000 random start/goal pairs on the real map, 0 or 20 obstacles | Identical path lengths; A\* expands about 40 cells vs BFS about 250 (16%) |
| E4 | WP-C | How far from optimal is greedy routing, and what does exactness cost? | k = 1 … 10 shelves, 100 random trips each | Greedy is 6–26% longer than optimal for k ≥ 2; pruning: about 7.4 thousand vs about 2 million nodes at k = 7 |
| E5 | WP-D | Do more robots help linearly? | 80 orders, 1 … 8 robots, 10 seeds | 8 robots give a 6.6× speed-up (not 8×); waits rise with fleet size |
| E6 | WP-E | Which decision matters most? | 80 orders, 8 robots, 3 routing × 2 robot-choice methods, 10 seeds | Backtracking + nearest: −25% distance vs the baseline; robot choice alone: −0.4% to −3% |

## Writing about an experiment (template for each report subsection)

1. **Question and hypothesis.** What the theory predicts (Big-O, optimal vs greedy).
2. **Setup.** Inputs, sizes, seeds, what is measured. Say that times are machine-dependent.
3. **Result.** One chart or table, described in one or two sentences.
4. **Analysis.** Does it match the theory? Explain every surprise. Examples: bubble = selection in comparisons; quick sort and duplicates; speed-up below linear.
5. **Insight.** What it means for the warehouse. For example, routing quality matters more than robot choice, so spend effort there.

## Adding your own experiment

Add a function `experimentX()` in `experiments.cpp`, call it from `main`, write a CSV with `openCsv(...)`, and add a plot function in `plot_results.py`. Keep fixed seeds so results are reproducible.
