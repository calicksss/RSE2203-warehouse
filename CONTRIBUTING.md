# Contribution guidelines

Six team members contribute to a single repository. The guidelines below are intended to keep the `main` branch in a working state and to ensure that every change is reviewed by a member other than its author. This independent review is the principal safeguard against the inclusion of unvalidated, AI-generated code.

## Initial setup (team leader)

1. Create an **empty, private** repository on GitHub (for example `rse2203-warehouse`) without a README or `.gitignore`.
2. In this folder, run:
   ```bash
   git init -b main            # omit if the folder already contains a .git folder
   git add .
   git commit -m "Initial version"
   git remote add origin https://github.com/<account>/rse2203-warehouse.git
   git push -u origin main
   ```
3. Under **Settings → Collaborators**, invite the other five members.
4. In `.github/CODEOWNERS`, replace the `@wp-…-owner` placeholders with the members' GitHub usernames.
5. Under **Settings → Branches → Add rule**, protect `main` with the following options:
   - "Require a pull request before merging", with one approval;
   - "Require review from Code Owners";
   - "Require status checks to pass", selecting the CI jobs `linux`, `windows` and `sanitizers`.

## Standard workflow (all members)

```bash
git switch main
git pull                                  # start from the latest main
git switch -c wp-c/astar-tie-break        # branch name: wp-<letter>/<short-topic>

# ... edit only the files in your own module folder(s) ...
cmake --build build && ctest --test-dir build      # run ALL tests, not only your own

git add modules/navigation
git commit -m "navigation: prefer smaller h on equal f in A*"
git push -u origin wp-c/astar-tie-break
```

Then open a **pull request** on GitHub and complete the validation checklist in the template. A member of a **different** work package reviews the pull request. It is merged only when continuous integration passes and the reviewer has approved it.

### Rules

- **Changes stay within the author's module.** Where something is needed from another module (a new function or a defect fix), open an issue or contact the module owner rather than editing that module directly.
- **`modules/core` is shared.** Changes to it affect every module and are therefore reviewed by the whole team.
- **Pull requests are kept small.** Each pull request addresses a single concern, ideally with fewer than about 200 changed lines, so that it can be reviewed properly.
- **Build output is never committed.** This includes `build/`, executables and `receipts/`; `.gitignore` already excludes them.
- **Code is accompanied by tests.** Each new function or defect fix includes a test that fails without the change.
- **AI-generated code is recorded.** Any such code is logged in `docs/GENAI_LOG.md` in the same pull request, stating the tool, the prompt, the problems found and the changes made.
- **Commit messages begin with the module name**, for example `shop: reject prices with three decimals`.

### Updating a branch after `main` has changed

```bash
git switch main && git pull
git switch wp-c/astar-tie-break
git merge main          # resolve any conflicts, run the tests, commit
```

## Useful commands

| Task | Command |
|------|---------|
| Build one module and its tests | `cmake --build build --target test_navigation` |
| Run one module's tests | `ctest --test-dir build -R navigation --output-on-failure` |
| Run selected test cases | `./build/modules/navigation/test_navigation "A*"` |
| Build faster without the interface | `cmake -S . -B build -DWH_BUILD_UI=OFF` |
| Check for memory errors (Linux/macOS) | `cmake -S . -B build-asan -DWH_SANITIZE=ON -DWH_BUILD_UI=OFF` |
| Check the module dependency rules | `python tools/check_dependencies.py` |
| Run mutation tests for one module | `python tools/mutation_test.py navigation` |
| Run one experiment | `./build/experiments/experiments routing` |

## Coding conventions

- C++17. Naming: `ClassName`, `functionName`, `memberVariable_`, `kConstant`.
- Each file begins with a comment stating its purpose, the data structure or algorithm it implements, and its time complexity.
- Simple, readable code is preferred to clever code, since each owner must be able to explain their module in the technical interviews.
- No `using namespace std;` in header files and no global variables.
