# Generative AI usage and evaluation log

The project brief [B] requires the use of at least two generative AI tools to assist in generating basic source code, and an evaluation of their responses against four criteria: **accuracy, completeness, clarity and relevance**. This log records the documented uses and retrospective experiment evidence, forming the basis of the generative AI section of the report.

**Recording procedure.** One entry is made per question or task. The prompt is summarised briefly or linked. Scores follow the rubric in `VALIDATION_GUIDE.md`, and **each score must be supported by evidence** such as a test name, a line number or a counter-example. Citation keys refer to the *References* section of the root `README.md`. Updated 25 November 2026 using the latest supplied team log and project archive. Member-recorded ratings are retained; retrospective ratings are marked `P` (proposed), and `NR` indicates insufficient evidence to rate.

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

## G1: Initial version of the code base (baseline)

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

  **These are historical claims made by the tool itself.** The current archive has since evolved. The fresh verification below tests the current archive, not the original 26 October snapshot.
- **Limitations stated by the tool:** see `DESIGN_DECISIONS.md` §3. In particular, the traffic rules are a heuristic with no proof that every congestion situation resolves, and the route planner does not account for other robots.
- **Verification performed:** The 25 November archive was freshly built in Linux with GCC 14.2.0, Release mode, UI and experiments enabled, and warnings treated as errors. `ctest` passed all 6/6 test executables: ds 23/23, shop 14/14, navigation 24/24, picking 11/11, fulfilment 21/21, sim 7/7 (100/100 individual tests). `python3 tools/check_dependencies.py` returned `dependency check: OK`. This was a retrospective assistant-run check, not a claim that each member personally repeated all seven validation steps.
- **Problems identified:** Later entries G3–G12 document missing tests, edge cases, endpoint handling and traffic/state defects; G13 documents a stale fulfilment mutation-script match. The original baseline's 360-run stress and 20-mutant claims have not been independently reproduced on the original snapshot.
- **Changes made:** See G3–G12 for the team-recorded patches. G13–G15 add retrospective comparison evidence; no production code was changed in this log update.
- **Scores (original baseline):** Accuracy NR · Completeness NR · Clarity NR · Relevance NR. The original source snapshot and complete response are not supplied; scoring a later revision as the original response would be misleading.
- **Evidence:** Current-build commands: `cmake -S . -B build -DWH_BUILD_UI=ON -DWH_WARNINGS_AS_ERRORS=ON -DCMAKE_BUILD_TYPE=Release`, `cmake --build build`, `ctest --test-dir build --output-on-failure`, `python3 tools/check_dependencies.py`. The module-specific tests and CSV results in G3–G15 provide additional evidence for the later code. Owner hand traces and independent reviewer sign-offs remain unverified.

## G2: Original merge-sort cross-check example (not an executed evaluation)

- **Status:** The original log supplied an illustrative C++17 stable merge-sort prompt, but no actual second-tool response or scores were recorded. It is retained as a historical placeholder, not counted as an evaluation.
- **Example prompt:** "Write a stable merge sort for std::vector in C++17 with a comparator, and state its time and space complexity."
- **Related evidence:** `Sorting: merge, insertion and bubble sort are stable`; mutation `merge takes from the RIGHT half on ties`. See G14 / E13 for the actual data-structures comparison. No score is assigned to the example.

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
- **Changes made:** None to its code. Finding for the project: add start/goal validation to `aStarPath` (planned at the time; subsequently fixed with tests in G9).
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
- **Changes made:** added E10 to `experiments.cpp`, an E10 plot to `plot_results.py`, and `e10_route_improvement.csv` and `e10_route_improvement.png` to `experiments/results/`. No navigation source files were changed.
- **Scores:** Accuracy 4/5 · Completeness 4/5 · Clarity 4/5 · Relevance 5/5
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
- **Scores:** Accuracy 4/5 · Completeness 4/5 · Clarity 4/5 · Relevance 4/5
- **Evidence for the scores:**
  - Accuracy: ran first time, my Codespace run reproduced every length and count, 0 mismatches over 1,040 trips; one point off because the first recommendation (raise `exactLimit`) was wrong for this project and had to be corrected after I checked the code.
  - Completeness: compares two exact methods for k = 1 to 14 with CSV and chart; one point off because k >= 11 uses only 20 trips and Held-Karp was not added to the module or its tests.
  - Clarity: commented, follows the style of E4 and E10; chart and table are readable.
  - Relevance: it tests whether a better exact algorithm would improve the program; the answer (no change needed at capacity 8) is itself useful for justifying the current design.

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
- **Scores:** Accuracy 4/5 · Completeness 4/5 · Clarity 4/5 · Relevance 5/5
- **Evidence for the scores:**
  - Accuracy: the tests failed before and passed after, and all 3 new mutants are caught; one point off because the design choice for non-walkable starts needed my caller check.
  - Completeness: fixes both defects with tests and mutants; F1 (wrong line numbers in `GridMap` errors) is still open.
  - Clarity: small change, commented, same style as the module.
  - Relevance: both defects came from my own validation of WP-C.

## G10: Cross-check with a second tool, product catalogue (WP-B)

- **Date / member / work package:** 25 November 2026 / Kavin / WP-B
- **Tool and version:** ChatGPT (GPT-6 Luna, medium)
- **Prompt (summary):** "In C++17, write a product catalogue: exact name lookup (case-insensitive), substring search, exact prices; state time and space complexity." (new chat, no project code)
- **Verification performed:** Its code, unmodified, compiled without warnings and was run against `wh::Catalogue` on the 91 real products with ASan/UBSan: 457 exact lookups and 1,567 substring searches, 0 differences on names; identical stock total.
- **Comparison:** Lookup: project uses a sorted index + binary search, O(L log N), N extra integers; ChatGPT uses a hash map, O(L) average, a copy of every name as extra memory; 69 ns vs 320 ns per lookup. Substring search: same linear scan in both. Project also searches categories and rejects duplicate names; ChatGPT does neither.
- **Scores (ChatGPT):** Accuracy 5/5 · Completeness 4/5 · Clarity 4/5 · Relevance 4/5
- **Evidence:** 0 differences in all tests (accuracy); no hash worst case, no price parsing, accepts duplicates (completeness); substring complexity ignores query length (clarity); generic, no categories (relevance).

