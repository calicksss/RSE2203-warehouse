# shop: catalogue, cart, checkout and receipts (WP-B)

**Owner:** _name_ · **Depends on:** core, ds · **Used by:** fulfilment (orders), sim, ui

This module implements the storefront. It loads the 91 products, maintains a cart subject to validation rules (stock, weight and positive quantities), and converts the cart into an `Order` at checkout as a single all-or-nothing operation. It also writes receipts and can generate random test orders. It has no knowledge of the robots or the user interface. Citation keys refer to the *References* section of the root `README.md`.

| File | Content | Data structure / algorithm | Complexity |
|------|---------|----------------------------|------------|
| `Money.hpp/.cpp` | Exact monetary amounts in integer cents | `long long` rather than `double` | O(1) |
| `Catalogue.hpp/.cpp` | Parsing and validation of `Products.txt`; product lookup | `vector<Product>`; name index sorted with **merge sort** and searched with **binary search** [L4], [L5a]; linear search for partial matches | loading: O(n²) duplicate check and O(n log n) sort; `findByName` O(log n); `search` O(n·m) |
| `Cart.hpp/.cpp` | Cart lines and the rules for adding and removing items | `std::map<productIndex, qty>` (balanced binary search tree) | O(log n) per line |
| `Order.hpp` | The order passed to the warehouse | plain structures | — |
| `Receipt.hpp/.cpp` | Text receipt written to `receipts/` | string formatting | O(lines) |
| `Shop.hpp/.cpp` | `checkout()` (validate every line, then update stock) and `generateOrders()` | — | O(lines) |

## How it works

- **Integer cents.** In binary floating point, `0.10 + 0.20` is not exactly `0.30`. A unit test adds 0.10 one thousand times: `Money` yields exactly $100.00, whereas `double` does not.
- **Binary search by name.** After loading, `byName_` holds the product indices sorted by lower-case name (sorted once with merge sort). `findByName("Milk")` halves the search range at each step, requiring about seven comparisons for 91 products instead of up to 91 [L5a].
- **All-or-nothing checkout.** Every line is first checked for sufficient stock; only then is stock deducted, the order created and the cart cleared. If any check fails, nothing changes.

## Building and testing this module only

```bash
cmake --build build --target test_shop
ctest --test-dir build -R shop --output-on-failure
```

## Validation

### Hand-trace exercises

Trace a binary search for `"milk"` in the name index of the actual catalogue (91 names). With which names is the key compared?
<details><summary>Answer (from the actual data)</summary>

Honey (mid = 45) → Peanut butter (68) → Mayonnaise (56) → Onions (62) → Mozzarella (59) → **Milk** (57). The search requires six comparisons; the worst case is ⌊log₂ 91⌋ + 1 = 7.
</details>

Determine by hand the result of `Money::parse` for `"12.5"`, `"12.05"`, `"12."`, `".5"` and `"1.234"`, and check the answers against `Money.cpp` and the tests.

### Mutation testing

`python tools/mutation_test.py shop` introduces three defects:
- the stock check ignores the quantity already in the cart;
- `"12.5"` is parsed as 12.05;
- the name index is never sorted.

All three are detected. Each owner adds further mutants, for example "checkout deducts stock before checking all lines". If no existing test detects it, a test is to be written.

### Review questions

- Binary search requires a sorted array [L5a]. Where is the array sorted, and with which algorithm? Why does `findByName` convert the name to lower case?
- Why can the search field not use binary search? (Consider that "ric" should match "Bag of rice".)
- Why is the duplicate check in `Catalogue::parse` O(n²)? Is this significant for 91 products, and for 1,000,000?
- What happens if the receipt file cannot be written? Should the order then be cancelled?

### Review for over-engineering

- With only 91 products, binary search is not measurably faster than a linear scan. Should it be retained to demonstrate [L5a], given that E2 shows how it scales, or removed?
- The comparator in `findByName` calls `toLower()` on every comparison and so creates new strings. Would storing the lower-case names once be both simpler and faster?
- Would a `std::vector<int>` of quantities indexed by product (91 entries) be simpler than a `std::map` for the cart?

### Cross-check with a second generative AI tool

Suggested prompt: *"In C++, should money be stored as double? Show a safe way to parse '12.50' into cents."* Does the response handle "12.5" (a single decimal digit) and reject "1.234"? Record the scores in `docs/GENAI_LOG.md`.

## Suggested modifications

1. Precompute the lower-case names in the catalogue and measure the effect with a comparison-counting test.
2. Add a discount-code feature to the cart (a percentage, never reducing the total below $0), with tests.
3. Add sorting by price to the shop screen using `mergeSort`, keeping products of equal price in name order (a use of stability [L4]).

## Experiment owned by this work package

- **E2** `experiments search`: mean and maximum comparisons for binary and linear search for n = 91 to 1,000,000, compared with log₂ n. The discussion should consider at what point the O(n log n) cost of sorting is recovered.
