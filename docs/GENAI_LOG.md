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
## Summary table (for the report)

| Entry | Member | Tool | Task | Acc. | Comp. | Clar. | Rel. | Principal finding |
|-------|--------|------|------|-----:|------:|------:|-----:|-------------------|
| G1 | | Claude | Complete code base | | | | | |
| G2 | | | | | | | | |
| G3 | Kavin | Claude | Shop checkout test (WP-B) | 4 | 4 | 4 | 5 | No code defects; added missing all-or-nothing checkout test |
| G4 | liyou | ChatGPT | A* cross-check (WP-C) | 4 | 4 | 4 | 4 | Code correct (0 mismatches on 100,000 grids); validates start/goal, which `aStarPath` does not |
| G5 | liyou | Claude | Extra tests and mutants (WP-C) | 4 | 4 | 4 | 5 | 3 test gaps closed: 21/21 tests, 0 mutation surprises |
| G6 | Kavin | Claude | E9 search-box experiment (WP-B) | 3 | 4 | 4 | 5 | First version had a suffix-comparison bug caught by the results check; current search is good enough for 91 products |
