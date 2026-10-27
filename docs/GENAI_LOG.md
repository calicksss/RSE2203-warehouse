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

## Summary table (for the report)

| Entry | Member | Tool | Task | Acc. | Comp. | Clar. | Rel. | Principal finding |
|-------|--------|------|------|-----:|------:|------:|-----:|-------------------|
| G1 | | Claude | Complete code base | | | | | |
| G2 | | | | | | | | |
