# shop: catalogue, cart, checkout and receipts (WP-B)

**Owner:** _name_ · **Depends on:** core, ds · **Used by:** fulfilment (orders), sim, ui

The storefront. It loads the 91 products, lets a customer fill a cart with checks (stock, weight, positive quantities), and turns the cart into an `Order` at checkout, all-or-nothing. It also writes a receipt and can generate random test orders. It knows nothing about robots or the screen.

| File | What | Data structure / algorithm | Big-O |
|------|------|----------------------------|-------|
| `Money.hpp/.cpp` | Exact money as integer cents | `long long` instead of `double` | O(1) |
| `Catalogue.hpp/.cpp` | Parse `Products.txt`, validate it, look products up | `vector<Product>`; name index sorted with **merge sort** → **binary search**; linear search for partial matches | load O(n²) duplicate check + O(n log n) sort; `findByName` O(log n); `search` O(n·m) |
| `Cart.hpp/.cpp` | Cart lines and the add/remove rules | `std::map<productIndex, qty>` (balanced BST) | O(log n) per line |
| `Order.hpp` | The order handed to the warehouse | plain structs | — |
| `Receipt.hpp/.cpp` | Text receipt, written to `receipts/` | string formatting | O(lines) |
| `Shop.hpp/.cpp` | `checkout()` (validate everything, then change stock) and `generateOrders()` | — | O(lines) |

## How it works (short)

- **Why integer cents?** `0.10 + 0.20` is not exactly `0.30` in binary floating point. A test adds 0.10 a thousand times: `Money` gives exactly $100.00, `double` does not.
- **Binary search by name.** After loading, `byName_` holds the product indices sorted by lower-case name (merge sort, once). `findByName("Milk")` then halves the range each step: about 7 comparisons for 91 products instead of up to 91.
- **All-or-nothing checkout.** First check every line has enough stock; only then take stock, create the order and clear the cart. Nothing changes if any check fails.

## Build and test only this module

```bash
cmake --build build --target test_shop
ctest --test-dir build -R shop --output-on-failure
```

## Validate it

### Hand-trace exercise

Binary search for `"milk"` in the real catalogue's name index (91 names). Which names does it compare with?
<details><summary>Answer (from the real data)</summary>

Honey (mid 45) → Peanut butter (68) → Mayonnaise (56) → Onions (62) → Mozzarella (59) → **Milk** (57). That is 6 comparisons, and ⌊log₂ 91⌋ + 1 = 7 is the worst case.
</details>

Also by hand: what does `Money::parse` return for `"12.5"`, `"12.05"`, `"12."`, `".5"`, `"1.234"`? Check against `Money.cpp` and the tests.

### Break it on purpose

`python tools/mutation_test.py shop` plants three bugs:
- the stock check ignores what is already in the cart;
- `"12.5"` is parsed as 12.05;
- the name index is never sorted.

All three are caught. Add your own, for example "checkout takes stock before checking all lines". Is there a test that catches it? If not, write one.

### Questions you should be able to answer

- Binary search needs a sorted array. Where is it sorted, and with which algorithm? Why does `findByName` lower-case the name?
- Why can't the search box use binary search? (Hint: "ric" should find "Bag of rice".)
- Why is the duplicate check in `Catalogue::parse` O(n²)? Is that a problem for 91 products? For 1,000,000?
- What happens if the receipt file cannot be written? Should the order be cancelled?

### Is this over-engineered?

- With only 91 products, binary search is not measurably faster than a linear scan. Keep it to demonstrate L5a, plus E2 shows it scales, or remove it?
- The comparator in `findByName` calls `toLower()` on every comparison, creating new strings. Would storing the lower-case names once be simpler *and* faster?
- `std::map` for the cart vs a `std::vector<int>` of quantities indexed by product (91 entries): which is simpler?

### Cross-check with a second GenAI tool

Prompt idea: *"In C++, should money be stored as double? Show a safe way to parse '12.50' into cents."* Does the other tool handle "12.5" (one decimal) and reject "1.234"? Score it in `docs/GENAI_LOG.md`.

## Modify it (starter tasks)

1. Precompute lower-case names in the catalogue and measure the difference with a counting test.
2. Add a "discount code" feature to the cart (percentage, never below $0), with tests.
3. Add "sort catalogue by price" to the shop screen using `mergeSort`, keeping equal prices in name order (stability).

## Experiment you own

- **E2** `experiments search`: average and maximum comparisons for binary vs linear search, n = 91 … 1,000,000, against log₂ n. Discuss when the O(n log n) sorting cost pays off.