## G11: E15 DistanceTable memoisation experiment, WP-C

- **Date / member / work package:** 25 November 2026 / liyou / WP-C (experiments)
- **Tool and version:** Claude (Anthropic), Claude Code, Sonnet 5.5
- **Prompt (summary):** "Write a standalone experiment for navigation that shows what the DistanceTable cache saves."
- **Output produced:** `experimentMemoisation()` (E15), which runs one exact route search three ways (a BFS for every question, a cold `DistanceTable` per trip, one warm `DistanceTable` for the whole run), plus `plot_memoisation()` and the `memo` dispatch key.
- **Verification performed:** I ran E15 in a Codespace: the question counts and BFS counts are identical to the tool's own run (only microseconds differ), and all three methods gave the same route length on every trip (0 mismatches).
- **Problems identified:** `RoutePlanner` is hard-wired to `DistanceTable`, so the experiment uses its own exact search (Held-Karp) as the workload rather than the planner itself. k = 7 and 8 use only 20 trips, and k = 1 to 3 timings are tiny and noisy.
- **Changes made:** added E15 to `experiments.cpp` and `plot_results.py`, with `e15_memoisation.csv` and `e15_memoisation.png`. No navigation source files changed. Conclusion (current archived CSV, ratio of mean times): at k = 8 the cache is about 539x faster when cold and 2,056x when warm (316,021.895 µs without memoisation, 586.650 µs cold, 153.685 µs warm). The earlier rounded estimates were 500x and 1,600x; the design conclusion is unchanged.
- **Scores:** Accuracy 4/5 · Completeness 4/5 · Clarity 4/5 · Relevance 4/5
- **Evidence for the scores:** Accuracy: my run reproduced all counts with 0 mismatches; one point off because the workload is a stand-in for the planner. Completeness: three arms for k = 1 to 8, but fewer trips at k = 7 and 8. Clarity: commented and in the style of E4. Relevance: it measures a design choice WP-C already made.

## G12: Code review, bug fixes and extra tests for WP-D picking

- **Date / member / work package:** 25 November 2026 / raelyn / WP-D (picking)
- **Tool and version:** Google Antigravity (Gemini)
- **Prompt (summary):** "Is there any bugs / issues to be fixed or improvements in the codes for modules/picking?"
- **Output produced:**
  - A full audit of `modules/picking` (`Battery.hpp`, `DockingStation.hpp`, `PickingRobot.hpp/.cpp`, `TrafficController.hpp/.cpp`);
  - 6 targeted fixes across `Battery.hpp`, `PickingRobot.hpp/.cpp` and `TrafficController.cpp` with inline comments;
  - 2 new test cases (`PickingRobot: argument validation and state encapsulation`, `PickingRobot: stranded robot with empty battery recovered by technician`) and negative-input checks in `Battery: stays between 0 and 100` in `modules/picking/tests/test_picking.cpp`;
  - A summary table and breakdown of the algorithms and data structures used in WP-D (FSM, `ds::LinkedQueue`, `ds::insertionSort`, A\* with dynamic obstacle replanning, and occupancy-based traffic rules).
- **Verification performed:**
  - Built and ran `test_picking` with MinGW GCC 14.2.0: 11/11 tests pass (up from 9/9);
  - Built and ran `test_sim` (80 orders with obstacles and 48 multi-robot stress runs): 7/7 tests pass with 0 collisions;
  - Checked all callers of `isAvailable()`, `isBusy()` and `tripId()` across `fulfilment/JobManager.cpp` and `ui/Views.cpp` to confirm no regressions in other modules.
- **Problems identified:**
  - The first run of the new test `PickingRobot: stranded robot with empty battery recovered by technician` failed (10/11 passed). `World::run` stops as soon as no robot satisfies `wantsToMove() || isBusy()`. Because a stranded robot with 0% battery had `wantsToMove() == false` and `isBusy() == false`, `World::run` exited at tick 1 before the technician swap at tick 30.
  - Investigating that failure exposed a related defect in `PickingRobot::isAvailable()`: a robot with 0% battery in `Idle` or `Returning` state still returned `isAvailable() == true` (which `JobManager.cpp:268` had been patching around with `r->isAvailable() && r->battery() > 0.0`).
- **Changes made:**
  - `TrafficController.cpp`: added missing `metrics.longestWait` update when `replan` fails in Rule 2a (line 155); recomputed `currentNext` and `currentBlocker` after Rule 2d A\* detour replanning so `sidestep` checks the actual blocking robot rather than the pre-detour blocker (lines 196–201); reset `plans_[i].next = 0` when clearing `path` in `sidestep` (line 114).
  - `PickingRobot.hpp`: added `!battery_.isEmpty()` to `isAvailable()` (line 53) and `battery_.isEmpty()` to `isBusy()` (line 76).
  - `PickingRobot.cpp`: added `dropCell == kNoCell || sorterIndex < 0` validation in `assignTrip` (line 63); reset `dropCell_ = kNoCell` and `tripId_ = 0` upon delivery in `State::Dropping` (lines 153–154); added `shelves.reserve(route_.size())` in `shelvesLeft()` (line 54).
  - `Battery.hpp`: guarded `drain` and `charge` with `if (amount > 0.0)` against negative values (lines 18–19).
  - `tests/test_picking.cpp`: expanded from 9 to 11 unit tests. No files outside `modules/picking` were modified.
