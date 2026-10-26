"""Mutation testing: break the code on purpose and check that a unit test notices.

Why: a test suite that always passes proves nothing by itself. If we plant a
bug ("mutant") and the tests still pass, either the tests are too weak or the
change did not really matter (an "equivalent mutant"). Both are worth
discussing in the report.

Usage (from the project folder, after one normal build in ./build):
    python tools/mutation_test.py                 # run every mutant
    python tools/mutation_test.py ds navigation   # only mutants of these modules

Each mutant: replace `find` with `replace` in `file` (exactly once), rebuild the
module's test program, run it, then restore the file. Add your own mutants to
the MUTANTS list below.
"""
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = os.path.join(ROOT, "build")

# (module, file, find, replace, what it simulates, expected: "caught" or "equivalent")
MUTANTS = [
    # --- WP-A ds ---------------------------------------------------------------------------------
    ("ds", "modules/ds/Sorting.hpp", "if (less(a[j], a[i])) buffer[k++] = a[j++];", "if (!less(a[i], a[j])) buffer[k++] = a[j++];",
     "merge takes from the RIGHT half on ties (breaks stability)", "caught"),
    ("ds", "modules/ds/Sorting.hpp", "while (j >= 0 && less(key, a[j])) {", "while (j >= 0 && !less(a[j], key)) {",
     "insertion sort shifts past EQUAL items (breaks stability)", "caught"),
    ("ds", "modules/ds/Sorting.hpp", "if (less(a[index], pivot)) {        // belongs in the left half",
     "if (!less(pivot, a[index])) {      // belongs in the left half",
     "quick sort puts items EQUAL to the pivot on the left (still sorts!)", "equivalent"),
    ("ds", "modules/ds/MinHeap.hpp", "if (right < n && data_[right] < data_[smallest]) smallest = right;",
     "if (false && right < n && data_[right] < data_[smallest]) smallest = right;", "heap sift-down ignores the right child", "caught"),
    ("ds", "modules/ds/LinkedQueue.hpp", "        delete old;\n        --size_;", "        delete old;",
     "dequeue forgets to decrease the size", "caught"),
    ("ds", "modules/ds/Search.hpp", "    while (lo <= hi) {", "    while (lo < hi) {",
     "binary search stops one step too early", "caught"),
    # --- WP-B shop -------------------------------------------------------------------------------
    ("shop", "modules/shop/Cart.cpp", "if (already + qty > p.stock)", "if (qty > p.stock)",
     "cart ignores what is already in the cart when checking stock", "caught"),
    ("shop", "modules/shop/Money.cpp", "        if (decimals == 1) fraction *= 10;     // \"12.5\" means 50 cents\n", "",
     "\"12.5\" parsed as 12.05", "caught"),
    ("shop", "modules/shop/Catalogue.cpp", "    mergeSort(cat.byName_,", "    if (false) mergeSort(cat.byName_,",
     "name index never sorted, so binary search runs on unsorted data", "caught"),
    # --- WP-C navigation -------------------------------------------------------------------------
    ("navigation", "modules/navigation/Pathfinding.cpp", "const int h = manhattan(map, next, goal);",
     "const int h = 3 * manhattan(map, next, goal);", "A* heuristic over-estimates (not admissible)", "caught"),
    ("navigation", "modules/navigation/RoutePlanner.cpp", "            s.visited[i] = 0;                    // undo the choice (backtrack)\n", "",
     "backtracking forgets to undo the 'visited' mark", "caught"),
    ("navigation", "modules/navigation/RoutePlanner.cpp", "if (s.prune && newLength + dist_.distance(a, s.end) >= s.best.length) continue;",
     "if (s.prune && newLength >= s.best.length) continue;",
     "weaker pruning bound: still exact, so only the 'far fewer nodes' test notices", "caught"),
    ("navigation", "modules/navigation/GridMap.cpp", "if (inBounds(q) && isWalkable(index(q))) out[n++] = index(q);",
     "if (inBounds(q)) out[n++] = index(q);", "robots may drive through walls and shelves", "caught"),
    # --- WP-D picking ----------------------------------------------------------------------------
    ("picking", "modules/picking/TrafficController.cpp", "if (blocker == -1 && !obstacles.isBlocked(next)) {",
     "if (!obstacles.isBlocked(next)) {", "robots drive into occupied cells (collisions)", "caught"),
    ("picking", "modules/picking/PickingRobot.cpp", "                cargo_.clear();\n", "",
     "robot keeps its cargo after delivering", "caught"),
    ("picking", "modules/picking/TrafficController.cpp", "    if (best == kNoCell) return false;\n    move(parked, other, best, metrics);",
     "    return false;\n    move(parked, other, best, metrics);", "parked robots are never asked to move", "caught"),
    # --- WP-E fulfilment -------------------------------------------------------------------------
    ("fulfilment", "modules/fulfilment/JobManager.hpp", "return key != o.key ? key < o.key : order < o.order;",
     "return key != o.key ? key > o.key : order < o.order;", "backlog heap serves the LEAST urgent order first", "caught"),
    ("fulfilment", "modules/fulfilment/JobManager.cpp", "static_cast<int>(trip.size()) < config_.tripCapacityUnits",
     "static_cast<int>(trip.size()) <= config_.tripCapacityUnits", "one item too many per trip", "caught"),
    ("fulfilment", "modules/fulfilment/JobManager.cpp", "            if (a.activeOrders() < b.activeOrders() || (a.activeOrders() == b.activeOrders() && loadA < loadB))",
     "            if (false && (a.activeOrders() < b.activeOrders() || loadA < loadB))",
     "always the first sorter instead of the least loaded", "caught"),
    ("fulfilment", "modules/fulfilment/SortingRobot.cpp", "    mergeSort(packing,", "    quickSort(packing,",
     "packing with quick sort (not stable)", "caught"),
]


