# ds: data structures and sorting/searching (WP-A)

**Owner:** _name_ · **Depends on:** nothing · **Used by:** shop, navigation, picking, fulfilment, ui

This folder holds the course's data structures and algorithms as small, general **templates**. They know nothing about warehouses, so you can test them on plain `int`s. The other modules reuse them, and every use is listed below.

| File | What | Big-O | Lecture | Used by |
|------|------|-------|---------|---------|
| `LinkedQueue.hpp` | FIFO queue on a singly linked list (head + tail pointer) | enqueue/dequeue O(1) | L2 queue, linked list | BFS frontier (`navigation`), robot route (`picking`), sorter conveyor (`fulfilment`) |
| `MinHeap.hpp` | Priority queue as a binary heap inside a `std::vector` | push/pop O(log n), top O(1) | L2 ADT, L4 (heap sort) | A\* open list (`navigation`), order backlog (`fulfilment`) |
| `Sorting.hpp` | Bubble, selection, insertion, merge, quick sort with a comparator | see table in the file | L4 | merge: catalogue index, packing list, UI; insertion: traffic order |
| `Search.hpp` | Binary search, iterative and recursive | O(log n) | L5a | catalogue name lookup (`shop`) |

## How it works (short)

- **LinkedQueue.** Nodes `{data, next}` are created with `new`. `dequeue` removes at `head_` and `enqueue` adds after `tail_`. Keeping a tail pointer is what makes `enqueue` O(1); the lecture's array queue shifts every element on dequeue, which is O(n). Because the class owns raw pointers it follows the **rule of three**: destructor, copy constructor and copy assignment.
- **MinHeap.** A complete binary tree stored in an array: `parent(i) = (i-1)/2`, children `2i+1` and `2i+2`. `push` appends and sifts up; `pop` moves the last item to the root and sifts down. `isValidHeap()` checks the heap rule and is used by the tests.
- **Sorting.** Every function takes `less(a, b)`, meaning "a must come before b". Merge sort is **stable** because on a tie it takes from the left half. Quick sort uses the lecture's partition (pivot at `a[lo]`, `pivotIndex` collects smaller items) with a median-of-three pivot. It recurses into the smaller half first, so the stack depth stays O(log n).
- **Search.** `compare(element, key)` returns <0, 0 or >0. `mid = lo + (hi - lo) / 2` avoids integer overflow.

## Build and test only this module

```bash
cmake --build build --target test_ds
ctest --test-dir build -R ds --output-on-failure
./build/modules/ds/test_ds merge          # only tests with "merge" in the name
```

## Validate it

### Hand-trace exercises (do them on paper, then check)

1. **Heap.** Push 5, 3, 8, 1, 4 into an empty `MinHeap<int>` and write the array after each push. Then `pop()` once.
   <details><summary>Answer</summary>

   `[5]` → `[3,5]` → `[3,5,8]` → `[1,3,8,5]` → `[1,3,8,5,4]`; `pop()` returns 1 and leaves `[3,4,8,5]`.
   </details>
2. **Quick sort partition** of the lecture array `[8,3,7,11,9,5,13,2,20,1]` (lo = 0, hi = 9). First apply median-of-three (first 8, middle 9, last 1), then partition. Where does the pivot end up?
   <details><summary>Answer</summary>

   The median of 8, 9 and 1 is 8, so the pivot is 8. After partitioning: `[2,3,7,1,5,8,13,11,20,9]`, with the pivot at index 5.
   </details>
3. **Stability.** Merge-sort `[5, 2a, 4, 2b, 1, 3]` (2a and 2b have equal keys). Draw the split/merge tree. Which comparison decides that 2a stays before 2b?
4. **Binary search** for 51 in the L5a array `[6,13,14,25,33,43,51,53,64,72]`: list lo, hi and mid for each step. (The test "lecture 5a example" checks the result.)

### Break it on purpose

`python tools/mutation_test.py ds` plants these bugs:
- merge takes from the right half on ties (caught by the stability test);
- insertion sort passes equal items (caught);
- sift-down ignores the right child (caught);
- dequeue forgets `--size_` (caught);
- binary search loop uses `<` instead of `<=` (caught);
- quick sort puts items equal to the pivot on the left: **survives**, because it is an *equivalent mutant* and still sorts correctly. Explain why in the report.

**Add two of your own**, for example "LinkedQueue::dequeue does not reset `tail_`" (which test catches it? Try building with `-DWH_SANITIZE=ON`).

### Questions you should be able to answer

- Why does `LinkedQueue` need a copy constructor? What goes wrong with the default one?
- Why is building a heap by n pushes O(n log n)? (Bonus: what is "heapify" and why is it O(n)?)
- Why is merge sort stable but quick sort and selection sort are not? Point to the exact line.
- `insertionSort` on already-sorted input makes exactly n−1 comparisons (there is a test). Why?
- Why is the A\* open list a heap and not a sorted vector?

### Is this over-engineered? (decide and record in `docs/DESIGN_DECISIONS.md`)

- `std::queue` and `std::priority_queue` exist. Is writing our own justified? (Our argument: it is the point of a DSA project, and it is tested against the STL.)
- Bubble and selection sort are only used by the experiments. Keep them in the library, or move them into `experiments/`?
- The comparator template parameter (`Less less`) makes the code general but harder to read. Would separate `int`-only versions be clearer?

### Cross-check with a second GenAI tool

Prompt idea: *"Write a binary min-heap priority queue in C++17 using std::vector, with push, pop and top, and give the complexity of each."* Compare it with `MinHeap.hpp`: same index formulas? What does it do on `pop()` of an empty heap? Record the scores in `docs/GENAI_LOG.md`.

## Modify it (starter tasks)

1. Add `heapSort(std::vector<T>&, Less)` using `MinHeap` and add it to experiment E1.
2. Add a 3-way partition quick sort (`< pivot`, `== pivot`, `> pivot`) and show in E1 that the "few-unique" case becomes O(n log n).
3. Add `LinkedQueue::reverse()` (L2 "other operations") with a test.

## Experiments you own

- **E1** `experiments sorting`: comparisons and time vs n for each algorithm and input type (random, sorted, reversed, few-unique). Discuss the empirical results against the theory (Lecture 3), best/worst cases, and why quick sort struggles with duplicates.
- **E2** `experiments search` (shared with WP-B): binary vs linear search comparisons against log₂ n.
