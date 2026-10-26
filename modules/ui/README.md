# ui: terminal interface with FTXUI (WP-F)

**Owner:** integrator (WP-F) · **Depends on:** sim, shop, FTXUI (`third_party/ftxui`, MIT) · **Used by:** app

- `Views.hpp/.cpp`: functions that **draw** (model → `ftxui::Element`). They take everything by `const&`, so they cannot change the model.
- `App.hpp/.cpp`: the **interactive** part: tabs F1–F6, buttons, lists, sliders, dialogs, keys, and the 50 ms clock thread that posts `Simulation::step()` to the UI thread.

| Screen | What it shows |
|--------|---------------|
| F1 SHOP | Catalogue (search, categories), product detail including the binary-search lookup check, cart summary |
| F2 CART | Cart lines, quantity buttons, checkout (standard / express) with a confirmation dialog, receipt |
| F3 LIVE | Floor map with robots, routes and obstacles; fleet, sorters, docks, order book, event tape. Keys: SPACE, +/−, S, O, R, P |
| F4 ORDERS | Every order with status, timing and the heaviest-first packing list |
| F5 LOAD | Generate 1–200 random orders |
| F6 ALGORITHMS | Where each course topic runs, with live counters; switch the routing and robot-choice algorithms while running |

FTXUI basics used here: `text`, `hbox`/`vbox` (side by side / stacked), `window` (a box with a title), `| color(...)`, and the components `Menu`, `Button`, `Input`, `Slider`, `Modal`, `Container::Tab`. See https://github.com/ArthurSonzogni/FTXUI for examples.

The UI is not part of the data-structures marks, so keep changes small. To build without it, use `-DWH_BUILD_UI=OFF`.