- **Scores:** Accuracy 4/5 · Completeness 5/5 · Clarity 5/5 · Relevance 5/5
- **Evidence for the scores:**
  - Accuracy: all 11/11 picking tests and 7/7 simulation stress tests pass after the fixes, and `JobManager.cpp:268` confirmed the `isAvailable()` flaw; one point deducted because the stranded-robot test failed on its first run until `isBusy()` was updated to include `battery_.isEmpty()`.
  - Completeness: covered all source and header files in `modules/picking`, fixed metric, traffic-blocker and state-cleanup defects, added unit tests, provided inline comments, and mapped WP-D algorithms to course materials ([L1b], [L2], [L4]).
  - Clarity: inline comments and file/line citations clearly explain every change.
  - Relevance: strictly respected the module boundary (only `modules/picking` touched) and directly targeted WP-D's state machine and traffic controller.

---

## G13: Fulfilment module cross-check — robot assignment, charging and memory (E11 and E12, WP-E)

- **Date / member / work package:** Sam / WP-E (`modules/fulfilment`) / results in the 25 November 2026 archive; original experiment date and exact prompt date not recorded. The WP-E ownership is documented in the root README.
- **Tools and versions:** OpenAI Codex and Anthropic Claude. Exact model versions are not recorded for these runs.
- **Prompt (reconstructed task summary, not a quotation):** Compare alternative AI-generated improvements to the warehouse fulfilment module against the existing scheduling algorithm; examine whether the alternatives improve order completion, travel distance, charging decisions, code quality and memory use. Original prompts and responses are not included in this archive.
- **Output produced (as documented in the experiment results):**
  - Baseline `base_first_available`: assign the first available robot.
  - Baseline `base_nearest`: select the robot closest to an access cell of any required shelf. This is the nearest-robot policy in the current `modules/fulfilment/JobManager.cpp` (`chooseRobot`).
  - Codex `codex_shortest_trip`: choose a candidate using the planned complete trip, including delivery to the sorter, instead of only the closest shelf. Recorded build switch: `WH_FULFILMENT_SHORTEST_TRIP`.
  - Claude `claude_attempt_defer_charging`: defer immediate charging of skipped robots during assignment and use a charging fallback when no robot can take the trip. Recorded switch: runtime `setDeferCharging` plus build default `WH_FULFILMENT_DEFER_CHARGING`.
  - These are different scheduling improvements to the same module, not identical algorithms. The two baseline policies are controls, not additional AI responses in this evaluation.

### Verification evidence — E11: performance, test coverage and code complexity

The archived `e11_all_results.csv` contains 6 scenarios × 4 policies × 50 seeds = **1,200 simulation runs** (10,800 raw metric rows). The scenarios cover 60–200 orders, 3–8 robots, greedy/backtracking routing and obstacles enabled/disabled. Across all policies, 130,000 of 130,000 orders were completed and zero collisions were recorded. This is evidence for the tested conditions, not a guarantee of safety or liveness on all possible maps.

| S1: 80 orders, 8 robots, backtracking, obstacles | Mean distance (cells) | Mean completion (ticks) | Mean lead time (ticks) |
|---|---:|---:|---:|
| First available (baseline) | 4,216.92 | 707.72 | 356.946 |
| Nearest (baseline) | 4,221.64 | 717.88 | 352.612 |
| Codex: complete-trip scoring | 4,195.46 | 707.92 | 350.346 |
| Claude: defer charging | 4,215.72 | 714.94 | 352.548 |

Codex reduced mean distance by about 0.62% and completion time by 1.39% relative to the nearest baseline in S1. Claude reduced them by about 0.14% and 0.41%, respectively. The paired tests for Codex against nearest report nominal completion-time p-values of 0.0351 (S1) and 0.0439 (S5); the other four scenarios do not reach 0.05. These are exploratory tests without a documented multiple-comparison adjustment; they do not establish a universal or statistically robust advantage. First available was also slightly faster than Codex on S1 mean completion time, although its mean lead time was longer.

| Recorded code / test metric | Nearest baseline | Codex | Claude |
|---|---:|---:|---:|
| Fulfilment `TEST_CASE` count | 21 | 22 | 23 |
| `chooseRobot` non-comment lines of code | 19 | 35 | 19 |
| `chooseRobot` cyclomatic complexity | 8 | 14 | 8 |
| `dispatchTrips` non-comment lines of code | 53 | 53 | 60 |
| `dispatchTrips` cyclomatic complexity | 18 | 18 | 20 |
| Variant-specific mutants caught by own suite | — | 2/4 | 4/5 |
| Same variant-specific mutants caught by baseline suite | — | 2/4 | 0/5 |

Mutation sets differ, so the percentages are not a like-for-like ranking. Codex tests caught choosing the longest route and excluding the sorter leg. The FirstAvailable-policy mutant and changed tie-breaking survived (the latter may be equivalent unless the tie rule is a requirement). Claude tests caught four charging-related defects, but the mutant that does not return skipped robots to the idle pool survived. Surviving mutants indicate possible test gaps, not proof that the unmutated code contains those defects.

### Verification evidence — E12: memory and space overhead

`e12_all_results.csv` contains full-simulation memory measurements for four order-count settings (three variants × ten seeds at each setting) and dispatch microbenchmarks. Measurements distinguish peak live heap, cumulative allocated bytes, allocation count and peak resident set size (RSS). Heap measurements use a malloc shim; RSS is recorded using `wait4` / `ru_maxrss` according to the archive.

| Full simulation: 400 orders, 8 robots, backtracking (mean of 10 seeds) | Nearest baseline | Codex | Claude |
|---|---:|---:|---:|
| Peak live heap (bytes) | 1,889,796.0 | 1,880,541.6 | 1,886,648.0 |
| Cumulative allocation (bytes) | 26,139,092.8 | 26,409,185.6 | 26,153,056.8 |
| Allocation count | 353,676.4 | 361,938.0 | 353,205.3 |
| Peak RSS (KB, as recorded) | 11,632 | 11,632 | 11,632 |
| `sizeof(JobManager)` (bytes) | 232 | 232 | 240 |
| `JobManager.o` size (bytes) | 61,072 | 61,648 | 62,793 |

