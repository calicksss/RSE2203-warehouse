/**
 * @file test_fulfilment.cpp
 * @brief Tests for the fulfilment module: shelf registry, sorting robot (put wall + packing), job manager.
 */
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "TestFramework.hpp"
#include "fulfilment/JobManager.hpp"
#include "fulfilment/ShelfRegistry.hpp"
#include "fulfilment/SortingRobot.hpp"

using namespace wh;

namespace {

struct PackedListener : FulfilmentListener {
    std::vector<OrderId> packedOrders;
    std::vector<std::vector<PickUnit>> packingLists;
    int sorted = 0;
    void onUnitsPicked(const std::vector<PickUnit>&, Tick) override {}
    void onUnitsDelivered(int, const std::vector<PickUnit>&, Tick) override {}
    void onUnitSorted(const PickUnit&, Tick) override { ++sorted; }
    void onOrderPacked(OrderId order, const std::vector<PickUnit>& list, int, Tick) override
    {
        packedOrders.push_back(order);
        packingLists.push_back(list);
    }
};

PickUnit unit(OrderId order, int product, double kg) { return {order, product, kNoCell, kg, 0}; }

/// Real map + catalogue + 4 sorters + docks + pickers, wired like the Simulation does it.
struct Warehouse {
    SimConfig config;
    GridMap map = GridMap::loadFromFile(std::string(WH_DATA_DIR) + "/Map.txt");
    Catalogue catalogue = Catalogue::loadFromFile(std::string(WH_DATA_DIR) + "/Products.txt");
    ShelfRegistry shelves{map, catalogue};
    DistanceTable distances{map};
    EventLog log;
    Metrics metrics;
    std::vector<std::unique_ptr<SortingRobot>> sorterStore;
    std::vector<SortingRobot*> sorters;
    std::vector<std::unique_ptr<PickingRobot>> pickerStore;
    std::vector<PickingRobot*> pickers;
    std::vector<DockingStation> docks;
    std::unique_ptr<JobManager> jobs;

    explicit Warehouse(int sorterCount = 4, int bins = 4, int pickerCount = 8, double battery = 100.0)
    {
        config.sorterBins = bins;
        for (int s = 0; s < sorterCount; ++s) {
            const CellIndex cell = map.sorters()[static_cast<std::size_t>(s)];
            sorterStore.push_back(std::make_unique<SortingRobot>(s + 1, s, cell, map.inductionCellFor(cell), bins));
            sorters.push_back(sorterStore.back().get());
        }
        for (int p = 0; p < pickerCount; ++p) {
            pickerStore.push_back(std::make_unique<PickingRobot>(100 + p, p + 1, map.homes()[static_cast<std::size_t>(p)], battery));
            pickers.push_back(pickerStore.back().get());
        }
        for (std::size_t d = 0; d < map.docks().size(); ++d) docks.emplace_back(static_cast<int>(d), map.docks()[d], config.chargePerTick);
        jobs = std::make_unique<JobManager>(config, catalogue, shelves, map, distances, sorters, log, metrics);
    }

    Order makeOrder(OrderId id, Priority priority, const std::vector<std::pair<std::string, int>>& items)
    {
        Order o;
        o.id = id;
        o.priority = priority;
        for (const auto& item : items) {
            const int idx = catalogue.findByName(item.first);
            if (idx < 0) throw std::runtime_error("unknown product in test: " + item.first);
            o.lines.push_back({idx, item.first, item.second, catalogue.at(idx).price, catalogue.at(idx).weightKg});
        }
        return o;
    }
};

} // namespace

// ---------------------------------------------------------------------------
// ShelfRegistry
// ---------------------------------------------------------------------------

TEST_CASE("ShelfRegistry: the real data has one product on every shelf")
{
    Warehouse w;
    for (int i = 0; i < w.catalogue.size(); ++i) {
        const CellIndex cell = w.shelves.shelfOf(i);
        CHECK(w.map.tile(cell) == Tile::Shelf);
        CHECK(w.shelves.productAt(cell) == i);
    }
}

TEST_CASE("ShelfRegistry: rejects a product that is not on a shelf, and a product/shelf count mismatch")
{
    Warehouse w;
    std::istringstream notOnShelf("Apple 1.50 0.20 10 1 3 Produce\n");   // (1,3) is floor
    CHECK_THROWS(ShelfRegistry(w.map, Catalogue::parse(notOnShelf)));
    std::istringstream tooFew("Apple 1.50 0.20 10 5 4 Produce\n");       // a real shelf, but only 1 product
    CHECK_THROWS(ShelfRegistry(w.map, Catalogue::parse(tooFew)));
}

// ---------------------------------------------------------------------------
// SortingRobot
// ---------------------------------------------------------------------------

TEST_CASE("SortingRobot: separates mixed deliveries into the right bins and packs complete orders")
{
    SimConfig config;
    EventLog log;
    PackedListener listener;
    SortingRobot sorter(1, 0, 0, 1, 2);
    sorter.openBin(7, 2);
    sorter.openBin(8, 3);
    CHECK(sorter.freeBins() == 0);
    CHECK_THROWS(sorter.openBin(9, 1));   // no free bin

    // One delivery with items of BOTH orders, interleaved.
    sorter.induct({unit(8, 1, 0.5), unit(7, 2, 1.0), unit(8, 3, 2.0), unit(7, 4, 0.2), unit(8, 5, 1.0)});
    CHECK(sorter.queuedUnits() == 5);
    for (Tick t = 1; t <= 20; ++t) {
        TickContext ctx{t, config, log, listener};
        sorter.tick(ctx);
    }
    CHECK(listener.sorted == 5);
    CHECK(listener.packedOrders.size() == 2);
    CHECK(sorter.freeBins() == 2);   // bins are free again after packing
    for (std::size_t k = 0; k < listener.packedOrders.size(); ++k)
        for (const PickUnit& u : listener.packingLists[k]) CHECK(u.order == listener.packedOrders[k]);
}

TEST_CASE("SortingRobot: packing list is heaviest first, and equal weights keep their arrival order (stable)")
{
    SimConfig config;
    EventLog log;
    PackedListener listener;
    SortingRobot sorter(1, 0, 0, 1, 1);
    sorter.openBin(1, 5);
    // products 10, 11, 12 all weigh 1.0 kg and arrive in that order
    sorter.induct({unit(1, 10, 1.0), unit(1, 20, 0.1), unit(1, 11, 1.0), unit(1, 30, 2.5), unit(1, 12, 1.0)});
    for (Tick t = 1; t <= 20; ++t) {
        TickContext ctx{t, config, log, listener};
        sorter.tick(ctx);
    }
    REQUIRE(listener.packingLists.size() == 1);
    const std::vector<PickUnit>& list = listener.packingLists[0];
    REQUIRE(list.size() == 5);
    CHECK(list[0].productIndex == 30);   // 2.5 kg
    CHECK(list[1].productIndex == 10);   // three 1.0 kg items, in arrival order
    CHECK(list[2].productIndex == 11);
    CHECK(list[3].productIndex == 12);
    CHECK(list[4].productIndex == 20);   // 0.1 kg last
}

TEST_CASE("SortingRobot: an item for an order without a bin is a logic error")
{
    SimConfig config;
    EventLog log;
    PackedListener listener;
    SortingRobot sorter(1, 0, 0, 1, 1);
    sorter.induct({unit(99, 1, 1.0)});
    TickContext ctx{1, config, log, listener};
    CHECK_THROWS(sorter.tick(ctx));
}

// ---------------------------------------------------------------------------
// JobManager: release (priority queue + least-loaded sorter)
// ---------------------------------------------------------------------------

TEST_CASE("JobManager: an express order overtakes standard orders that arrived a little earlier")
{
    Warehouse w(1, 1, 0);   // ONE sorter with ONE bin, no pickers: only one order can be released
    w.jobs->submit(w.makeOrder(1, Priority::Standard, {{"Milk", 1}}), 0);
    w.jobs->submit(w.makeOrder(2, Priority::Standard, {{"Eggs", 1}}), 5);
    w.jobs->submit(w.makeOrder(3, Priority::Express, {{"Coffee", 1}}), 10);
    w.jobs->update(10, w.pickers, w.docks);
    CHECK(w.jobs->record(3)->status == OrderStatus::Picking);
    CHECK(w.jobs->record(1)->status == OrderStatus::Queued);
    CHECK(w.jobs->record(2)->status == OrderStatus::Queued);
    CHECK(w.jobs->backlogSize() == 2);
}

TEST_CASE("JobManager: a standard order that waited long enough is NOT overtaken (no starvation)")
{
    Warehouse w(1, 1, 0);
    w.jobs->submit(w.makeOrder(1, Priority::Standard, {{"Milk", 1}}), 0);
    w.jobs->submit(w.makeOrder(2, Priority::Express, {{"Coffee", 1}}), 200);   // key 200 - 120 = 80 > 0
    w.jobs->update(200, w.pickers, w.docks);
    CHECK(w.jobs->record(1)->status == OrderStatus::Picking);
    CHECK(w.jobs->record(2)->status == OrderStatus::Queued);
}

TEST_CASE("JobManager: orders are spread over the least-loaded sorters")
{
    Warehouse w(4, 4, 0);
    for (int id = 1; id <= 4; ++id) w.jobs->submit(w.makeOrder(id, Priority::Standard, {{"Milk", 1}}), 0);
    w.jobs->update(1, w.pickers, w.docks);
    for (SortingRobot* s : w.sorters) CHECK(s->activeOrders() == 1);   // one each, not four on SRT-1
}

