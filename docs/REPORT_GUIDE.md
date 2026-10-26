# Report, demo and submission guide

The brief's deliverables (due before week 13, 22 Nov 2026) and where the material for each one lives in this repository.

## Grading → evidence

| Grade component | Weight | Strongest evidence in this repo |
|-----------------|-------:|---------------------------------|
| Project quality & implementation | 35% | Module structure, own data structures, tests per module, CI, `check_dependencies.py`, mutation tests |
| Report | 30% | Sections below |
| Experiment, analysis & insight | 20% | `experiments/` (E1–E6) CSVs and charts, v3 vs v4 comparison in `DESIGN_DECISIONS.md` |
| Presentation & demo | 15% | The interactive program (F1–F6), especially LIVE and ALGORITHMS |

## IEEE report: section by section

| Section | What to write | Source material |
|---------|---------------|-----------------|
| I. Introduction / objectives | Online grocery orders need fast, fair, collision-free fulfilment. List problem statements P1–P6. | `README.md` → Problem statements |
| II. Literature review / state of the art | Real warehouses (Amazon/Kiva, Ocado, AutoStore); multi-robot path finding (A\*, cooperative A\*, CBS, PIBT); order picking as a TSP; scheduling heuristics. Cite properly. Explain why you chose the simpler course methods. | `DESIGN_DECISIONS.md` §1 |
| III. System overview & architecture | Module diagram, class diagram, tick sequence, order life cycle, robot state machine | `ARCHITECTURE.md` (diagrams), screenshots |
| IV. Method | One subsection per work package: data structure, algorithm, pseudocode, Big-O, why it fits | Module READMEs (each has a "How it works" section) |
| V. Results | Tables and charts E1–E6, each with the setup (seeds, sizes) | `experiments/results/`, `experiments/README.md` |
| VI. Discussion | Empirical vs theoretical (Lecture 3), greedy vs exact (L5b), congestion limits speed-up, over-engineering trade-off (v3 vs v4), limitations | `DESIGN_DECISIONS.md` §1, §3 |
| VII. GenAI evaluation | Tools used, rubric scores, examples of errors found, how you validated | `GENAI_LOG.md`, `VALIDATION_GUIDE.md`, mutation-test output |
| VIII. Conclusion | What works, what you learned, future work | `DESIGN_DECISIONS.md` §4 |

**Figures you can generate:**
- the charts: `python experiments/plot_results.py`;
- a text dashboard picture: `Warehouse --snapshot 150 > frame.txt` (open it in a terminal and take a screenshot);
- architecture diagrams: open `ARCHITECTURE.md` on GitHub and screenshot, or export them from https://mermaid.live.

## Demo video (15 minutes, all six members)

| Time | Who | Content |
|------|-----|---------|
| 0:00–1:30 | WP-F | Problem, architecture diagram, how the modules fit |
| 1:30–3:30 | WP-B | Shop: search, binary search lookup, cart rules, checkout with receipt |
| 3:30–5:30 | WP-A | Own data structures; E1/E2 charts; stability shown with the packing list |
| 5:30–8:00 | WP-C | BFS vs A\* (E3); route planning greedy vs backtracking (E4); switch routing live on F6 |
| 8:00–10:00 | WP-D | Live map: obstacle avoidance (press `O`), robots passing each other, charging; E5 |
| 10:00–12:00 | WP-E | Express vs standard orders, sorter bins on F3, packing order on F4; E6 |
| 12:00–14:00 | All | GenAI evaluation: what was wrong or over-engineered, how you validated (mutation tests) |
| 14:00–15:00 | WP-F | Conclusion and limitations |

Tip: record the terminal at 1920×1080 with Windows Terminal at a font size where the whole dashboard fits.

## Submission checklist

- [ ] `ctest` passes on a lab PC (Windows), not only on your laptop.
- [ ] `docs/GENAI_LOG.md` has entries from every member, and at least two different tools across the team.
- [ ] Validation sign-off table in `VALIDATION_GUIDE.md` is complete.
- [ ] `README.md` owner names are filled in.
- [ ] Zip named `RSE2203_TeamXY.zip`, **without** `build/`, containing sources, `third_party/` and `data/`.
- [ ] Video, report (IEEE template) and individual peer-review form uploaded to X-Site.
