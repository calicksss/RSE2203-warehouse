#pragma once
/**
 * @file JobManager.hpp
 * @brief The dispatcher: queues orders, gives them to sorters, builds picker trips, manages charging.
 *
 * Every tick, update() runs three steps:
 *
 * 1. RELEASE orders (priority queue + greedy load balancing)
 *    New orders wait in a binary min-heap (ds::MinHeap) keyed by
 *        key = submit tick - (express ? expressHeadStart : 0)
 *    so the most urgent order is always on top: O(log n) per order.
 *    The top order goes to the sorter with a free bin and the LEAST work
 *    (fewest open orders, then fewest waiting items). This is the greedy
 *    "give the next job to the least-loaded processor" rule from Lecture 5b.
 *    Its items become PickUnits in that sorter's "pool" of items to fetch.
 *
 * 2. BATTERIES
 *    A robot on a trip that could not reach a dock any more is sent to charge
 *    now (its trip is paused, not lost). An idle robot below `lowBattery`
 *    charges. Docks are chosen greedily: the nearest free one.
 *
 * 3. DISPATCH trips (greedy batching + greedy assignment)
 *    While there are idle robots and items waiting:
 *      a. take the pool whose first item is the most urgent;
 *      b. fill a trip with items in urgency order while they fit
 *         (tripCapacityUnits items, tripPayloadKg kilograms): greedy first-fit.
 *         One trip may carry items of several orders; the sorter separates them;
 *      c. choose a robot: the NEAREST idle robot (greedy) or, as a baseline,
 *         the first idle robot;
 *      d. order the shelves with the RoutePlanner (backtracking or greedy);
 *      e. check the battery is enough for the whole trip plus the way to a dock.
 *
 * JobManager implements FulfilmentListener, so robots can report progress
 * (picked / delivered / sorted / packed) without knowing this class.
 */
#include <map>
#include <string>
#include <vector>

#include "core/Config.hpp"
#include "core/EventLog.hpp"
#include "core/Metrics.hpp"
#include "core/Robot.hpp"
#include "ds/MinHeap.hpp"
#include "fulfilment/ShelfRegistry.hpp"
#include "fulfilment/SortingRobot.hpp"
#include "navigation/DistanceTable.hpp"
#include "navigation/RoutePlanner.hpp"
#include "picking/DockingStation.hpp"
#include "picking/PickingRobot.hpp"
#include "shop/Catalogue.hpp"
#include "shop/Order.hpp"

namespace wh {

enum class OrderStatus { Queued, Picking, Sorting, Packing, Completed };
const char* toString(OrderStatus s);

/// Everything we know about one order's progress.
struct OrderRecord {
    Order order;
    OrderStatus status = OrderStatus::Queued;
    int sorterIndex = -1;
    int unitsTotal = 0;
    int unitsPicked = 0;
    int unitsSorted = 0;
    Tick submitted = 0;
    Tick released = -1;
    Tick completed = -1;
    std::vector<std::string> packingList;   ///< product names, heaviest first
};

class JobManager : public FulfilmentListener {
public:
    JobManager(const SimConfig& config, const Catalogue& catalogue, const ShelfRegistry& shelves, const GridMap& map,
               DistanceTable& distances, const std::vector<SortingRobot*>& sorters, EventLog& log, Metrics& metrics);

    /// A new customer order arrives. Orders with no items, or an id we already have, are ignored.
    void submit(const Order& order, Tick now);

    /// The three steps described above. Call once per tick, before the robots move.
    void update(Tick now, const std::vector<PickingRobot*>& pickers, std::vector<DockingStation>& docks);

    // FulfilmentListener (called by the robots)
    void onUnitsPicked(const std::vector<PickUnit>& units, Tick now) override;
    void onUnitsDelivered(int sorterIndex, const std::vector<PickUnit>& units, Tick now) override;
    void onUnitSorted(const PickUnit& unit, Tick now) override;
    void onOrderPacked(OrderId order, const std::vector<PickUnit>& packingList, int sorterIndex, Tick now) override;

    // Questions (for the UI, tests and experiments)
    const OrderRecord* record(OrderId id) const;          ///< nullptr if unknown
    const std::vector<OrderId>& orderIds() const { return sequence_; }   ///< in arrival order
    int backlogSize() const { return backlog_.size(); }
    int pendingUnits() const;                             ///< released items not yet on a robot
    int openOrders() const { return openOrders_; }
    bool allWorkDone() const { return openOrders_ == 0; }

private:
    struct BacklogEntry {
        Tick key;       ///< smaller = more urgent
        OrderId order;  ///< tie-break: older id first
        bool operator<(const BacklogEntry& o) const { return key != o.key ? key < o.key : order < o.order; }
    };

    void releaseOrders(Tick now);
    void manageBatteries(Tick now, const std::vector<PickingRobot*>& pickers, std::vector<DockingStation>& docks);
    void dispatchTrips(Tick now, const std::vector<PickingRobot*>& pickers, std::vector<DockingStation>& docks);

    int mostUrgentPool() const;
    std::vector<PickUnit> takeTripUnits(int pool);
    void returnUnits(int pool, const std::vector<PickUnit>& units);
    std::vector<PickStop> groupByShelf(const std::vector<PickUnit>& units) const;
    int chooseRobot(const std::vector<PickingRobot*>& candidates, const std::vector<PickStop>& stops);
    int distanceToNearestDock(CellIndex from, const std::vector<DockingStation>& docks);
    bool sendToCharge(Tick now, PickingRobot& robot, std::vector<DockingStation>& docks, const std::string& reason);

    const SimConfig& config_;
    const Catalogue& catalogue_;
    const ShelfRegistry& shelves_;
    const GridMap& map_;
    DistanceTable& dist_;
    RoutePlanner router_;
    std::vector<SortingRobot*> sorters_;
    EventLog& log_;
    Metrics& metrics_;

    MinHeap<BacklogEntry> backlog_;                 ///< orders not yet given to a sorter
    std::map<OrderId, OrderRecord> records_;        ///< every order ever submitted (balanced BST: O(log n))
    std::vector<OrderId> sequence_;                 ///< order ids in arrival order (for display)
    std::vector<std::vector<PickUnit>> pools_;      ///< per sorter: items waiting for a picker, most urgent first
    int openOrders_ = 0;
    int nextTripId_ = 1;
};

} // namespace wh
