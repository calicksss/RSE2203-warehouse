# Generative AI usage and evaluation log

The project brief [B] requires the use of at least two generative AI tools to assist in generating basic source code, and an evaluation of their responses against four criteria: **accuracy, completeness, clarity and relevance**. Every substantial use of such a tool is recorded in this log, which forms the basis of the generative AI section of the report.

**Recording procedure.** One entry is made per question or task. The prompt is summarised briefly or linked. Scores follow the rubric in `VALIDATION_GUIDE.md`, and **each score must be supported by evidence** such as a test name, a line number or a counter-example. Citation keys refer to the *References* section of the root `README.md`.

---

## Entry template

```
### G<number>: <short title>
- Date / member / work package:
- Tool and version:          (e.g. ChatGPT, GitHub Copilot, Claude, Gemini)
- Prompt (summary or link):
- Output produced:
- Verification performed:    (tests run, hand trace, comparison with another tool, mutation test)
- Problems identified:
- Changes made:
- Scores:  Accuracy _/5 · Completeness _/5 · Clarity _/5 · Relevance _/5
- Evidence for the scores:
```

---

## G1: Initial version of the code base (baseline; to be scored by the team)

- **Date / member / work package:** 26 October 2026 / team leader / all
- **Tool:** Claude (Anthropic), used as an agent that wrote and executed code.
- **Prompt (summary):** "Use the current iteration of the warehouse program and cater it to our course level. Teammates must be able to understand, validate and modify it. Split it into separate parts for six members on GitHub."
- **Output produced:**
  - eight modules with tests (74 test cases in six test programs);
  - an experiments program and a plotting script;
  - documentation;
  - the GitHub configuration (continuous integration, code owners, templates).
- **Verification reported by the tool:**
  - the code compiles with warnings treated as errors;
  - all module tests pass;
  - a stress test of 360 simulation runs completed every order with no collisions;
  - A\* path lengths match breadth-first search;
  - backtracking routes match an exhaustive search;
  - all 20 planted defects were either detected or explained as equivalent.

  **These are claims made by the tool itself.** They are to be reproduced by the team (`ctest`, `tools/mutation_test.py`, `experiments`) before being relied upon.
- **Limitations stated by the tool:** see `DESIGN_DECISIONS.md` §3. In particular, the traffic rules are a heuristic with no proof that every congestion situation resolves, and the route planner does not account for other robots.
- **Verification performed by the team:** _(each owner records the outcome of steps 1–7 for their module)_
- **Problems identified:** _(to be completed)_
- **Changes made:** _(to be completed)_
- **Scores:** Accuracy _/5 · Completeness _/5 · Clarity _/5 · Relevance _/5
- **Evidence:** _(to be completed)_

## G2: Cross-check with a second tool, _(function)_ (example: WP-A, merge sort)

- **Tool:** _(for example ChatGPT)_
- **Prompt:** "Write a stable merge sort for std::vector in C++17 with a comparator, and state its time and space complexity."
- **Comparison with `modules/ds/Sorting.hpp`:** _(Is the algorithm the same? Are ties handled in the same way, i.e. is it stable [L4]? Are the complexity claims correct?)_
- **Test used as evidence:** `Sorting: merge, insertion and bubble sort are stable`, together with the mutant "merge takes from the RIGHT half on ties".
- **Scores (second tool):** Accuracy _/5 · Completeness _/5 · Clarity _/5 · Relevance _/5
- **Scores (Claude, same prompt):** Accuracy _/5 · Completeness _/5 · Clarity _/5 · Relevance _/5

_(Entries G3, G4, … are added for each member. At least one entry per member is expected, and at least two different tools must be used across the team [B].)_

---
## G3: Shop checkout all-or-nothing test (WP-B)

