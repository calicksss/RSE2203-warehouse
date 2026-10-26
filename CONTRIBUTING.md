## Everyday workflow (everyone)

```bash
git switch main
git pull                                  # start from the latest main
git switch -c wp-c/astar-tie-break        # branch name: wp-<letter>/<short-topic>

# ... edit only files in YOUR module folder(s) ...
cmake --build build && ctest --test-dir build      # ALL tests, not just yours

git add modules/navigation
git commit -m "navigation: prefer smaller h on equal f in A*"
git push -u origin wp-c/astar-tie-break
```

Then open a **Pull Request** on GitHub. The template asks you to tick the validation checklist. Ask a member from a **different** work package to review. Merge only when CI is green and the reviewer approved.

### Rules

- **Stay in your folder.** If you need something from another module (a new function, a bug fix), open an issue or ask its owner. Do not just edit their files.
- **`modules/core` is shared.** A change there affects everyone, so the whole team reviews it.
- **Small pull requests.** One idea per PR, ideally under ~200 changed lines. Big PRs do not get reviewed properly.
- **Never commit `build/`**, executables or `receipts/`. `.gitignore` already excludes them.
- **Tests come with code.** A new function or a bug fix comes with a test that fails without the change.
- **GenAI-written code** must be logged in `docs/GENAI_LOG.md` in the same PR: which tool, the prompt, what was wrong, and what you changed.
- **Commit messages** start with the module: `shop: reject prices with three decimals`.

### Updating your branch when `main` moved on

```bash
git switch main && git pull
git switch wp-c/astar-tie-break
git merge main          # fix any conflicts, run the tests, commit
```

## Useful commands

| Task | Command |
|------|---------|
| Build only my module + its tests | `cmake --build build --target test_navigation` |
| Run only my module's tests | `ctest --test-dir build -R navigation --output-on-failure` |
| Run only some test cases | `./build/modules/navigation/test_navigation "A*"` |
| Faster builds without the UI | `cmake -S . -B build -DWH_BUILD_UI=OFF` |
| Memory-error checking (Linux/macOS) | `cmake -S . -B build-asan -DWH_SANITIZE=ON -DWH_BUILD_UI=OFF` |
| Check module include rules | `python tools/check_dependencies.py` |
| Plant bugs and see if tests catch them | `python tools/mutation_test.py navigation` |
| My experiment only | `./build/experiments/experiments routing` |

## Code style (keep it consistent with what is there)

- C++17. Names: `ClassName`, `functionName`, `memberVariable_`, `kConstant`.
- Every file starts with a comment saying what it is for, the data structure or algorithm, and its Big-O.
- Prefer simple code over clever code. The person interviewed about it is you.
- No `using namespace std;` in headers. No global variables.