The three variants have very similar peak memory in this scenario. Codex reduced peak live heap slightly (about 0.49%) but increased cumulative allocations and allocation count. Claude added 8 bytes to `JobManager`; object-file size increased by 2.82% compared with 0.94% for Codex. These binary and heap sizes are build/platform dependent and are not asymptotic space-complexity bounds.

The dispatch microbenchmark also covers 50, 100, 200 and 400 pending orders at starting battery levels of 100% and 45%, with ten seeds per variant and setting (240 runs). At 400 pending orders and 45% battery, the mean extra peak heap during update is 73,498.4 bytes (nearest), 66,000.0 bytes (Codex) and 73,500.0 bytes (Claude). Codex uses about 10.2% less temporary peak memory in this particular dispatch test, but that does not imply a similar improvement in the entire simulation.

### Evaluation and decision

- **Problems / limitations identified:** Codex repeats full route evaluation for candidates and increases the complexity of `chooseRobot`; its performance improvement is small and scenario dependent. Claude adds charging-state/dispatch logic and has an uncovered idle-pool restoration mutant. Neither variant demonstrates a material peak-memory improvement at 400 orders. The full external comparison harness and alternative source snapshots are absent from the archive; the CSV records outcomes rather than a complete independent reproducibility package.
- **Changes made:** The current project still contains nearest-shelf selection (or FirstAvailable) and immediate charging of a robot with insufficient energy for the trip. Neither experimental variant is present as a selectable implementation in the archived `JobManager.cpp`. No new production-code modification is claimed here. Recommended decision: retain the simpler baseline; consider Codex's complete-trip scoring only if gains justify the extra complexity, and improve charging/policy tests before adoption.
- **Proposed scores (Codex, pending owner approval):** Accuracy 4/5 P · Completeness 4/5 P · Clarity 4/5 P · Relevance 4/5 P.
- **Proposed scores (Claude, pending owner approval):** Accuracy 4/5 P · Completeness 4/5 P · Clarity NR · Relevance 4/5 P.
- **Evidence for the proposed scores:**
  - Accuracy: all archived scenarios complete without reported collisions for both variants. A provisional 4 is conservative because the alternative code and hand traces are unavailable; no specific defect in the unmutated variants is established by these results.
  - Completeness: both integrate an improvement and add tests, but Codex's policy mutant and Claude's idle-pool mutant expose gaps; no claim of complete edge-case coverage is justified.
  - Clarity: Codex supplied 21 added/3 removed README lines and its main change is isolated in `chooseRobot`, but its complexity rose from 8 to 14; 4 is a tentative documentation/complexity-based rating, not a confirmed first-hand readability review. Claude added no README lines in the recorded comparison; source/readability evidence is insufficient to give a defensible numerical clarity rating.
  - Relevance: both address the reconstructed scheduling objective; the small gains and added complexity support a provisional 4 rather than an unqualified 5. Confirm against the actual original prompt.
- **Evidence files:** `experiments/results/e11_all_results.csv` (sections `raw_run`, `summary`, `paired_test_vs_base`, `mutation_test`, `code_metric`, `table_E11`), `experiments/results/e12_all_results.csv` (sections `fullsim_raw`, `fullsim_summary`, `microbench_raw`, `microbench_summary`, `static_size` and paired-test sections), `experiments/results/plots/e11_results.png`, `e12_results.png`, `modules/fulfilment/JobManager.cpp`.

### Current WP-E regression and mutation verification (fresh retrospective check)

- Built the actual archived module and ran its tests: 21/21 passed, including priority/ageing, load balancing, weight and unit limits, nearest versus first-available assignment, charging, stable packing and order completion. Full project: 100/100 tests passed (G1).
- Ran the repository mutation script for fulfilment. Three mutations were caught by the unit suite: reverse backlog urgency, always assign the first sorter, and replace stable packing merge sort with quicksort. The fourth was **skipped**, not survived: the script's search string no longer matches the current `std::max(1, config_.tripCapacityUnits)` capacity expression in `JobManager.cpp`.
- Applied the equivalent corrected capacity mutant temporarily to the extracted working copy (`<` to `<=` in that expression). It compiled and failed `JobManager: trips never exceed the item or weight capacity` at `test_fulfilment.cpp:292` (20/21 pass). Restored the source and rebuilt. Thus all four intended baseline mutants were detected by tests when applied, but the committed script still needs its outdated match string updated. No source or script change is included in this deliverable.
- The fresh baseline results do not reproduce the missing E11/E12 variant harnesses, nor do they constitute personal owner/reviewer sign-off.

### Supporting fulfilment experiments (E6 and E7)

These are algorithm-validation evidence for WP-E, not additional independently attributed AI responses. E6 compares three routing and two assignment policies for 80 orders and eight robots over ten seeds per combination (60 runs). As-given/first-available averages 5,668.7 moves and 900.2 ticks; backtracking/nearest averages 4,222.3 moves and 693.0 ticks (25.52% fewer moves and 23.02% fewer completion ticks). The much larger benefit comes from routing rather than assignment alone. E7 (`experimentFulfilment`, command `fulfilment`) compares urgency-order first-fit, weight-descending first-fit and an exact minimum-trip backtracking reference for a two-constraint packing problem. In `e7_fulfilment.csv`, all 24,000 pools have `capped=0`; first-fit matched the exact minimum in 23,220/24,000 pools (96.75%), versus 21,182/24,000 (88.26%) for first-fit decreasing. This supports the current urgency-order greedy method on the tested workload. The configured trip limit is 8 units and 12 kg (`modules/core/Config.hpp`); E7's synthetic workload explores additional payload settings. Do not confuse E7's packing comparison with the archived E11/E12 AI scheduling comparison. The original AI prompt and authorship for E6/E7 are not established by their CSVs.

---

## G14: Data structures module cross-check — Claude versus Codex (E13, WP-A)