- **Date / member / work package:** 15 November 2026 / Kavin / WP-B
- **Tool and version:** Claude (Anthropic), Opus 5.5
- **Prompt (summary):** "Review my work package (modules/shop) for bugs or issues and fix them."
- **Output produced:** No defects found in the shop code. Identified that the all-or-nothing checkout rule had no test for the failure case, and wrote one (+30 lines in `tests/test_shop.cpp`).
- **Verification performed:** Built and ran all tests (6/6 suites pass, MinGW GCC 14.2). Mutation: merging the stock check and deduction loops in `Shop::checkout` passes every old test but fails the new one. `tools/mutation_test.py shop`: 3/3 caught.
- **Problems identified:** The first mutant it proposed (double deduction) was already caught by the old tests, so it did not prove the new test's value; replaced with a fairer mutant.
- **Changes made:** None to the generated test.
- **Scores:** Accuracy 4/5 · Completeness 4/5 · Clarity 4/5 · Relevance 5/5
- **Evidence:** Accuracy: test passes on the real code and fails on the merged-loop mutant, but the first mutant it proposed did not prove anything. Completeness: covers the all-or-nothing rule but left the `Money::parse("12.")` edge case untested. Clarity: comments explain each check, but faking low stock with a second catalogue needs a moment to understand. Relevance: targets the exact rule promised in the shop README (WP-B).

## G4: Cross-check with a second tool, A* on a grid (WP-C)

- **Date / member / work package:** 15 November 2026 / liyou / WP-C
- **Tool and version:** ChatGPT (GPT-6)
- **Prompt (summary):** "Implement A* on a 4-connected grid in C++ with a binary heap. Explain why the first time the goal is popped the path is optimal." (new chat, no project code shown)
- **Output produced:** C++ A* with `std::priority_queue` and a Manhattan heuristic, a proof by contradiction, a note on consistency, a complexity table and exam tips.
- **Verification performed:** Its code, saved unmodified, was compared with a plain BFS on 100,000 random grids (3x3 to 20x20, 0-45% walls) with ASan and UBSan on: 0 mismatches, including 16,106 unreachable cases, all reported "not found". Start = goal gives cost 0 and a one-cell path; a start or goal on a wall or out of bounds gives "not found". Compared line by line with `modules/navigation/Pathfinding.cpp`.
- **Comparison with `Pathfinding.cpp`:** Same algorithm and heuristic; the tie-break is equivalent (larger g versus smaller h). Stale entries: ChatGPT skips entries with `g != gScore` (cells can be reopened), while the project uses a `closed[]` flag that relies on a consistent heuristic. ChatGPT rejects a start or goal on a wall; `aStarPath` does not check this (a path from a shelf cell is found). ChatGPT has no blocked-cell mask; the project has one, with the goal exempt. ChatGPT's path includes the start cell; the project's does not.
- **Problems identified:** Step 3 of its proof does not state that a node on the optimal path is on the open list with its optimal g. It adds an unrequested exam-tips section and an offer of a practice exercise.
- **Changes made:** None to its code. Finding for the project: add start/goal validation to `aStarPath` (planned, with a test first).
- **Complexity and data structures:** Both are A* with a binary-heap open list and Manhattan distance, with time O(V log V) (the project's header says O(E log V), the same since E ≤ 4V) and space O(V); ChatGPT states both bounds without being asked and they match. The project uses the course's own `ds::MinHeap` and flat per-cell arrays, ChatGPT uses `std::priority_queue` and nested vectors of pairs, so the big-O is the same and only the data-structure choice differs.
- **Scores (ChatGPT):** Accuracy 4/5 · Completeness 4/5 · Clarity 4/5 · Relevance 4/5
- **Scores (Claude, same prompt, project code):** Accuracy 5/5 · Completeness 4/5 · Clarity 5/5 · Relevance 5/5
- **Evidence:** ChatGPT accuracy: 0 mismatches on 100,000 grids, but the proof's step 3 is loose. Completeness: handles the edge cases but has no blocked-cell mask. Clarity and relevance: long answer with unrequested extras. Claude: test "A*: same length as BFS on 500 random pairs of the real map" and the mutation runs pass; no start/goal validation (completeness); the header explains consistency and lazy deletion (clarity); it fits `GridMap` and `ds::MinHeap` (relevance).

## G5: Extra tests and mutants for WP-C navigation

- **Date / member / work package:** 15 November 2026 / liyou / WP-C
- **Tool and version:** Claude (Anthropic), Sonnet 5.5
- **Prompt (summary):** "Review the navigation module for bugs and guide me through verification, validation and experiments."
- **Output produced:** A review of `modules/navigation`; 5 new test cases (blocked goal in `bfsPath`, `DistanceTable` rejecting non-walkable cells, the `exactLimit` boundary, an unreachable dock, sorter induction cells); 4 mutants for `tools/mutation_test.py`; a standalone probe program (not committed).
- **Verification performed:** All tests pass (16/16 before, 21/21 after), including under ASan and UBSan and in the full run across all modules. `tools/mutation_test.py navigation`: 3 surprises before the new tests, 0 after. I also ran a "break it on purpose" change for every one of the 16 original tests (15 detected; the tiny-map A* test did not detect a 3x heuristic, but the real-map test did).
- **Problems identified:** The tool first said the suite had 15 tests; it has 16. Its first suggested mutant for the invalid-maps test would have read an empty list (undefined behaviour) and was replaced with a safe one. It reported findings 1-5 (wrong line numbers in `GridMap` errors, `bfsDistances` ignoring the blocked-goal rule, the O key not respecting `maxObstacles`, k!*2^k understating shelves with three sides, `aStarPath` accepting a non-walkable start); these are not fixed in this PR.
- **Changes made:** Appended the 5 tests to `modules/navigation/tests/test_navigation.cpp` and the 4 mutants to `tools/mutation_test.py`; no source changes.
- **Scores:** Accuracy 4/5 · Completeness 4/5 · Clarity 4/5 · Relevance 5/5
- **Evidence:** Accuracy: all tests pass and the mutation run goes from 3 surprises to 0, but two mistakes (the test count and the unsafe mutant). Completeness: closes three gaps, but findings 1-5 are only reported. Clarity: test names state the claim, and the section header explains why they were added. Relevance: three tests target the surviving mutants (exactLimit, DistanceTable, bfsPath); the dock and induction-cell tests pin behaviour the old tests did not check.

## G6: E9 search-box experiment, linear scan vs suffix array (WP-B)

- **Date / member / work package:** 16 November 2026 / Kavin / WP-B
- **Tool and version:** Claude (Anthropic), Opus 5.5
- **Prompt (summary):** "Design an experiment for my work package (shop) based on the course slides, to justify whether the current algorithm is good enough or can be improved. Compare algorithms at a second-order level." Slides L3, L4, L5a, L7 and L8 and `experiments/README.md` were provided.
- **Output produced:** Experiment E9 (`experiments substring`): the search box compared as (A) the current scan, (B) a scan over names lower-cased once and (C) a suffix array (all suffixes sorted with `mergeSort`, then found by binary search). Chart function `plot_substring` and README rows.
- **Verification performed:** The experiment checks that A, B and C return identical products for all 300 searches at every size ("same = yes"). Binary-search comparisons match log₂ S (9.9 vs 9.8 up to 20.8 vs 20.7). Re-run on my own machine (MinGW GCC 14.2): every count column identical to the AI's reference CSV; only timings differ. AddressSanitizer build reported no errors.
- **Problems identified:** The first version contained a bug in the suffix comparison (a shared buffer made each suffix be compared with itself), so the searches returned wrong products. The built-in "same results" check detected it, and it was replaced by a direct `std::string::compare` on positions. Its first README text also overstated the memory per name (corrected to 10–17 entries per name).
- **Changes made:** Fixed comparison as above; README memory figure corrected.
- **Scores:** Accuracy 3/5 · Completeness 4/5 · Clarity 4/5 · Relevance 5/5
- **Evidence:** Accuracy: the first version returned wrong products (all three searches disagreed) until the suffix comparison was fixed; the final version gives "same = yes" on all 300 searches at every size. Completeness: covers the search box end to end (correctness, comparisons, time, memory, break-even), but searches names only (not categories) and has no searches for text that matches nothing. Clarity: the header comment explains the question, the three versions and why a suffix array allows binary search; the table columns need the README to decode. Relevance: uses the course's mergeSort [L4] and binary search [L5a, L7] on the shop's own search function and answers "good enough or improve?" directly.
---

## G7: E10 route-improvement experiment (greedy + 2-opt), WP-C

- **Date / member / work package:** 16 November 2026 / liyou / WP-C (experiments)
- **Tool and version:** Claude (Anthropic), Claude Code, Sonnet 5.5
- **Prompt (summary):** "Design a standalone experiment in `experiments/experiments.cpp` that tests whether the current route planner can be improved: compare greedy, greedy plus a local search, and the exact backtracking search on the same trips as E4. Keep it outside the navigation module."
- **Output produced:**
  - `experimentRouteImprovement()` (E10) and its helpers: `routeLength`, and `improveRoute`, which repeats 2-opt segment reversal and "visit a shelf from its other side" moves until no move shortens the route;
  - the `improve` dispatch key and usage string;
  - `plot_route_improvement()` in `plot_results.py`, which draws route length and time in one chart.
- **Verification performed:**
  - I ran E10 myself in a Codespace. The route lengths, gaps and counts are identical to the tool's own run; only the microsecond timings differ.
  - The experiment uses the same seed and trip sampling as E4. The greedy and exact averages in E10 equal E4's (for example k=5: 78.5 and 68.8; k=10: 99.6 and 79.1).
  - Checks on the CSV (1000 trips): 2-opt is never shorter than exact, never longer than greedy, and greedy is never shorter than exact.
  - The navigation module is unchanged, so its 21 tests and the dependency check still pass (`ctest`, `check_dependencies.py`).
- **Problems identified:**
  - The first version of the experiment was named E9, which a teammate had already taken. I renamed it to E10.
  - The tool-supplied `plot_results.py` was built from an older copy and would have deleted a teammate's E8 plotting code, so only the E10 function was pasted in.
  - 2-opt does not always reach the optimum: it matches exact in 100/100 trips at k=1 but only 26/100 at k=10 (gap 10.3%). It is a heuristic, not a replacement for exact search.
- **Changes made:** added E10 to `experiments.cpp`, an E10 plot to `plot_results.py`, and `e10_route_improvement.csv` and `e10_route_improvement.png` to `experiments/results/`. No navigation source files were changed. <add anything you edited by hand>
- **Scores:** Accuracy <4>/5 · Completeness <4>/5 · Clarity <4>/5 · Relevance <5>/5
- **Evidence for the scores:**
  - Accuracy: the output compiled and ran first time, and my Codespace run reproduced every route length; deduction because the first plot script would have removed E8 and the experiment name clashed with E9.
  - Completeness: compares three methods on the same 100 trips for k=1..10 with CSV and chart; deduction because it only tests one local search (2-opt plus side change) and does not try Held-Karp or other improvements.
  - Clarity: the code is commented and follows the style of E4; the chart titles and table columns are readable.
  - Relevance: it directly answers Calix's request to justify the current algorithm or improve it. It supports keeping `exactLimit = 8` and suggests greedy + 2-opt as a fallback above it.

## G8: E14 Held-Karp vs backtracking experiment, WP-C

- **Date / member / work package:** 25 November 2026 / liyou / WP-C (experiments)
- **Tool and version:** Claude (Anthropic), Claude Code, Sonnet 5.5
- **Prompt (summary):** "Suggest and write an experiment for navigation: compare an exact method that scales better than backtracking with pruning, on the same trips as E4, standalone outside the navigation module."
- **Output produced:** `experimentHeldKarp()` (E14) with a Held-Karp dynamic program over (subset of shelves, last shelf, side), the `heldkarp` dispatch key, and `plot_heldkarp()` in `plot_results.py`.
- **Verification performed:** I ran E14 in a Codespace: the route lengths, node counts and state counts are identical to the tool's own run (only microseconds differ). Both methods gave the same length on all 1,040 trips with both methods (k = 1 to 12), 0 mismatches, and the exact lengths for k <= 10 equal the exact column of E4. One untimed run fills the DistanceTable cache first so both methods are timed on lookups only.
- **Problems identified:** The tool first suggested raising `exactLimit` above 8. I checked `tripCapacityUnits` (`core/Config.hpp:35`, enforced in `JobManager.cpp:207`) and it is 8, so trips never exceed 8 shelves and are already planned exactly; the suggestion would change no route. Timings at k = 1 to 4 are a few microseconds and noisy. For k = 11 to 14 only 20 trips were used, each k with a different sample.
- **Changes made:** added E14 to `experiments.cpp`, its plot to `plot_results.py`, and `e14_heldkarp.csv` / `e14_heldkarp.png` to `experiments/results/`. No navigation source files were changed. Conclusion: Held-Karp is 17x faster at k = 8 and 68x at k = 10 with identical routes, but the module needs no change; it is a future improvement if the trip capacity is raised.
- **Scores:** Accuracy <4>/5 · Completeness <4>/5 · Clarity <4>/5 · Relevance <4>/5
- **Evidence for the scores:**
  - Accuracy: ran first time, my Codespace run reproduced every length and count, 0 mismatches over 1,040 trips; one point off because the first recommendation (raise `exactLimit`) was wrong for this project and had to be corrected after I checked the code.
  - Completeness: compares two exact methods for k = 1 to 14 with CSV and chart; one point off because k >= 11 uses only 20 trips and Held-Karp was not added to the module or its tests.
  - Clarity: commented, follows the style of E4 and E10; chart and table are readable.
  - Relevance: it tests whether a better exact algorithm would improve the program; the answer (no change needed at capacity 8) is itself useful for justifying the current design.
  
## G10: Cross-check with a second tool, product catalogue (WP-B)

- **Date / member / work package:** 25 November 2026 / Kavin / WP-B
- **Tool and version:** ChatGPT (GPT-6 Luna, medium)
- **Prompt (summary):** "In C++17, write a product catalogue: exact name lookup (case-insensitive), substring search, exact prices; state time and space complexity." (new chat, no project code)
- **Verification performed:** Its code, unmodified, compiled without warnings and was run against `wh::Catalogue` on the 91 real products with ASan/UBSan: 457 exact lookups and 1,567 substring searches, 0 differences on names; identical stock total.
- **Comparison:** Lookup: project uses a sorted index + binary search, O(L log N), N extra integers; ChatGPT uses a hash map, O(L) average, a copy of every name as extra memory; 69 ns vs 320 ns per lookup. Substring search: same linear scan in both. Project also searches categories and rejects duplicate names; ChatGPT does neither.
- **Scores (ChatGPT):** Accuracy 5/5 · Completeness 4/5 · Clarity 4/5 · Relevance 4/5
- **Evidence:** 0 differences in all tests (accuracy); no hash worst case, no price parsing, accepts duplicates (completeness); substring complexity ignores query length (clarity); generic, no categories (relevance).

## G9: Fix of defects F2 and F5 in Pathfinding.cpp (tests first), WP-C

- **Date / member / work package:** 25 November 2026 / liyou / WP-C (navigation)
- **Tool and version:** Claude (Anthropic), Claude Code, Sonnet 5.5
- **Prompt (summary):** "Fix F2 (bfsDistances calls a blocked goal unreachable, bfsPath and A* do not) and F5 (A* accepts a start on a shelf, DistanceTable says unreachable). Write the tests first."
- **Output produced:** 3 new tests in `test_navigation.cpp`, a change to `Pathfinding.cpp` (a `walkableEndpoints` check in `bfsPath` and `aStarPath`; `bfsDistances` records the distance to a blocked cell but does not expand it), a comment update in `Pathfinding.hpp`, and 3 new mutants in `tools/mutation_test.py`.
- **Verification performed:**
  - The 3 new tests failed on the old code and pass after the fix; the navigation test program passes 24/24 and `ctest` passes.
  - `python3 tools/mutation_test.py navigation`: 7 mutants, 0 surprises (the 3 new mutants are all caught).
  - Callers checked by `grep`: `bfsDistances` is only called without a blocked mask (`GridMap.cpp:131`, `DistanceTable.cpp:22`); `aStarPath` is only called from `TrafficController.cpp:50`, and its goals (home, dock, access cell, induction cell) are all walkable, so no existing behaviour changes.
  - E4 route results are identical before and after the change; `check_dependencies.py` passes.
- **Problems identified:** The rule for a non-walkable start (reject it) was the tool's design choice, copied from `DistanceTable`; I confirmed it with the caller check above, since a wrong rule could have stopped a robot from planning.
- **Changes made:** applied the patch with the tool's script, then removed the helper files; no other source files changed.
- **Scores:** Accuracy <4>/5 · Completeness <4>/5 · Clarity <4>/5 · Relevance <5>/5
- **Evidence for the scores:**
  - Accuracy: the tests failed before and passed after, and all 3 new mutants are caught; one point off because the design choice for non-walkable starts needed my caller check.
  - Completeness: fixes both defects with tests and mutants; F1 (wrong line numbers in `GridMap` errors) is still open.
  - Clarity: small change, commented, same style as the module.
  - Relevance: both defects came from my own validation of WP-C.

## Summary table (for the report)

| Entry | Member | Tool | Task | Acc. | Comp. | Clar. | Rel. | Principal finding |
|-------|--------|------|------|-----:|------:|------:|-----:|-------------------|
| G1 | | Claude | Complete code base | | | | | |
| G2 | | | | | | | | |
| G3 | Kavin | Claude | Shop checkout test (WP-B) | 4 | 4 | 4 | 5 | No code defects; added missing all-or-nothing checkout test |
| G4 | liyou | ChatGPT | A* cross-check (WP-C) | 4 | 4 | 4 | 4 | Code correct (0 mismatches on 100,000 grids); validates start/goal, which `aStarPath` does not |
| G5 | liyou | Claude | Extra tests and mutants (WP-C) | 4 | 4 | 4 | 5 | 3 test gaps closed: 21/21 tests, 0 mutation surprises |
| G6 | Kavin | Claude | E9 search-box experiment (WP-B) | 3 | 4 | 4 | 5 | First version had a suffix-comparison bug caught by the results check; current search is good enough for 91 products |
| G7 | liyou | Claude | E10: greedy + 2-opt vs exact routes | 4 | 4 | 4 | 5 | 2-opt cuts greedy's gap from 25.9% to 10.3% at k=10 in microseconds, but is exact in only 26/100 trips |
| G8 | liyou | Claude | E14: Held-Karp vs backtracking exact routes | 4 | 4 | 4 | 4 | Same optimal routes; Held-Karp 17x faster at k = 8, 68x at k = 10, but trips never exceed 8 shelves, so no change needed |
| G9 | liyou | Claude | Fix F2 + F5 in Pathfinding.cpp | 4 | 4 | 4 | 5 | BFS and A* now agree on blocked goals and reject non-walkable start/goal; 3 tests and 3 mutants added |
| G10 | Kavin | ChatGPT | Catalogue cross-check (WP-B) | 5 | 4 | 4 | 4 | Same results on 91 products; hash map 4-5x faster per lookup but uses more memory; same search-box scan |
