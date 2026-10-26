# GenAI usage and evaluation log

The brief requires **at least two GenAI tools** to help generate basic source code, and an evaluation of their responses on **Accuracy, Completeness, Clarity and Relevance**. Record every substantial use here. This file is the raw material for the report's GenAI section.

How to fill it in: one entry per question or task. Keep the prompt short (or link to a shared chat). Scores use the rubric in `VALIDATION_GUIDE.md`, and **each score needs evidence**: a test name, a line number or a counter-example.

---

## Entry template

```
### G<number>: <short title>
- Date / member / work package:
- Tool + version:            (e.g. ChatGPT GPT-5, GitHub Copilot, Claude, Gemini)
- Prompt (summary or link):
- What it produced:
- What we checked:           (tests run, hand trace, compared with tool X, mutation test)
- Problems found:
- What we changed:
- Scores:  Accuracy _/5 · Completeness _/5 · Clarity _/5 · Relevance _/5
- Evidence for the scores:
```

---

## G1: First version of the whole code base (baseline, needs scoring by the team)

- **Date / member / WP:** 26 Oct 2026 / team leader / all
- **Tool:** Claude (Anthropic), used as an agent that wrote and ran code.
- **Prompt (summary):** "Use the current iteration of the warehouse program and cater it to our RSE2203 course level (lecture slides 1a–5b and the team project brief attached). Teammates must be able to understand, validate and modify it. Split it into separate parts for six members on GitHub."
- **What it produced:**
  - 8 modules with tests (74 test cases in 6 test programs);
  - an experiments program and a chart script;
  - documentation;
  - the GitHub set-up.
- **What the tool reported about its own checks:**
  - builds with warnings as errors;
  - all module tests pass;
  - a 360-run stress test finished every order with 0 collisions;
  - A\* path lengths match BFS;
  - backtracking routes match a brute-force search;
  - 20 planted bugs were all detected or explained.

  **These are the tool's claims. Re-run them yourselves** (`ctest`, `tools/mutation_test.py`, `experiments`) before you rely on them.
- **Known limitations it stated itself:** see `DESIGN_DECISIONS.md` §3 (the traffic rules are a heuristic with no proof that every jam ends; the route planner ignores other robots; etc.).
- **What we checked:** _(each owner adds their module's steps 1–7 here)_
- **Problems found:** _(fill in)_
- **What we changed:** _(fill in)_
- **Scores:** Accuracy _/5 · Completeness _/5 · Clarity _/5 · Relevance _/5
- **Evidence:** _(fill in)_

## G2: Second tool, cross-check of _(function)_ (example: WP-A, merge sort)

- **Tool:** _(e.g. ChatGPT)_
- **Prompt:** "Write a stable merge sort for std::vector in C++17 with a comparator, and state its time and space complexity."
- **Comparison with `modules/ds/Sorting.hpp`:** _(same algorithm? ties handled the same way, i.e. stable? complexity claims?)_
- **Test used as evidence:** `Sorting: merge, insertion and bubble sort are stable` (+ mutation "merge takes from the RIGHT half on ties")
- **Scores (tool 2):** Accuracy _/5 · Completeness _/5 · Clarity _/5 · Relevance _/5
- **Scores (Claude, same question):** Accuracy _/5 · Completeness _/5 · Clarity _/5 · Relevance _/5

_(Add G3, G4, … for each member. Aim for at least one entry per member, with at least two different tools across the team.)_

---

## Summary table (for the report)

| Entry | Member | Tool | Task | Acc | Comp | Clar | Rel | Main finding |
|-------|--------|------|------|----:|-----:|-----:|----:|--------------|
| G1 | | Claude | Whole code base | | | | | |
| G2 | | | | | | | | |
