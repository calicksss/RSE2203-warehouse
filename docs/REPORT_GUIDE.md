# Report, demonstration and submission guide

This document lists the deliverables specified in the project brief [B], which are due before week 13 (22 November 2026), and identifies where the supporting material for each is located in this repository. Citation keys refer to the *References* section of the root `README.md`.

## Assessment components and supporting evidence

The weightings below are those given for the team project in the brief [B].

| Assessment component | Weight | Principal evidence in this repository |
|----------------------|-------:|---------------------------------------|
| Project quality and implementation | 35% | Modular structure, purpose-built data structures, per-module tests, continuous integration, `check_dependencies.py`, mutation tests |
| Report | 30% | See the section-by-section outline below |
| Experiment, analysis and insight discussion | 20% | `experiments/` (E1–E6) CSV files and charts; the v3–v4 comparison in `DESIGN_DECISIONS.md` |
| Presentation and demonstration | 15% | The interactive program (F1–F6), in particular the LIVE and ALGORITHMS screens |

## Structure of the IEEE report

The brief [B] specifies a technical report in IEEE format with the sections below.

| Section | Content | Source material |
|---------|---------|-----------------|
| I. Introduction, background and objectives | The need for fast, fair and collision-free fulfilment of online grocery orders; problem statements P1–P6. | `README.md`, *Problem statements* |
| II. Literature review and state of the art | Automated warehouse systems (for example Amazon/Kiva, Ocado, AutoStore); multi-robot path finding (A\*, cooperative A\*, Conflict-Based Search, PIBT); order picking as a travelling-salesman problem; scheduling heuristics. Sources must be cited, and the choice of the simpler course methods justified. | `DESIGN_DECISIONS.md` §1 and its reference list |
| III. System overview and architecture | Module diagram, class diagram, tick sequence, order life cycle, robot state machine. | `ARCHITECTURE.md`, screenshots |
| IV. Method and approach | One subsection per work package: data structure, algorithm, pseudocode, time complexity and suitability. | Module READMEs (*How it works*) |
| V. Results, analysis and discussion | Tables and charts for E1–E6, each with its experimental set-up (seeds, sizes); comparison of empirical and theoretical behaviour [L3]; greedy versus exact methods [L5b]; congestion as a limit on speed-up; the over-engineering trade-off (v3 versus v4); limitations. | `experiments/results/`, `experiments/README.md`, `DESIGN_DECISIONS.md` §1 and §3 |
| VI. Evaluation of generative AI | Tools used, rubric scores, examples of errors identified, validation method. | `GENAI_LOG.md`, `VALIDATION_GUIDE.md`, mutation-test output |
| VII. Conclusion | Achievements, lessons learnt, future work. | `DESIGN_DECISIONS.md` §4 |

**Figures that can be generated from the repository:**
- charts: `python experiments/plot_results.py`;
- a dashboard frame: `Warehouse --snapshot 150 > frame.txt`, displayed in a terminal and captured as a screenshot;
- architecture diagrams: rendered by GitHub from `ARCHITECTURE.md`, or exported from https://mermaid.live.

## Presentation and demonstration video (15 minutes [B], all six members)

| Time | Presenter | Content |
|------|-----------|---------|
| 0:00–1:30 | WP-F | Problem statement, architecture diagram and module structure |
| 1:30–3:30 | WP-B | Shop: search, binary-search lookup, cart rules, checkout and receipt |
| 3:30–5:30 | WP-A | Purpose-built data structures; E1 and E2 charts; stability illustrated by the packing list |
| 5:30–8:00 | WP-C | BFS versus A\* (E3); greedy versus backtracking route planning (E4); live change of routing method on F6 |
| 8:00–10:00 | WP-D | Live map: obstacle avoidance (key `O`), robots passing one another, charging; E5 |
| 10:00–12:00 | WP-E | Express versus standard orders, sorter bins on F3, packing order on F4; E6 |
| 12:00–14:00 | All | Evaluation of generative AI: errors and over-engineering identified, validation by mutation testing |
| 14:00–15:00 | WP-F | Conclusion and limitations |

Recording note: Windows Terminal at 1920 × 1080, with a font size at which the whole dashboard is visible, gives a legible recording.

## Submission checklist

- [ ] `ctest` passes on a laboratory PC (Windows) as well as on members' own computers.
- [ ] `docs/GENAI_LOG.md` contains entries from every member, covering at least two different tools across the team [B].
- [ ] The sign-off table in `VALIDATION_GUIDE.md` is complete.
- [ ] Owner names are entered in `README.md` and the module READMEs.
- [ ] The software package is named `RSE2203_TeamXY.zip` [B], excludes `build/`, and contains the source files, `third_party/` and `data/`.
- [ ] The report (IEEE template), the video and each member's individual peer-review form are uploaded to X-Site [B].
