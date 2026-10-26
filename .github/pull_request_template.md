## What changed

<!-- One or two sentences. Link the issue if there is one: "Closes #12". -->

## Work package

- [ ] WP-A ds
- [ ] WP-B shop
- [ ] WP-C navigation
- [ ] WP-D picking
- [ ] WP-E fulfilment
- [ ] WP-F integration

## Was GenAI used for this change?

- [ ] No
- [ ] Yes. Tool(s): ______ . I added an entry to `docs/GENAI_LOG.md` (prompt, what was wrong, what I changed).

## Validation (the author ticks these, the reviewer checks them)

- [ ] I can explain every line I changed without looking at the AI's answer.
- [ ] `ctest --test-dir build` passes on my machine (all modules, not just mine).
- [ ] I added or updated a test that would FAIL if my change were wrong.
- [ ] I tried at least one "break it on purpose" change (see my module's README) and a test caught it.
- [ ] Complexity (Big-O) in the comments/README is still correct.
- [ ] I checked for over-engineering: is there a simpler way that meets the brief?
- [ ] `python tools/check_dependencies.py` passes (my module only includes modules it may use).

## Reviewer (from a DIFFERENT work package)

- [ ] I read the code and the test, and I understand both.
- [ ] I ran the tests myself.
- Questions / concerns:
