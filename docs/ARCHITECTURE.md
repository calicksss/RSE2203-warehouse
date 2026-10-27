# Architecture

GitHub renders the diagrams in this document automatically. A diagram is edited by changing the text of its `mermaid` block; changes can be previewed at https://mermaid.live. Citation keys refer to the *References* section of the root `README.md`.

## 1. Modules and the dependency rule

```mermaid
graph TD
    subgraph WP-F["WP-F integration"]
        app["app<br/>main.cpp"]
        ui["ui<br/>App, Views (FTXUI)"]
        sim["sim<br/>Simulation"]
    end
    core["core (WP-F, shared)<br/>Types, Config, EventLog,<br/>Metrics, WorkItem, Robot"]
    ful["fulfilment (WP-E)<br/>JobManager, SortingRobot,<br/>ShelfRegistry"]
    pick["picking (WP-D)<br/>PickingRobot, Battery,<br/>DockingStation, TrafficController"]
    nav["navigation (WP-C)<br/>GridMap, Pathfinding (BFS, A*),<br/>DistanceTable, RoutePlanner, ObstacleManager"]
    shop["shop (WP-B)<br/>Money, Catalogue, Cart,<br/>Order, Receipt, Shop"]
    ds["ds (WP-A)<br/>LinkedQueue, MinHeap,<br/>Sorting, Search"]
    app --> ui --> sim --> ful
    ful --> pick --> nav
    ful --> shop
    nav --> ds
    shop --> ds
    pick --> core
    nav --> core
    shop --> core
```

An arrow denotes "uses". The dependency graph is acyclic, so each module can be built and tested using only the modules beneath it. The `ds` module has no dependencies at all, which allows WP-A to test its containers on plain `int` values without any warehouse code.

Circular dependencies are avoided as follows:

- **Robots and the job manager:** robots report progress through the abstract class `FulfilmentListener` in `core/Robot.hpp`, which `JobManager` implements. No robot class includes `JobManager.hpp`.
- **Picking robots and sorting robots:** a picking robot knows only the drop-off *cell* and the *index* of its sorter. On delivery it calls `listener.onUnitsDelivered(sorterIndex, items)`, and the job manager passes the items to the corresponding sorter.

## 2. Main classes

```mermaid
classDiagram
    class Robot {
        <<abstract>>
        +tick(ctx)*
        +stateLabel()*
        +activity()*
        +isBusy()*
        +id() / name() / cell()
    }
    class PickingRobot {
        -State state_
        -Battery battery_
        -LinkedQueue route_
        -vector cargo_
        +assignTrip(stops, dropCell, sorter, trip)
        +sendToDock(dock)
        +moveTo(cell) / recordWait()
    }
    class SortingRobot {
        -LinkedQueue conveyor_
        -vector bins_
        +openBin(order, items)
        +induct(items)
    }
    class FulfilmentListener {
        <<interface>>
        +onUnitsPicked()*
        +onUnitsDelivered()*
        +onUnitSorted()*
        +onOrderPacked()*
    }
    class JobManager {
        -MinHeap backlog_
        -map records_
        -vector pools_
        -RoutePlanner router_
        +submit(order, now)
        +update(now, pickers, docks)
    }
    class TrafficController {
        -vector plans_
        -vector occupant_
        +step(pickers, obstacles, now, log, metrics)
    }
    class Simulation {
        -unique_ptr pickers (owned)
        -unique_ptr sorters (owned)
        -Robot* fleet_
        +submit(order)
        +step()
    }
    Robot <|-- PickingRobot
    Robot <|-- SortingRobot
    FulfilmentListener <|.. JobManager
    Simulation *-- PickingRobot
    Simulation *-- SortingRobot
    Simulation *-- JobManager
    Simulation *-- TrafficController
    JobManager --> SortingRobot : opens bins, inducts items
    JobManager --> PickingRobot : assigns trips
    TrafficController --> PickingRobot : moves
    PickingRobot ..> FulfilmentListener : reports to
    SortingRobot ..> FulfilmentListener : reports to
```

**Ownership.** `Simulation` owns every robot through `std::unique_ptr`, so robots are released automatically and never move in memory. All other classes hold non-owning raw pointers (`PickingRobot*`), meaning that they use the object without owning it. `fleet_` stores every robot as a `Robot*`, so that a single loop can call `robot->tick()` on both robot types through dynamic dispatch, an application of polymorphism as introduced in Lecture 1b [L1b].

## 3. Sequence of one simulation tick

```mermaid
sequenceDiagram
    participant UI as App (UI thread)
    participant S as Simulation
    participant O as ObstacleManager
    participant J as JobManager
    participant T as TrafficController
    participant P as PickingRobot x8
    participant R as SortingRobot x4
    UI->>S: step()
    S->>O: update(): expire / spawn obstacles
    S->>J: update(): release orders, batteries, dispatch trips
    J->>R: openBin(order)
    J->>P: assignTrip(stops) / sendToDock(dock)
    S->>T: step(): move each picker <= 1 cell
    T->>P: moveTo(cell) or recordWait()
    S->>P: tick(): arrive, pick, drop, charge
    P-->>J: onUnitsPicked / onUnitsDelivered
    J->>R: induct(items)
    S->>R: tick(): sort one item, pack full bins
    R-->>J: onUnitSorted / onOrderPacked
    S->>S: checkInvariants(): no shared cells, no swaps, no walls
```

## 4. Order life cycle

```mermaid
stateDiagram-v2
    [*] --> Queued: checkout, then Simulation submit
    Queued --> Picking: a sorter has a free bin (heap top released)
    Picking --> Sorting: every item picked
    Sorting --> Packing: every item in its bin
    Packing --> Completed: packTicks later, packing list sorted heaviest first
    Completed --> [*]
```

## 5. Picking-robot state machine

```mermaid
stateDiagram-v2
    [*] --> Idle
    Idle --> ToShelf: assignTrip
    Returning --> ToShelf: assignTrip (new trip on the way home)
    ToShelf --> Picking: arrived at the shelf side
    Picking --> ToShelf: more shelves
    Picking --> ToSorter: last shelf done
    ToSorter --> Dropping: arrived at the sorter
    Dropping --> Returning: items handed over
    Returning --> Idle: arrived home
    Idle --> Returning: pushed aside by traffic
    Idle --> ToDock: sendToDock
    ToShelf --> ToDock: battery critical (trip paused)
    ToSorter --> ToDock: battery critical (trip paused)
    Returning --> ToDock: sendToDock
    ToDock --> Charging: arrived at the dock
    Charging --> ToShelf: charged, trip had shelves left
    Charging --> ToSorter: charged, was carrying items
    Charging --> Returning: charged, no trip
```

## 6. Threads (interface only)

The model (`sim` and all modules beneath it) is single-threaded; it neither sleeps nor produces output. The interface runs FTXUI on the main thread. A clock thread wakes every 50 ms and posts a task to the interface thread with `App::Post`; that task calls `Simulation::step()` as many times as the selected speed requires. Because only one thread ever accesses the model, no locks are required.

## 7. Data files

- `data/Map.txt`: one character per cell (`W` wall, `.` floor, `S` shelf, `C` charging dock, `R` sorting robot, `T` picking-robot home); spaces are ignored. `GridMap::parse` reports the line number of any error and uses breadth-first search to verify that every shelf, sorter, dock and home is reachable.
- `data/Products.txt`: `Name Price WeightKg Stock ShelfRow ShelfCol [Category]`. `ShelfRegistry` verifies that every product is located on an `S` cell and that the number of products equals the number of shelves (91).