TEST_CASE("JobManager: invalid submissions are ignored")
{
    Warehouse w(1, 1, 0);
    Order empty;
    empty.id = 5;
    w.jobs->submit(empty, 0);
    CHECK(w.jobs->record(5) == nullptr);
    w.jobs->submit(w.makeOrder(6, Priority::Standard, {{"Milk", 1}}), 0);
    w.jobs->submit(w.makeOrder(6, Priority::Standard, {{"Milk", 2}}), 0);   // duplicate id
    CHECK(w.jobs->orderIds().size() == 1);
    CHECK(w.jobs->openOrders() == 1);
}

// ---------------------------------------------------------------------------
// JobManager: dispatch (batching + robot choice + battery)
// ---------------------------------------------------------------------------

TEST_CASE("JobManager: trips never exceed the item or weight capacity")
{
    Warehouse w(4, 4, 8);
    // 12 bags of rice = 24 kg: needs at least 2 trips at 12 kg each.
    w.jobs->submit(w.makeOrder(1, Priority::Standard, {{"Bag of rice", 12}}), 0);
    w.jobs->submit(w.makeOrder(2, Priority::Standard, {{"Apple", 10}, {"Banana", 5}}), 0);
    w.jobs->update(1, w.pickers, w.docks);
    int carried = 0;
    for (PickingRobot* r : w.pickers) {
        CHECK(r->unitsOnTrip() <= w.config.tripCapacityUnits);
        carried += r->unitsOnTrip();
    }
    CHECK(carried + w.jobs->pendingUnits() == 27);   // every item is on a robot or still waiting
    CHECK(w.metrics.tripsDispatched >= 3);
}

TEST_CASE("JobManager: 'Nearest' gives the trip to the closest idle robot, 'FirstAvailable' to robot 1")
{
    for (AssignmentMethod method : {AssignmentMethod::Nearest, AssignmentMethod::FirstAvailable}) {
        Warehouse w(4, 4, 8);
        w.config.assignment = method;
        // Coffee is on the right-hand side of the map, close to the robots whose home is on the right.
        w.jobs->submit(w.makeOrder(1, Priority::Standard, {{"Coffee", 1}}), 0);
        w.jobs->update(1, w.pickers, w.docks);
        int chosen = -1;
        for (std::size_t i = 0; i < w.pickers.size(); ++i)
            if (w.pickers[i]->hasWork()) chosen = static_cast<int>(i);
        REQUIRE(chosen >= 0);

        const CellIndex shelf = w.shelves.shelfOf(w.catalogue.findByName("Coffee"));
        auto distanceOf = [&](int i) {
            int best = kUnreachable;
            for (CellIndex a : w.map.accessCells(shelf))
                best = std::min(best, w.distances.distance(w.pickers[static_cast<std::size_t>(i)]->cell(), a));
            return best;
        };
        if (method == AssignmentMethod::FirstAvailable) {
            CHECK(chosen == 0);
        } else {
            for (int i = 0; i < 8; ++i) CHECK(distanceOf(chosen) <= distanceOf(i));
        }
    }
}

TEST_CASE("JobManager: robots with low battery are sent to charge instead of getting a trip")
{
    Warehouse w(4, 4, 2, 20.0);   // two pickers at 20 % (below lowBattery = 30 %)
    w.jobs->submit(w.makeOrder(1, Priority::Standard, {{"Milk", 1}}), 0);
    w.jobs->update(1, w.pickers, w.docks);
    for (PickingRobot* r : w.pickers) {
        CHECK(r->isCharging());
        CHECK(!r->hasWork());
    }
    CHECK(w.jobs->pendingUnits() == 1);   // the item waits for a charged robot
    int reserved = 0;
    for (const DockingStation& d : w.docks) reserved += d.isFree() ? 0 : 1;
    CHECK(reserved == 2);
}

TEST_CASE("JobManager: a trip's shelves are visited from a real access cell and end at the right sorter")
{
    Warehouse w(4, 4, 1);
    w.jobs->submit(w.makeOrder(1, Priority::Standard, {{"Milk", 1}, {"Eggs", 1}, {"Coffee", 1}}), 0);
    w.jobs->update(1, w.pickers, w.docks);
    PickingRobot& r = *w.pickers[0];
    REQUIRE(r.hasWork());
    CHECK(r.stopsLeft() == 3);
    CHECK(r.unitsOnTrip() == 3);
    const int sorter = w.jobs->record(1)->sorterIndex;
    CHECK(r.sorterIndex() == sorter);
    CHECK(r.goal() != kNoCell);
    CHECK(w.map.isWalkable(r.goal()));
}