- **Date / member / work package:** Experiment date not recorded / Marissa (WP-A owner; experiment authorship to confirm) / `modules/ds`.
- **Tools and versions:** Anthropic Claude and OpenAI Codex; exact versions not recorded.
- **Prompt (reconstructed task summary):** Compare independently generated implementations of the project's sorting, searching, linked queue and min-heap data structures, including correctness, time/space trade-offs, compatibility and test quality. Original prompts and responses are unavailable.
- **Output produced:** Two replacement versions of `modules/ds`, compared in the same warehouse project. Claude has more course-linked functions and comments; Codex supports move-only types, a real queue move constructor, `emplace` and merge sort without a default-constructor requirement. These features are recorded in E13; do not assume they are present in the current production module.
- **Verification performed (archived results):** Both variants built with warnings as errors, passed 6/6 module test executables and built the UI. The neutral correctness harness recorded 1,780 sorting, 6,256 binary-search, 605 heap and 308 queue checks passed by each variant (8,949 total each), and no ASan/UBSan/leak errors. Claude passed its 23 tests and Codex its 4. Codex could not compile Claude's suite because of missing interfaces (`isValidHeap`, `items`, `binarySearchRecursive`, `detail::partition`); Claude passed Codex's 4 tests.

| Comparison | Claude | Codex |
|---|---:|---:|
| Quicksort comparisons, all-equal n=10,000 | 50,024,997 | 127,072 |
| Quicksort comparisons, random n=10,000 | 153,635 | 178,589 |
| Quicksort comparisons, reversed n=10,000 | 236,088 | 125,452 |
| Heap push/pop n=1,000,000 integers, median ms | 143.7758 | 110.9896 |
| Binary search max comparisons, n=1,000,000 | 20 | 20 |
| Header comment lines / non-blank lines | 149/440 (33.9%) | 8/196 (4.1%) |

- **Problems identified:** Codex's `MinHeap::emplace(5)` on heap `{1}` returns a reference to the top (1) instead of the inserted value (5), a likely interface defect identified by the feature probe, not used in the current warehouse. Claude's duplicate-heavy quicksort performs much more work (about 394x as many comparisons on 10,000 equal values) and it lacks several modern C++ features; these are design trade-offs, not automatically correctness defects. Timing results are machine-specific; the archive uses medians of three alternating runs, treating differences within 15% as ties for time results.
- **Changes made / decision:** No adoption of the Codex alternative is documented by E13. Keep the teaching-oriented implementation and test coverage; consider duplicate-aware quicksort and modern queue improvements separately, with regression tests and review of heap semantics. No overall winner is established.
- **Proposed scores (Claude):** Accuracy 4/5 P · Completeness 4/5 P · Clarity 4/5 P · Relevance 4/5 P.
- **Proposed scores (Codex):** Accuracy 3/5 P · Completeness 3/5 P · Clarity 3/5 P · Relevance 4/5 P.
- **Evidence for the proposed scores:** Accuracy: both pass the neutral checks; Claude is provisionally rated 4 pending source/hand trace; the Codex `emplace` counterexample supports 3 if the intended contract is to return the inserted item. Completeness: Claude has broader tests and course-linked APIs but lacks move-only support; Codex has fewer tests and missing existing interfaces (not all may have been specified in the original task). Clarity: Claude's greater documentation and course-linked functions support a tentative 4; Codex's 4.1% comment density and compressed code support a tentative 3, pending actual readability review. Relevance: both address the inferred module objective; Claude fits the teaching interfaces and Codex provides stronger duplicate-input performance, but neither is an unqualified drop-in improvement. Verify ratings against the original task.
- **Evidence files:** `experiments/results/e13_ds_comparison.csv` (`integration`, `own_tests`, `neutral_correctness`, `features`, `sorting_comparisons`, `structures_time`, `sorting_time`, `code_metrics`, `verdict`), `experiments/results/plots/e13_ds_comparison.png`. E1 sorting and E2 search provide supporting baseline data, not separate AI scores.


