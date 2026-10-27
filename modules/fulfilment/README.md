# fulfilment: job manager and sorting station (WP-E)

**Owner:** _name_ · **Depends on:** core, ds, shop, navigation, picking · **Used by:** sim, ui

This module contains the job manager, which decides which robot performs which task, and the sorting robots, which separate mixed deliveries back into individual packed orders. Citation keys refer to the *References* section of the root `README.md`.

| File | Content | Data structure / algorithm | Complexity |
|------|---------|----------------------------|------------|
| `JobManager.hpp/.cpp` | Order backlog, release of orders to sorters, battery management, trip formation, robot selection, progress tracking | Backlog: **`ds::MinHeap`** (priority queue); records: `std::map` (balanced binary search tree); **greedy** least-loaded sorter, **greedy first-fit** trip filling and **greedy nearest** robot [L5b]; calls `RoutePlanner` | release O(log n) per order; dispatch O(pool + R·k) per trip, plus route planning |
| `SortingRobot.hpp/.cpp` | Put-wall sorter: conveyor, bins and packing | Conveyor: **`ds::LinkedQueue`** [L2]; bins: small `vector` searched linearly; packing: **`ds::mergeSort`** (stable, heaviest first) [L4] | O(1) per sorted item; O(b) bin lookup; O(m log m) packing |
| `ShelfRegistry.hpp/.cpp` | Product-to-shelf and shelf-to-product lookup, validated against the map | two arrays | O(1) |

## How it works

1. **Backlog (priority queue).** Each order is keyed by `submit tick − (express ? 120 : 0)`, and the heap always returns the smallest key. An express order therefore overtakes standard orders that arrived up to 120 ticks earlier, but never older ones, so **no order can be postponed indefinitely**. A test covers each of these properties.
2. **Release to a sorter (greedy load balancing).** The order at the top of the heap is assigned to the sorter that has a free bin and the fewest open orders (ties are broken by the fewest waiting items). This is a greedy assignment rule of the kind illustrated by the processor-scheduling example in Lecture 5b [L5b]. The order's items become `PickUnit`s in that sorter's pool, kept in order of urgency.
3. **Battery management.** A robot on a trip that is close to being unable to reach a dock is sent to charge immediately, and its trip is suspended. An idle robot below 30% charge is sent to charge. In both cases the nearest *free* dock is selected and reserved.
4. **Dispatch.**
   - Select the pool whose first item is the most urgent.
   - Fill a trip greedily in order of urgency (first-fit, up to eight items and 12 kg).
   - Group the items by shelf.
   - Select the nearest idle robot.
   - Order the shelves with the `RoutePlanner`.
   - Confirm that the battery covers the trip and the subsequent journey to a dock. A fully charged robot may always start.
5. **Sorting.** In each tick the sorting arm takes the item at the front of the conveyor and places it in the bin of its order. A complete bin is packed for four ticks, after which `onOrderPacked` reports the packing list, heaviest item first.

## Building and testing this module only

```bash
cmake --build build --target test_fulfilment
ctest --test-dir build -R fulfilment --output-on-failure
```

## Validation

### Hand trace 1: order of release

With `expressHeadStart = 120`, the orders are: #1 standard at t = 0, #2 standard at t = 5, #3 express at t = 10, #4 express at t = 200 and #5 standard at t = 150. Assume that all five are still in the backlog at t = 200 and that bins become free one at a time. Compute each heap key and the order of release.
<details><summary>Answer</summary>

Keys: #1 = 0, #2 = 5, #3 = −110, #4 = 80, #5 = 150. The order of release is **#3, #1, #2, #4, #5**. Express order #4 does *not* overtake #1 or #2, because they had already waited more than 120 ticks longer.
</details>

### Hand trace 2: first-fit trip filling

The pool contains, in order of urgency, seven bags of rice (2.0 kg each), one apple (0.2 kg) and one carton of milk (1.0 kg). The capacity is eight items and 12 kg. Which items are placed on the first trip?
<details><summary>Answer</summary>

Six bags of rice (12.0 kg). The seventh bag of rice (14 kg in total), the apple (12.2 kg) and the milk (13 kg) would each exceed the weight limit. Six items are therefore dispatched and three remain in the pool.
</details>

### Hand trace 3: packing order

A bin receives, in this order, product 10 (1.0 kg), 20 (0.1 kg), 11 (1.0 kg), 30 (2.5 kg) and 12 (1.0 kg). Write the packing list, and explain why the three 1.0 kg items must remain in the order 10, 11, 12 (test "packing list is heaviest first …"; stability [L4]).

### Mutation testing

`python tools/mutation_test.py fulfilment` introduces four defects:
- the heap serves the least urgent order first;
- one item too many is placed on a trip;
- orders are always released to the first sorter;
- packing uses quick sort, which is not stable.

All four are detected. Each owner adds further mutants, for example "the battery check ignores the journey back to a dock". If no test fails, one is to be written (low drain combined with long trips).

### Review questions

- Why is a heap used for the backlog rather than a sorted list? What are the costs of insertion and removal in each case?
- Why does a fixed head start for express orders prevent indefinite postponement? Which alternative rule *would* cause standard orders to starve?
- Greedy first-fit does not produce optimal packing (the bin-packing problem is NP-hard). Give a pool for which a different selection would fill the trip better [L5b].
- Why does a picking robot report `onUnitsDelivered(sorterIndex, …)` instead of calling the sorter directly? (See `docs/ARCHITECTURE.md` on avoiding circular dependencies.)
- What problem does the rule "a fully charged robot may always start" prevent? (Consider a trip longer than one battery charge.)

### Review for over-engineering

- `JobManager` is the largest class. Would dividing it into `Backlog`, `Dispatcher` and `ChargingManager` make it easier to test, or merely add files?
- Is a separate pool per sorter necessary, or could all items wait in a single list?
- Is `ShelfRegistry` necessary, given that `Product` already stores its shelf position?

### Cross-check with a second generative AI tool

Suggested prompt: *"Design a priority rule for an order queue where express orders go first but standard orders must not starve. Give the data structure and complexity."* Does the response propose ageing, a fixed offset or two separate queues? Compare it with the present design and assess which is simplest.

## Suggested modifications

1. **Deadline scheduling.** Give express orders a deadline (submission time + 200) and release orders by earliest deadline first; compare lead times in E6.
2. **Opportunistic charging.** Send idle robots below 60% to charge when no work is waiting, and assess whether this reduces interrupted trips later.
3. **Number of bins.** Set `sorterBins` to 2, 4 or 8 and measure the effect on lead time (as an additional part of E6).

## Experiment owned by this work package

- **E6** `experiments policies`: routing method (as given, greedy, backtracking) combined with robot selection (first available, nearest) over ten seeds, measuring completion time, lead time and distance travelled. The discussion should identify which decision has the greatest effect and why: routing reduces the distance by about 25%, whereas robot selection reduces it by at most about 3%.
