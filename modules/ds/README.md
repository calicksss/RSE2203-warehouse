# ds: data structures and sorting/searching algorithms (WP-A)

**Owner:** _name_ · **Depends on:** none · **Used by:** shop, navigation, picking, fulfilment, ui

This module implements the data structures and algorithms of the course as small, general-purpose **templates**. They contain no warehouse-specific code and can therefore be tested on plain `int` values. They are reused by the other modules as listed below. Citation keys refer to the *References* section of the root `README.md`.

| File | Content | Complexity | Course reference | Used by |
|------|---------|------------|------------------|---------|
| `LinkedQueue.hpp` | First-in-first-out queue on a singly linked list with head and tail pointers | enqueue/dequeue O(1) | Queue ADT and singly linked list [L2] | BFS frontier (`navigation`), robot route (`picking`), sorter conveyor (`fulfilment`) |
| `MinHeap.hpp` | Priority queue implemented as a binary heap in a `std::vector` | push/pop O(log n), top O(1) | Abstract data types [L2]; heap sort [L4] | A\* open list (`navigation`), order backlog (`fulfilment`) |
| `Sorting.hpp` | Bubble, selection, insertion, merge and quick sort with a comparator | see the table in the file | [L4] | merge sort: catalogue index, packing list, interface; insertion sort: traffic ordering |
| `Search.hpp` | Binary search, iterative and recursive | O(log n) | [L5a] | product lookup by name (`shop`) |

## How it works

- **LinkedQueue.** Nodes `{data, next}` are allocated with `new`; `dequeue` removes the node at `head_` and `enqueue` appends after `tail_`. Maintaining a tail pointer makes `enqueue` O(1). By contrast, the array-based queue in Lecture 2 keeps the front at index 0 and therefore shifts every element on `dequeue`, which is O(n) [L2]. Because the class owns raw pointers, it follows the **rule of three** (destructor, copy constructor and copy-assignment operator), in line with the constructor/destructor discussion in [L1b].
- **MinHeap.** A complete binary tree stored in an array, with `parent(i) = (i − 1)/2` and children at `2i + 1` and `2i + 2`. `push` appends and sifts up; `pop` moves the last element to the root and sifts down. `isValidHeap()` checks the heap property and is used by the tests.
- **Sorting.** Each function takes a comparator `less(a, b)` that returns true when `a` must precede `b`. Merge sort is **stable** because, on equal keys, it takes the element from the left half. Quick sort uses the partition scheme presented in Lecture 4 (pivot at `a[lo]`, with `pivotIndex` collecting smaller elements) together with the median-of-three pivot recommended there [L4]. It recurses into the smaller partition first, which bounds the stack depth by O(log n).
- **Search.** `compare(element, key)` returns a negative value, zero or a positive value. The midpoint is computed as `lo + (hi − lo)/2` to avoid integer overflow.

## Building and testing this module only

```bash
cmake --build build --target test_ds
ctest --test-dir build -R ds --output-on-failure
./build/modules/ds/test_ds merge          # only tests whose name contains "merge"
```

## Validation

### Hand-trace exercises (to be completed on paper and then checked)

1. **Heap.** Insert 5, 3, 8, 1 and 4 into an empty `MinHeap<int>` and record the array after each insertion; then call `pop()` once.
   <details><summary>Answer</summary>

   `[5]` → `[3,5]` → `[3,5,8]` → `[1,3,8,5]` → `[1,3,8,5,4]`; `pop()` returns 1 and leaves `[3,4,8,5]`.
   </details>
2. **Quick-sort partition** of the array used in the Lecture 4 demonstration, `[8,3,7,11,9,5,13,2,20,1]`, with lo = 0 and hi = 9 [L4]. Apply median-of-three selection (first 8, middle 9, last 1) and then partition. What is the final position of the pivot?
   <details><summary>Answer</summary>

   The median of 8, 9 and 1 is 8, which becomes the pivot. After partitioning, the array is `[2,3,7,1,5,8,13,11,20,9]` and the pivot is at index 5.
   </details>
3. **Stability.** Apply merge sort to `[5, 2a, 4, 2b, 1, 3]`, where 2a and 2b have equal keys. Draw the tree of splits and merges and identify the comparison that keeps 2a before 2b.
4. **Binary search** for 51 in the array from Lecture 5a, `[6,13,14,25,33,43,51,53,64,72]` [L5a]. Record lo, hi and mid at each step. The test "lecture 5a example" checks the result.

### Mutation testing

`python tools/mutation_test.py ds` introduces the following defects:
- merge sort takes from the right half on equal keys (detected by the stability test);
- insertion sort moves past equal elements (detected);
- sift-down ignores the right child (detected);
- `dequeue` does not decrement the size (detected);
- the binary-search loop condition uses `<` instead of `<=` (detected);
- quick sort places elements equal to the pivot in the left partition. This mutant **survives** because it is an *equivalent mutant*: the array is still sorted correctly. The report should explain why.

Each owner adds at least two further mutants, for example "`LinkedQueue::dequeue` does not reset `tail_`". (Which test detects it? Consider building with `-DWH_SANITIZE=ON`.)

### Review questions

- Why does `LinkedQueue` require a user-defined copy constructor, and what would go wrong with the compiler-generated one?
- Why is building a heap by n successive insertions O(n log n)? What is bottom-up heap construction, and why is it O(n)?
- Why is merge sort stable while quick sort and selection sort are not [L4]? Identify the exact line responsible.
- Why does `insertionSort` perform exactly n − 1 comparisons on input that is already sorted [L4]? (A test confirms this.)
- Why is the A\* open list a heap rather than a sorted vector?

### Review for over-engineering (decisions recorded in `docs/DESIGN_DECISIONS.md`)

- `std::queue` and `std::priority_queue` are available. Is a purpose-built implementation justified? (The argument for: it is the subject of the module, and it is tested against the standard library.)
- Bubble sort and selection sort are used only by the experiments. Should they remain in the library or move to `experiments/`?
- The comparator template parameter (`Less less`) makes the code general but harder to read. Would separate `int`-only versions be clearer?

### Cross-check with a second generative AI tool

Suggested prompt: *"Write a binary min-heap priority queue in C++17 using std::vector, with push, pop and top, and give the complexity of each."* Compare the response with `MinHeap.hpp`. Are the index formulas the same? How does it handle `pop()` on an empty heap? Record the scores in `docs/GENAI_LOG.md`.

## Suggested modifications

1. Add `heapSort(std::vector<T>&, Less)` based on `MinHeap` (heap sort is listed among further sorting methods in [L4]) and include it in experiment E1.
2. Add a three-way-partition quick sort (`< pivot`, `= pivot`, `> pivot`) and show in E1 that the "few-unique" case becomes O(n log n).
3. Add `LinkedQueue::reverse()`, listed among additional ADT operations in [L2], together with a test.

## Experiments owned by this work package

- **E1** `experiments sorting`: comparisons and execution time as functions of n for each algorithm and input type (random, sorted, reversed, few-unique). The discussion should compare the empirical results with the theoretical analysis [L3], address best and worst cases, and explain the behaviour of quick sort with many duplicate keys.
- **E2** `experiments search` (shared with WP-B): comparisons for binary and linear search, compared with log₂ n.
