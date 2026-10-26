# Validation guide

The brief allows GenAI for generating code, but **the output has to be validated**. Graders will also ask you about your module in the demo and the report. This guide turns "validate it" into concrete steps that leave evidence behind.

**Golden rule:** code you cannot explain is not validated, even if every test passes. Tests written by the same AI that wrote the code can share its blind spots.

---

## Who does what

| Role | Responsibility |
|------|----------------|
| **Owner** (one per work package) | Runs steps 1–7 below for their module and fills in the sign-off table at the end. |
| **Reviewer** (a member of a *different* work package) | Reviews the owner's pull requests, re-runs the tests, and tries one hand trace independently. |
| **Integrator** (WP-F) | Keeps `main` green and runs the whole-system tests and experiments before the report is written. |

## Suggested timeline (submission before 22 November 2026)

| Week | Goal |
|------|------|
| 27 Oct – 2 Nov | Everyone builds the project and validates their module (steps 1–7), logging findings in `GENAI_LOG.md`. |
| 3 – 9 Nov | Fix what you found, simplify what is over-engineered, extend one thing, run your experiment. |
| 10 – 16 Nov | Write your report section (method + results + discussion) and record your demo segment. |
| 17 – 21 Nov | Integrate the report and video; final test run on a lab PC; zip and submit. |

---

## The seven steps (for each module)

### 1. Read and explain

Open every file in your module folder. For each function, write one sentence in your own words: what it does, why it is needed, and what its Big-O is. Compare that with the comment in the code.
- Is the comment **correct**? AI comments often claim a complexity the code does not have.
- Is anything **unused**? `grep` for the function name across `modules/`.
- *Evidence:* your notes, or a short "explained by …" line in your module README.

### 2. Run the tests, then read them

```bash
ctest --test-dir build -R <module> --output-on-failure
./build/modules/<module>/test_<module>            # see each test by name
```

For each `TEST_CASE`, ask what claim it proves and whether it would fail if the claim were false. List every claim in the header comments, such as "stable" or "never two robots in one cell". Mark each one *tested* or *not tested*, and add tests for the gaps.

### 3. Hand-trace a small example

Each module README has a small example: a 6-item array, the 6×7 tiny map, a 3-shelf route. Work it out on paper first. Then check your answer with a debugger breakpoint, a temporary `std::cout`, or the existing test. This is the best preparation for the technical interviews.

### 4. Break it on purpose (mutation testing)

```bash
python tools/mutation_test.py <module>
```

The script plants known bugs one at a time and reports whether a test caught each one. Two outcomes deserve a sentence in the report:
- **SURVIVED but the program is now wrong.** The tests are too weak, so add a test.
- **SURVIVED but the program is still right.** This is an *equivalent mutant*. Example: quick sort putting equal items on the left still sorts correctly. Explain why.

Then add **two mutants of your own** to the list in `tools/mutation_test.py`.

### 5. Cross-check with a second GenAI tool (≥ 2 tools are required by the brief)

Pick one core function of your module. Ask a *different* tool (ChatGPT, Copilot, Gemini …) to write or explain it, without showing it our code first. Then compare:
- Did both give the same algorithm? Same complexity? Same edge cases?
- Where they differ, which one is right? Prove it with a test.
- Score both answers with the rubric below and record them in `docs/GENAI_LOG.md`.

### 6. Look for over-engineering

For each class or feature, ask:

| Question | If "no" … |
|----------|-----------|
| Is it needed to meet the brief or one of our problem statements (README)? | Remove it or move it to "future work". |
| Is it actually used? (search for callers) | Remove it. |
| Would a simpler data structure do? (an array instead of a hash map, a vector instead of a list) | Use the simpler one, or write down why not. |
| Can you explain it in 2 minutes at the interview? | Simplify it until you can. |
| Is the extra complexity measurable in an experiment? | Measure it, or drop it. |

Each module README lists debatable spots under "Is this over-engineered?". Decide, and record the decision in `docs/DESIGN_DECISIONS.md`.

### 7. Check the logic (sanity checklist)

- [ ] **Edge cases:** empty input, one element, duplicates, the maximum size, unreachable goals.
- [ ] **Off-by-one:** `<` vs `<=` in loops and capacity checks; first and last element.
- [ ] **Invariants:** what must always be true (heap rule, "one robot per cell", "every item has a bin")? Is it checked in a test?
- [ ] **Ownership:** who `new`s and who `delete`s? Can a pointer outlive what it points to?
- [ ] **Units and overflow:** ticks vs seconds, kg vs units, cents vs dollars; can an `int` overflow?
- [ ] **Determinism:** same seed → same result? (Experiments depend on it.)
- [ ] **Complexity claims:** does an experiment or a counting test back up the Big-O in the comment?

---

## GenAI evaluation rubric (the brief's four criteria)

Score each AI answer from 1 to 5 and give **evidence**: a test name, a line number, or a counter-example.

| Score | Accuracy *(is it correct?)* | Completeness *(anything missing?)* | Clarity *(easy to understand?)* | Relevance *(on topic?)* |
|------:|------------|--------------|---------|-----------|
| 5 | No errors found after tests and a hand trace | Handles every edge case we know of | A classmate understands it on first read | Answers exactly what was asked |
| 4 | Minor slip (comment or naming), logic correct | One minor case missing | Needs one re-read | Small detours |
| 3 | A real bug in an uncommon case | Several cases missing | Needs explanation from someone else | Partly off-topic or over-engineered |
| 2 | Wrong in common cases | Major parts missing | Confusing | Mostly a different problem |
| 1 | Does not work / does not compile | Skeleton only | Unreadable | Unrelated |

Typical GenAI failure modes to look for: invented complexity claims; code that is correct but far beyond the course level (over-engineering); tests that only check the "happy path"; comments that describe what the code *should* do rather than what it does; outdated library APIs.

---

## Sign-off (fill in as you go)

| Module | Owner | Steps 1–7 done | Mutants added | 2nd GenAI tool used | Findings logged | Reviewer |
|--------|-------|----------------|---------------|---------------------|-----------------|----------|
| ds (WP-A) | | | | | | |
| shop (WP-B) | | | | | | |
| navigation (WP-C) | | | | | | |
| picking (WP-D) | | | | | | |
| fulfilment (WP-E) | | | | | | |
| core / sim / ui (WP-F) | | | | | | |
