# fulfilment: job manager and sorting station (WP-E)

**Owner:** _name_ · **Depends on:** core, ds, shop, navigation, picking · **Used by:** sim, ui

The "brain" that decides who does what, and the sorting robots that turn mixed deliveries back into packed orders.

| File | What | Data structure / algorithm | Big-O |
|------|------|----------------------------|-------|
| `JobManager.hpp/.cpp` | Order backlog, release to sorters, batteries, trip building, robot choice, progress tracking | backlog: **`ds::MinHeap`** (priority queue); records: `std::map` (BST); **greedy** least-loaded sorter, **greedy first-fit** trip filling, **greedy nearest** robot; calls `RoutePlanner` | release O(log n) per order; dispatch O(pool + R·k) per trip + route planning |
| `SortingRobot.hpp/.cpp` | Put-wall sorter: conveyor, bins, packing | conveyor: **`ds::LinkedQueue`**; bins: small `vector` with linear search; packing: **`ds::mergeSort`** (stable, heaviest first) | O(1) per sorted item; O(b) bin lookup; O(m log m) packing |
| `ShelfRegistry.hpp/.cpp` | Product ↔ shelf lookups, checked against the map | two arrays | O(1) |

## How it works (short)

1. **Backlog (priority queue).** `key = submit tick − (express ? 120 : 0)`; the heap always returns the smallest key. An express order overtakes standard orders that arrived up to 120 ticks earlier, but never older ones, so there is **no starvation** (there is a test for each).
2. **Release (greedy load balancing).** The top order goes to the sorter with a free bin and the fewest open orders (ties: fewest waiting items). This is the Lecture 5b "least-loaded processor" idea. Its items become `PickUnit`s in that sorter's pool, kept in urgency order.
3. **Batteries.** A robot on a trip that is close to not reaching any dock goes to charge now (the trip pauses). An idle robot below 30% charges. The nearest *free* dock is chosen and reserved.
4. **Dispatch.**
   - Take the pool with the most urgent first item.
   - Fill a trip greedily (first-fit: up to 8 items and 12 kg, in urgency order).
   - Group the items by shelf.
   - Choose the nearest idle robot.
   - Order the shelves with the RoutePlanner.
   - Check the battery covers the trip plus the way to a dock (a fully charged robot may always start).
5. **Sorting.** Each tick the arm takes the front item of the conveyor and puts it in its order's bin. A full bin is packed for 4 ticks; then `onOrderPacked` reports the packing list, heaviest first.

## Build and test only this module

```bash
cmake --build build --target test_fulfilment
ctest --test-dir build -R fulfilment --output-on-failure
```

## Validate it

### Hand-trace 1: who is released first?

`expressHeadStart = 120`. Orders: #1 standard at t=0, #2 standard at t=5, #3 express at t=10, #4 express at t=200, #5 standard at t=150. Suppose all five are still waiting in the backlog at t=200, and one bin frees up at a time. Compute each heap key and the release order.
<details><summary>Answer</summary>

Keys: #1 = 0, #2 = 5, #3 = −110, #4 = 80, #5 = 150. The release order is **#3, #1, #2, #4, #5**. Express #4 does *not* overtake #1 or #2: they had already waited more than 120 ticks longer.
</details>

### Hand-trace 2: first-fit trip filling

The pool, in urgency order: 7 × rice (2.0 kg), apple (0.2 kg), milk (1.0 kg). Capacity is 8 items and 12 kg. Which items go on the first trip?
<details><summary>Answer</summary>

6 rice = 12.0 kg. The 7th rice (14 kg), the apple (12.2 kg) and the milk (13 kg) do not fit. So 6 items go on the trip and 3 stay in the pool.
</details>

### Hand-trace 3: packing order

The bin receives, in this order: product 10 (1.0 kg), 20 (0.1 kg), 11 (1.0 kg), 30 (2.5 kg), 12 (1.0 kg). Write the packing list. Why must the three 1.0 kg items stay in the order 10, 11, 12? (Test "packing list is heaviest first …".)

### Break it on purpose

`python tools/mutation_test.py fulfilment` plants four bugs:
- the heap serves the least urgent order;
- one item too many per trip;
- always the first sorter;
- packing with quick sort (not stable).

All four are caught. Add your own, for example "the battery check ignores the way back to a dock". Which test fails? If none does, write one: low drain plus long trips.

### Questions you should be able to answer

- Why a heap and not a sorted list for the backlog? What are push and pop in each?
- Why is the express rule "a fixed head start" free of starvation? What rule *would* starve standard orders?
- Greedy first-fit is not optimal packing (bin packing is NP-hard). Give a pool where a different choice would fill the trip better.
- Why does a picker report `onUnitsDelivered(sorterIndex, …)` instead of calling the sorter directly? (See `docs/ARCHITECTURE.md`, avoiding a dependency loop.)
- What does "a fully charged robot may always start" prevent? (Hint: a trip longer than one battery.)

### Is this over-engineered?

- `JobManager` is the largest class. Would splitting it into `Backlog`, `Dispatcher` and `ChargingManager` make it easier to test, or just add files?
- Is the per-sorter pool needed? Could all items wait in one list?
- Is `ShelfRegistry` needed, given `Product` already stores its shelf position?

### Cross-check with a second GenAI tool

Prompt idea: *"Design a priority rule for an order queue where express orders go first but standard orders must not starve. Give the data structure and complexity."* Does the other tool suggest aging, a fixed offset, or two queues? Compare with ours and argue which is simplest.

## Modify it (starter tasks)

1. **Deadline scheduling:** give express orders a deadline (submit + 200) and release by earliest deadline first. Compare lead times in E6.
2. **Opportunistic charging:** idle robots below 60% charge when there is no work. Does it reduce long trips later?
3. **Bin sizing:** make `sorterBins` 2, 4 or 8 and measure the effect on lead time (add a section to E6).

## Experiment you own

- **E6** `experiments policies`: routing (as-given, greedy, backtracking) × robot choice (first, nearest) over 10 seeds. Makespan, lead time and distance. Discuss which decision matters most and why: routing saves about 25% of the distance, robot choice at most about 3%.
