# Architecture

GitHub draws the diagrams below automatically. To edit one, change the text in the `mermaid` block; https://mermaid.live previews it.

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

An arrow means "uses". The arrows never form a loop, so each module can be built and tested with only the modules below it. `ds` uses nothing at all. This is why WP-A can test its queue on plain `int`s without a warehouse.

How the loops are avoided:

- **Robots → JobManager:** robots report progress through the abstract class `FulfilmentListener` in `core/Robot.hpp`, and `JobManager` implements it. A robot never includes `JobManager.hpp`.
- **PickingRobot → SortingRobot:** a picker only knows the drop-off *cell* and the sorter's *number*. On delivery it calls `listener.onUnitsDelivered(sorterIndex, items)`, and the JobManager passes the items to the right sorter.

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

**Ownership:** `Simulation` owns every robot through `std::unique_ptr`, so robots are deleted automatically and never move in memory. Everyone else holds plain pointers (`PickingRobot*`), which means "I use it but do not own it". `fleet_` holds all robots as `Robot*`, so one loop can call `robot->tick()` on both kinds (polymorphism, Lecture 1b).

## 3. One tick

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

## 4. Life of an order

```mermaid
stateDiagram-v2
    [*] --> Queued: checkout, then Simulation submit
    Queued --> Picking: a sorter has a free bin (heap top released)
    Picking --> Sorting: every item picked
    Sorting --> Packing: every item in its bin
    Packing --> Completed: packTicks later, packing list sorted heaviest first
    Completed --> [*]
```

## 5. Picking robot states

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

## 6. Threads (UI only)

The model (`sim` and everything below it) is single-threaded. It never sleeps and never prints. The UI runs FTXUI on the main thread. A small clock thread wakes every 50 ms and posts a job to the UI thread with `App::Post`. That job calls `Simulation::step()` as many times as the chosen speed needs. Only one thread ever touches the model, so no locks are needed.

## 7. Data files

- `data/Map.txt`: one character per cell: `W` wall, `.` floor, `S` shelf, `C` charging dock, `R` sorting robot, `T` picker home. Spaces are ignored. `GridMap::parse` reports the line number of any error and checks with BFS that every shelf, sorter, dock and home can be reached.
- `data/Products.txt`: `Name Price WeightKg Stock ShelfRow ShelfCol [Category]`. `ShelfRegistry` checks that every product sits on an `S` cell and that there are exactly as many products as shelves (91).