def run(cmd):
    return subprocess.run(cmd, cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=600)


def main():
    only = set(sys.argv[1:])
    if not os.path.isdir(BUILD):
        print("Build the project first:  cmake -S . -B build  &&  cmake --build build")
        return 2
    results = []
    for module, path, find, replace, what, expected in MUTANTS:
        if only and module not in only:
            continue
        full = os.path.join(ROOT, path)
        original = open(full, encoding="utf-8").read()
        if original.count(find) != 1:
            results.append((module, what, "SKIPPED (code changed: text not found exactly once)", expected))
            continue
        try:
            open(full, "w", encoding="utf-8").write(original.replace(find, replace))
            target = "test_" + module
            build = run(["cmake", "--build", "build", "--target", target])
            if build.returncode != 0:
                outcome = "caught (does not compile)"
            else:
                exe = os.path.join(BUILD, "modules", module, target)
                if os.name == "nt" and not os.path.exists(exe):
                    exe = os.path.join(BUILD, "modules", module, "Debug", target + ".exe")
                try:
                    test = subprocess.run([exe], cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=120)
                    outcome = "caught" if test.returncode != 0 else "SURVIVED"
                except subprocess.TimeoutExpired:
                    outcome = "caught (test timed out)"
        finally:
            open(full, "w", encoding="utf-8").write(original)
        results.append((module, what, outcome, expected))
        print(f"[{module:10}] {outcome:28} expected {expected:10}  {what}")

    # Rebuild the real code so ./build is left in a good state.
    for module in sorted({m for m, *_ in MUTANTS if not only or m in only}):
        run(["cmake", "--build", "build", "--target", "test_" + module])

    surprises = [r for r in results if (r[2].startswith("caught")) != (r[3] == "caught")]
    print(f"\n{len(results)} mutants, {len(surprises)} surprise(s).")
    for module, what, outcome, expected in surprises:
        print(f"  SURPRISE [{module}] {what}: {outcome}, expected {expected}")
    return 0 if not surprises else 1


if __name__ == "__main__":
    sys.exit(main())