## G15: Generate WP-D modules/picking implementation (Gemini)
Date / member / work package: 25 November 2026 / Raelyn / WP-D
Tool and version: Gemini (Google), Gemini 4 Carbon (Medium)
Prompt (summary or link): "For WP-D, create a Battery.hpp, DockingStation.hpp, PickingRobot.hpp & PickingRobot.cpp, TrafficController.hpp & TrafficController.cpp, CMakeLists.txt and README.md in modules/picking" (with RSE2203-warehouse-main_wo_modules.zip attached).
Output produced: All 8 WP-D files in modules/picking: Battery.hpp, DockingStation.hpp, PickingRobot.hpp/.cpp, TrafficController.hpp/.cpp, CMakeLists.txt, and README.md.
Verification performed:
Built the full project (including FTXUI wh_ui and Warehouse.exe) with MinGW GCC 14.2 and WH_WARNINGS_AS_ERRORS=ON: 0 warnings, 0 errors.
Dependency check (tools/check_dependencies.py): passed (picking only includes core, ds, navigation, picking).
Unit & integration tests (ctest): 6/6 suites passed (test_picking 11/11, test_sim 48-run stress test).
Mutation testing (picking): 3/3 planted mutants caught (0 surprises).
Headless run (Warehouse --headless 80): 80/80 orders completed in 733 ticks with 0 collisions.
Problems identified: Later cross-tool benchmarking (G14) revealed three 2nd-order weaknesses compared with Claude's baseline: TrafficController::move used path.erase(path.begin()) (𝑂(𝐿)O(L) per move) instead of an 𝑂(1)
O(1) index cursor; shelvesLeft() omitted reserve(), causing geometric reallocations; and sidestep() picked uniformly at random without filtering out the blocking robot's path, solving only 28/50 narrow-corridor seeds within 600 ticks (vs. Claude's 50/50).
Changes made: None to the generated source files (kept unmodified for fair cross-tool comparison against Claude's WP-D implementation).
Scores: Accuracy 4/5 · Completeness 5/5 · Clarity 5/5 · Relevance 5/5
Evidence for the scores:
Accuracy: compiled first try with -Werror, passed 6/6 test suites, 3/3 mutants, and 6/6 defensive invariant probes, but lost 1 point because uniform-random sidestep() stalled on 22/50 narrow-corridor seeds in G14.
Completeness: delivered all 8 requested files, full state machine, technician recovery, argument validation, and README hand traces.
Clarity: cleanly structured headers/sources with lecture references and no unused code.
Relevance: matched all project interfaces (JobManager, Simulation, Views, test_picking, and mutation_test.py) without altering other modules.

## G16: Second-order DSA empirical comparison of Claude vs. Gemini modules/picking (CSV + PNG)
Date / member / work package: 25 November 2026 / Raelyn / WP-D
Tool and version: Gemini (Google), Gemini 4 Carbon (High)
Prompt (summary or link): "can you do a comparison of the algorithms in module/picking for these 2 folders and tell me which one is better, the one done by Claude or the one done by Gemini. Compare the algorithms in Data Structure and Algorithm (DSA) 2nd order so time and space complexity. Give me the results in 1 csv file and 1 .png file." (with both Claude and Gemini project archives attached).
Output produced: SHA-256 diff confirming only the 8 modules/picking files differed; standalone C++17 benchmark (bench_picking.cpp) with a global operator new/delete heap profiler; 164-row CSV (picking_claude_vs_gemini.csv); and 6-panel chart (picking_claude_vs_gemini.png).
Verification performed: Compiled both archives in Release (-O3, -Werror), ran 6/6 ctest suites and 3/3 mutation tests on both, executed 6 invariant probes (Gemini 6/6 vs. Claude 0/6), micro-benchmarks B1–B5, 50-seed corridor deadlock tests (Claude 50/50 in 25 ticks vs. Gemini 28/50 in 280 ticks), E5 1–8 robot sweeps (15 seeds), and 3 stress scenarios; visually inspected the PNG chart.
Problems identified:
Initial benchmark binary crashed (0xC0000374) due to libstdc++-6.dll mixing its own operator new with the custom operator delete; fixed by linking with -static.
Two analytical overstatements were later caught in G15: claiming Gemini's path step made stepping "
𝑂(𝐿2) O(L2) vs. 𝑂(1) O(1)" without noting that both versions already scan 𝑂(𝐿) O(L) per tick for obstacles, and reporting >4,500× speedup for unitsOnTrip() where -O3 likely optimised away Gemini's constant loop.
Minor label overlap in Panels B and C of the first PNG render.
Changes made: Added -static to the benchmark build, adjusted PNG label coordinates and re-rendered cleanly, and corrected the two complexity/timing overstatements in G15.
Scores: Accuracy 3/5 · Completeness 5/5 · Clarity 4/5 · Relevance 5/5
Evidence for the scores:
Accuracy: all benchmark builds, tests, and counts ran cleanly once -static was added, but 2 points deducted for the initial DLL heap mismatch and the two overstatements (O(L^2) stepping claim and compiler-optimised unitsOnTrip() speedup).
Completeness: covered static size, dynamic heap churn, micro-benchmarks, corridor deadlocks, E5 scaling, stress runs, and invariant probes in one CSV and one PNG.
Clarity: 6-panel figure and summary tables were readable after fixing two minor label overlaps.
Relevance: directly answered which implementation is better at a 2nd-order DSA level with the requested CSV and PNG outputs.

## G17: Complexity comparison mapped to the 7 identified WP-D algorithms
Date / member / work package: 25 November 2026 / Raelyn / WP-D
Tool and version: Gemini (Google), Gemini 4 Neon (Medium)
Prompt (summary or link): "Using these algorithms identified, do a time and space complexity comparison between claude and gemini"
Output produced: Side-by-side time and space complexity comparison table organized directly by the 7 algorithms (14 sub-operations) from G16, plus overall per-tick totals (𝑂(𝑉+𝑅2+𝑅⋅𝐿+𝑟⋅𝑉log𝑉) O(V+R2+R⋅L+r⋅VlogV) time, 𝑂(𝑉+𝑅⋅𝐿+𝑅(𝑘+𝑢))
O(V+R⋅L+R(k+u)) space) and a 1st-order vs. 2nd-order summary verdict.
Verification performed: Cross-checked all 14 rows against G16, and the source files of both implementations.
Problems identified: None.
Changes made: None.
Scores: Accuracy 5/5 · Completeness 5/5 · Clarity 5/5 · Relevance 5/5
Evidence for the scores: Every row matched the exact 7-algorithm structure from G16 and the verified complexity bounds from G16 without re-introducing overstatements.

| Scenario / metric | Claude | Gemini |
|---|---:|---:|
| Corridor length 7: seeds resolved within 600 ticks | 50/50 | 28/50 |
| Corridor length 7: recorded mean ticks | 25.0 | 279.68 |
| Corridor length 13: seeds resolved within 600 ticks | 50/50 | 28/50 |
| Corridor length 13: recorded mean ticks | 31.0 | 283.04 |
| Heavy-obstacle 200-order stress: mean makespan (ticks) | 1,648.40 | 1,679.93 |
| Heavy-obstacle stress: mean waits | 422.67 | 562.60 |
| 1,000 path queries, path length 256: allocation count (incl. setup) | 1,001 | 1 |
| 1,000 path queries, path length 256: measured time (µs) | 169.3 | 54.7 |

All 15 seeds per recorded stress scenario completed without collisions for both. In contrast, the targeted corridor tests expose Gemini's weaker liveness heuristic; 22/50 trials per corridor length did not resolve within the cutoff. The archived mean ticks are reported as recorded; they must not be interpreted as successful-only means without the missing harness definition.

### E17: theoretical time and space comparison (not an additional AI response)

| Operation | Claude | Gemini |
|---|---|---|
| Advance one cell along a planned path | O(1) cursor increment | O(L) vector front erase |
| Return `plannedPath()` | O(L) time and temporary storage (copy) | O(1) time and extra space (reference) |
| Query `unitsOnTrip()` | O(k) linked-queue scan | O(1) cached count; extra stored state |
| Mark robot cells for a detour | O(V) occupancy scan | O(R) robot scan |
| Path-aware/random side-step | O(L) path-aware filtering | O(1) bounded-neighbour selection |
| Overall per tick (E17 model) | O(V + R² + R·L + r·V log V) | Same asymptotic bound |

Here V is grid cells, R is picking robots, L is remaining path length, k is trip stops, and r is replans in a tick. E17 records the same overall space bound O(V + R·L + R·(k + u)), where u is trip units. Both retain the same first-order complexity; Gemini is cheaper for several queries, while Claude's cursor advancement and more informed side-step rule have benefits. E17 counts 6 Gemini-favoured, 2 Claude-favoured and 6 tied sub-operations by complexity; this is not a correctness or overall quality score. The empirical corridor outcome shows why lower per-operation complexity alone does not establish a better traffic algorithm.

- **Problems identified:** Claude lacks the six extra defensive behaviours measured in E16; Gemini can fail to resolve the corridor scenario within the time limit, and its vector front erase shifts remaining elements. Extremely small cached-count timings may be sensitive to compiler optimisation; do not infer an exact speed-up from zero/near-zero measurements.
- **Changes made / decision:** No adoption of the Gemini alternative is documented. Prefer the path-aware traffic heuristic for congestion handling, while considering Gemini's defensive checks and allocation-saving query implementations individually. Confirm any source changes through the picking and simulation regression suites; no production change is claimed by this log entry.
- **Proposed scores (Claude):** Accuracy 3/5 P · Completeness 3/5 P · Clarity NR · Relevance 4/5 P.
- **Proposed scores (Gemini):** Accuracy 3/5 P · Completeness 4/5 P · Clarity NR · Relevance 4/5 P.
- **Evidence for the proposed scores:** Accuracy: both pass the normal integration suite, but Claude misses defensive behaviours and Gemini's corridor liveness limitation is substantial in a targeted scenario. Completeness: Claude misses six explicit probes; Gemini covers those six but does not consistently resolve a tested congestion case. Clarity cannot be scored defensibly from timing and complexity tables alone. Relevance: both tackle the inferred picking task, but the safety/efficiency trade-offs require review against the original prompt and actual project contracts. These are provisional ratings, not member sign-offs.
- **Evidence files:** `experiments/results/e16_picking_comparison.csv` (`integration`, `robustness_probes`, `corridor_deadlock`, `e5_fleet_scaling`, `stress_scenarios`, `micro_time_B*`, `micro_space_B*`), `experiments/results/e17_algorithms_complexity_comparison.csv`, `experiments/results/plots/e16_picking_comparison.png`, `e17_algorithms_complexity_comparison.png`. E5 fleet-size scaling is supporting baseline evidence, not a separate AI response.

---

## Experiment-to-log traceability

The experiment numbers below follow the CSV filenames in the current archive, not earlier draft numbering. `experiments/experiments.cpp` exposes E1–E10, E14 and E15 through its command-line dispatch. E11, E12, E13 and E16 are archived external comparison results; E17 is an archived analytical complexity table. The external comparison programs are not included, so `experiments all` does not regenerate those five files. CSV and plot evidence is present for all 17 experiment numbers. Experiment numbers and GenAI entry numbers are separate identifiers.

| Experiment | Module / work package | Purpose | GenAI record / evidence status |
|---|---|---|---|
| E1 | Data structures / WP-A | Sorting growth | G1 baseline; G14 comparative support; original specific AI prompt not recorded |
| E2 | Shop / WP-B | Linear vs binary search | G1 baseline; G10 related catalogue cross-check |
| E3 | Navigation / WP-C | BFS vs A* | G4, G5, G9 |
| E4 | Navigation / WP-C | Greedy vs exact routes | G7, G8, G9 related verification |
| E5 | Picking / WP-D | Fleet scaling | G12 and G15 supporting baseline |
| E6 | Fulfilment / WP-E | Routing and robot selection policies | G13 supporting baseline |
| E7 | Fulfilment / WP-E | Trip filling and exact packing | G13 supporting validation; distinct from E11/E12 |
| E8 | Shop / WP-B | Lookup trees and insertion order | Experiment result present; no separate AI evaluation recorded |
| E9 | Shop / WP-B | Substring search | G6 |
| E10 | Navigation / WP-C | Greedy + 2-opt | G7 |
| E11 | Fulfilment / WP-E | Claude vs Codex scheduling | G13 |
| E12 | Fulfilment / WP-E | Claude vs Codex memory | G13 (same variants, additional evidence) |
| E13 | Data structures / WP-A | Claude vs Codex module comparison | G14 |
| E14 | Navigation / WP-C | Held-Karp vs backtracking | G8 |
| E15 | Navigation / WP-C | DistanceTable memoisation | G11 |
| E16 | Picking / WP-D | Claude vs Gemini module comparison | G15; G12 is the distinct Gemini code-review entry |
| E17 | Picking / WP-D | Analytical complexity comparison | G15 (same variants, additional evidence) |

## Summary table (for the report)

| Entry | Member | Tool | Task | Acc. | Comp. | Clar. | Rel. | Principal finding |
|-------|--------|------|------|-----:|------:|------:|-----:|-------------------|
| G1 | Team leader | Claude | Original code base | NR | NR | NR | NR | Current revision builds; 100/100 tests pass; original response not available for scoring |
| G2 | — | — | Historical example only | — | — | — | — | Not an executed evaluation; excluded from totals |
| G3 | Kavin | Claude | Shop checkout test (WP-B) | 4 | 4 | 4 | 5 | No code defects; added missing all-or-nothing checkout test |
| G4 | liyou | ChatGPT | A* cross-check (WP-C) | 4 | 4 | 4 | 4 | 0 mismatches on 100,000 grids; endpoint validation gap in earlier code fixed in G9 |
| G4 | liyou | Claude | A* cross-check (earlier code) | 5 | 4 | 5 | 5 | Original member rating; endpoint validation gap later fixed in G9 |
| G5 | liyou | Claude | Extra tests and mutants (WP-C) | 4 | 4 | 4 | 5 | 3 test gaps closed: 21/21 tests, 0 mutation surprises |
| G6 | Kavin | Claude | E9 search-box experiment (WP-B) | 3 | 4 | 4 | 5 | First version had a suffix-comparison bug caught by the results check; current search is good enough for 91 products |
| G7 | liyou | Claude | E10: greedy + 2-opt vs exact routes | 4 | 4 | 4 | 5 | 2-opt cuts greedy's gap from 25.9% to 10.3% at k=10 in microseconds, but is exact in only 26/100 trips |
| G8 | liyou | Claude | E14: Held-Karp vs backtracking exact routes | 4 | 4 | 4 | 4 | Same optimal routes; Held-Karp 17x faster at k = 8, 68x at k = 10, but trips never exceed 8 shelves, so no change needed |
| G9 | liyou | Claude | Fix F2 + F5 in Pathfinding.cpp | 4 | 4 | 4 | 5 | BFS and A* now agree on blocked goals and reject non-walkable start/goal; 3 tests and 3 mutants added |
| G10 | Kavin | ChatGPT | Catalogue cross-check (WP-B) | 5 | 4 | 4 | 4 | Same results on 91 products; hash map 4-5x faster per lookup but uses more memory; same search-box scan |
| G11 | liyou | Claude | E15: DistanceTable memoisation | 4 | 4 | 4 | 4 | The archive shows about 539x cold and 2,056x warm versus repeated BFS at k = 8 |
| G12 | raelyn | Gemini (Antigravity) | Code review, bug fixes and tests (WP-D) | 4 | 5 | 5 | 5 | Fixed stale blocker after detour, missing longestWait update, and flat-battery isAvailable()/isBusy() bug; 11/11 tests pass |
| G13 | Sam (WP-E owner) | Codex | E11/E12: full-trip assignment | 4 P | 4 P | 4 P | 4 P | Small S1 gains; 2/4 own mutants caught; similar peak memory |
| G13 | Sam (WP-E owner) | Claude | E11/E12: defer charging | 4 P | 4 P | NR | 4 P | 4/5 own mutants caught; modest gains; similar peak memory |
| G14 | Marissa (WP-A owner) | Claude | E13: data structures | 4 P | 4 P | 4 P | 4 P | Neutral tests pass; stronger documentation/tests, weak duplicate quicksort |
| G14 | Marissa (WP-A owner) | Codex | E13: data structures | 3 P | 3 P | 3 P | 4 P | Duplicate-aware quicksort; likely `emplace` defect; missing interfaces |
| G15	| Raelyn | Gemini	| Implement WP-D modules/picking | 4 | 5 | 5 | 5 | Builds with -Werror, passes 6/6 suites & 3/3 mutants; blind random sidestep() stalls on 22/50 narrow-corridor seeds |
| G16	| Raelyn | Gemini | 2nd-order DSA benchmark (Claude vs. Gemini) |	3 | 5 |	4 |	5	| Claude wins traffic/corridor motion (50/50, -28% waits, 1.6x faster); Gemini wins heap churn & 6/6 invariant probes; two complexity/timing overstatements later corrected in G15
| G17	| Raelyn | Gemini |	7-algorithm complexity comparison (Claude vs. Gemini) |	5 |	5 | 5 | 5 |	Mapped all 14 sub-operations cleanly to the 7 WP-D algorithms; consistent with G15 bounds 

**Score status:** Plain numbers are member-recorded evaluations carried forward from the supplied log; `P` denotes a proposed retrospective rating supported by archived evidence but requiring the owner to review and approve it. `NR` means not rateable with the material available (not a zero). A dash denotes not applicable. G4 records two responses, and G12 is distinct from G15. E12 and E17 are supporting analyses, not additional AI responses by themselves. There are 11 recorded historical numerical response evaluations (G3–G12, including both G4 responses), and six retrospective comparative responses (G13–G15). Do not include provisional or unavailable scores in an average of member-recorded ratings.

## Completion and evidence status

| Work package | Owner (root README) | Recorded evidence | Outstanding confirmation |
|---|---|---|---|
| WP-A – data structures | Marissa | G14, E1, E13 | Original prompts, variant source, scores, owner/reviewer sign-off |
| WP-B – shop | Kavin | G3, G6, G10; E2, E8, E9 | E8-specific AI usage not recorded; sign-off not evidenced |
| WP-C – navigation | liyou | G4, G5, G7–G9, G11; E3, E4, E10, E14, E15 | Owner/reviewer sign-off not evidenced |
| WP-D – picking | Raelyn | G12 and G15; E5, E16, E17 | G15 prompt/source/ratings and owner confirmation; sign-off not evidenced |
| WP-E – fulfilment | Sam | G13; E6, E7, E11, E12; 21/21 regression tests; four intended baseline mutants caught when applied | Original comparison prompts and tool versions; missing alternative sources/harnesses; approve proposed scores and resolve clarity NR; update stale mutation-script match; owner/reviewer sign-off |
| WP-F – integration | Calix | G1 baseline, current 100/100 tests, UI build and dependency check; all experiment CSVs | No attributable separate AI prompt/response or evaluation in the supplied log; record it if used, and complete owner/reviewer sign-off |

The project has documented use of more than two AI tools (Claude, ChatGPT/Codex and Gemini), and every experiment E1–E17 is traced above. The log is complete as a record of the supplied evidence, including the latest member additions and the retrospective experiment comparisons. It does not invent original prompts, exact model versions, personal hand traces or missing sign-offs. The external E11–E13/E16 alternative implementations and comparison harnesses are not in the archive; E17 is an analytical result. Those experiments have been audited from their CSVs, not independently rerun. For a final owner-approved submission, recover the original outputs where possible and complete the outstanding confirmations in `VALIDATION_GUIDE.md`. No source code or persistent repository files were changed by this documentation update.
