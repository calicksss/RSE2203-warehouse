# ui: terminal interface using FTXUI (WP-F)

**Owner:** integrator (WP-F) · **Depends on:** sim, shop, FTXUI (`third_party/ftxui`, MIT licence) · **Used by:** app

- `Views.hpp/.cpp`: functions that **render** the model as `ftxui::Element` objects. They receive the model by `const&` and therefore cannot modify it.
- `App.hpp/.cpp`: the **interactive** layer, comprising the F1–F6 tabs, buttons, lists, sliders, dialogs and keys, and the 50 ms clock thread that posts `Simulation::step()` to the interface thread.

| Screen | Content |
|--------|---------|
| F1 SHOP | Catalogue (search and categories), product details including the binary-search lookup check, cart summary |
| F2 CART | Cart lines, quantity buttons, checkout (standard or express) with a confirmation dialogue, receipt |
| F3 LIVE | Floor map showing robots, routes and obstacles; fleet, sorters, docks, order book and event log. Keys: SPACE, +/−, S, O, R, P |
| F4 ORDERS | All orders with status, timing and the heaviest-first packing list |
| F5 LOAD | Generation of 1 to 200 random orders |
| F6 ALGORITHMS | Where each data structure and algorithm from the course is used, with live counters; the routing and robot-assignment methods can be changed during a run |

FTXUI elements used: `text`, `hbox`/`vbox` (horizontal and vertical layout), `window` (a bordered box with a title) and `| color(...)`; the components `Menu`, `Button`, `Input`, `Slider`, `Modal` and `Container::Tab`. Examples are available at https://github.com/ArthurSonzogni/FTXUI.

The interface is not assessed as part of the data structures and algorithms content, so changes to it should be kept small. It can be omitted from the build with `-DWH_BUILD_UI=OFF`.
