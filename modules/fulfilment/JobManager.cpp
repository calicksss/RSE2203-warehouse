#include "fulfilment/JobManager.hpp"

#include <cmath>

namespace wh {

const char* toString(OrderStatus s)
{
    switch (s) {
        case OrderStatus::Queued:    return "QUEUED";
        case OrderStatus::Picking:   return "PICKING";
        case OrderStatus::Sorting:   return "SORTING";
        case OrderStatus::Packing:   return "PACKING";
        case OrderStatus::Completed: return "COMPLETE";
    }
    return "?";
}

JobManager::JobManager(const SimConfig& config, const Catalogue& catalogue, const ShelfRegistry& shelves, const GridMap& map,
                       DistanceTable& distances, const std::vector<SortingRobot*>& sorters, EventLog& log, Metrics& metrics)
    : config_(config),
      catalogue_(catalogue),
      shelves_(shelves),
      map_(map),
      dist_(distances),
      router_(map, distances),
      sorters_(sorters),
      log_(log),
      metrics_(metrics),
      pools_(sorters.size())
{}

// ---------------------------------------------------------------------------
// Orders in
// ---------------------------------------------------------------------------

void JobManager::submit(const Order& order, Tick now)
{
    if (order.units() == 0 || records_.count(order.id) > 0) return;
    OrderRecord rec;
    rec.order = order;
    rec.unitsTotal = order.units();
    rec.submitted = now;
    records_[order.id] = rec;
    sequence_.push_back(order.id);

    const Tick key = now - (order.priority == Priority::Express ? config_.expressHeadStart : 0);
    backlog_.push({key, order.id});
    ++openOrders_;
    ++metrics_.ordersSubmitted;
    log_.add(now, order.priority == Priority::Express ? Severity::Warning : Severity::Info, "JOBS",
             "Order #" + std::to_string(order.id) + " received: " + std::to_string(order.units()) + " item(s), " +
                 toString(order.priority));
}

void JobManager::update(Tick now, const std::vector<PickingRobot*>& pickers, std::vector<DockingStation>& docks)
{
    releaseOrders(now);
    manageBatteries(now, pickers, docks);
    dispatchTrips(now, pickers, docks);
}

// ---------------------------------------------------------------------------
// Step 1: release orders to sorters
// ---------------------------------------------------------------------------

void JobManager::releaseOrders(Tick now)
{
    while (!backlog_.empty()) {
        // Greedy: the least-loaded sorter that still has a free bin.
        int best = -1;
        for (std::size_t s = 0; s < sorters_.size(); ++s) {
            if (sorters_[s]->freeBins() == 0) continue;
            if (best < 0) {
                best = static_cast<int>(s);
                continue;
            }
            const SortingRobot& a = *sorters_[s];
            const SortingRobot& b = *sorters_[static_cast<std::size_t>(best)];
            const int loadA = a.queuedUnits() + static_cast<int>(pools_[s].size());
            const int loadB = b.queuedUnits() + static_cast<int>(pools_[static_cast<std::size_t>(best)].size());
            if (a.activeOrders() < b.activeOrders() || (a.activeOrders() == b.activeOrders() && loadA < loadB))
                best = static_cast<int>(s);
        }
        if (best < 0) return;   // every bin is busy: the remaining orders keep waiting in the heap

        const BacklogEntry entry = backlog_.pop();
        OrderRecord& rec = records_.at(entry.order);
        sorters_[static_cast<std::size_t>(best)]->openBin(entry.order, rec.unitsTotal);
        rec.status = OrderStatus::Picking;
        rec.sorterIndex = best;
        rec.released = now;

        std::vector<PickUnit> units;
        for (const OrderLine& line : rec.order.lines)
            for (int q = 0; q < line.qty; ++q)
                units.push_back({entry.order, line.productIndex, shelves_.shelfOf(line.productIndex), line.unitWeightKg, entry.key});
        returnUnits(best, units);   // inserts them into the pool in urgency order

        log_.add(now, Severity::Info, "JOBS", "Order #" + std::to_string(entry.order) + " -> SRT-" + std::to_string(best + 1));
    }
}

void JobManager::returnUnits(int pool, const std::vector<PickUnit>& units)
{
    // Keep the pool sorted by priorityKey (most urgent first). Each unit is
    // inserted after all units with a key <= its key (like one step of
    // insertion sort), so units with equal keys stay first-come-first-served.
    std::vector<PickUnit>& p = pools_[static_cast<std::size_t>(pool)];
    for (const PickUnit& u : units) {
        std::size_t pos = p.size();
        while (pos > 0 && u.priorityKey < p[pos - 1].priorityKey) --pos;
        p.insert(p.begin() + static_cast<long>(pos), u);
    }
}

int JobManager::pendingUnits() const
{
    int n = 0;
    for (const std::vector<PickUnit>& p : pools_) n += static_cast<int>(p.size());
    return n;
}

// ---------------------------------------------------------------------------
// Step 2: batteries
// ---------------------------------------------------------------------------

int JobManager::distanceToNearestDock(CellIndex from, const std::vector<DockingStation>& docks)
{
    int best = kUnreachable;
    for (const DockingStation& d : docks) best = std::min(best, dist_.distance(from, d.cell()));
    return best;
}

bool JobManager::sendToCharge(Tick now, PickingRobot& robot, std::vector<DockingStation>& docks, const std::string& reason)
{
    DockingStation* nearest = nullptr;   // greedy: nearest FREE dock
    int nearestDistance = kUnreachable;
    for (DockingStation& d : docks) {
        if (!d.isFree()) continue;
        const int dd = dist_.distance(robot.cell(), d.cell());
        if (dd < nearestDistance) {
            nearestDistance = dd;
            nearest = &d;
        }
    }
    if (nearest == nullptr) return false;   // all docks busy: try again next tick
    nearest->reserve(robot.id());
    robot.sendToDock(*nearest);
    ++metrics_.chargeSessions;
    log_.add(now, robot.hasWork() ? Severity::Warning : Severity::Info, robot.name(),
             reason + " (" + std::to_string(static_cast<int>(std::lround(robot.battery()))) + "%) -> dock C" +
                 std::to_string(nearest->id() + 1));
    return true;
}

void JobManager::manageBatteries(Tick now, const std::vector<PickingRobot*>& pickers, std::vector<DockingStation>& docks)
{
    for (PickingRobot* r : pickers) {
        if (r->isCharging() || r->battery() <= 0.0) continue;
        const PickingRobot::State s = r->state();
        if (s == PickingRobot::State::Picking || s == PickingRobot::State::Dropping) continue;   // finish first

        if (r->hasWork()) {
            // On a trip: go now while the nearest dock is still within reach.
            const double needed = distanceToNearestDock(r->cell(), docks) * config_.drainPerMove + config_.batteryReserve / 2;
            if (r->battery() < needed) sendToCharge(now, *r, docks, "Battery critical, trip paused");
        } else if (r->isAvailable() && r->battery() < config_.lowBattery) {
            sendToCharge(now, *r, docks, "Low battery");
        }
    }
}

// ---------------------------------------------------------------------------
// Step 3: build trips and give them to robots
// ---------------------------------------------------------------------------

int JobManager::mostUrgentPool() const
{
    int best = -1;
    for (std::size_t s = 0; s < pools_.size(); ++s) {
        if (pools_[s].empty()) continue;
        if (best < 0 || pools_[s].front().priorityKey < pools_[static_cast<std::size_t>(best)].front().priorityKey)
            best = static_cast<int>(s);
    }
    return best;
}

std::vector<PickUnit> JobManager::takeTripUnits(int pool)
{
    // Greedy first-fit: walk the pool in urgency order and take every item that still fits.
    std::vector<PickUnit>& p = pools_[static_cast<std::size_t>(pool)];
    std::vector<PickUnit> trip;
    std::vector<PickUnit> rest;
    double weight = 0.0;
    for (const PickUnit& u : p) {
        const bool fits = static_cast<int>(trip.size()) < config_.tripCapacityUnits && weight + u.weightKg <= config_.tripPayloadKg + 1e-9;
        if (fits) {
            trip.push_back(u);
            weight += u.weightKg;
        } else {
            rest.push_back(u);
        }
    }
    p = rest;
    return trip;
}

std::vector<PickStop> JobManager::groupByShelf(const std::vector<PickUnit>& units) const
{
    // Items on the same shelf are picked in one stop. Linear search over the
    // stops is fine: a trip has at most tripCapacityUnits (8) items.
    std::vector<PickStop> stops;
    for (const PickUnit& u : units) {
        int found = -1;
        for (std::size_t i = 0; i < stops.size(); ++i)
            if (stops[i].shelf == u.shelf) found = static_cast<int>(i);
        if (found < 0) {
            PickStop s;
            s.shelf = u.shelf;
            stops.push_back(s);
            found = static_cast<int>(stops.size()) - 1;
        }
        stops[static_cast<std::size_t>(found)].units.push_back(u);
    }
    return stops;
}

int JobManager::chooseRobot(const std::vector<PickingRobot*>& candidates, const std::vector<PickStop>& stops)
{
    if (candidates.empty()) return -1;
    if (config_.assignment == AssignmentMethod::FirstAvailable) return 0;   // baseline: list order

    // Greedy: the robot closest to any shelf of the trip.
    int best = -1;
    int bestDistance = kUnreachable;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        for (const PickStop& stop : stops) {
            for (CellIndex a : map_.accessCells(stop.shelf)) {
                const int d = dist_.distance(candidates[i]->cell(), a);
                if (d < bestDistance) {
                    bestDistance = d;
                    best = static_cast<int>(i);
                }
            }
        }
    }
    return best;
}

void JobManager::dispatchTrips(Tick now, const std::vector<PickingRobot*>& pickers, std::vector<DockingStation>& docks)
{
    std::vector<PickingRobot*> idle;
    for (PickingRobot* r : pickers)
        if (r->isAvailable() && r->battery() > 0.0) idle.push_back(r);

    while (!idle.empty()) {
        const int pool = mostUrgentPool();
        if (pool < 0) return;                       // nothing to fetch
        const std::vector<PickUnit> units = takeTripUnits(pool);
        std::vector<PickStop> stops = groupByShelf(units);
        const SortingRobot& sorter = *sorters_[static_cast<std::size_t>(pool)];

        // Try robots (nearest first) until one has enough battery for this trip.
        bool assigned = false;
        while (!idle.empty() && !assigned) {
            const int pick = chooseRobot(idle, stops);
            PickingRobot& robot = *idle[static_cast<std::size_t>(pick)];
            idle.erase(idle.begin() + pick);

            std::vector<CellIndex> shelfCells;
            for (const PickStop& s : stops) shelfCells.push_back(s.shelf);
            const RoutePlan plan = router_.plan(config_.routing, config_.exactRouteLimit, robot.cell(), shelfCells, sorter.inductionCell());

            const double energy = (plan.length + distanceToNearestDock(sorter.inductionCell(), docks)) * config_.drainPerMove +
                                  config_.batteryReserve;
            // A FULLY charged robot may always start: if a trip were longer than one full
            // charge, no robot could ever take it. The mid-trip check in manageBatteries
            // then sends it to charge on the way, and it resumes the trip afterwards.
            const bool fullyCharged = robot.battery() >= config_.chargeTarget - 1e-6;
            if (robot.battery() < energy && !fullyCharged) {
                sendToCharge(now, robot, docks, "Not enough charge for the next trip");
                continue;   // try the next robot for the same trip
            }

            std::vector<PickStop> route;
            for (std::size_t k = 0; k < plan.order.size(); ++k) {
                PickStop stop = stops[static_cast<std::size_t>(plan.order[k])];
                stop.access = plan.access[k];
                route.push_back(stop);
            }
            const int tripId = nextTripId_++;
            robot.assignTrip(route, sorter.inductionCell(), pool, tripId);
            ++metrics_.tripsDispatched;
            assigned = true;

            std::string ordersText;
            for (const PickUnit& u : units) {
                const std::string tag = "#" + std::to_string(u.order);
                if (ordersText.find(tag) == std::string::npos) ordersText += (ordersText.empty() ? "" : ",") + tag;
            }
            log_.add(now, Severity::Info, robot.name(),
                     "Trip #" + std::to_string(tripId) + ": " + std::to_string(units.size()) + " item(s), " +
                         std::to_string(route.size()) + " shelf/shelves, " + std::to_string(plan.length) + " moves, orders " + ordersText);
        }
        if (!assigned) {
            returnUnits(pool, units);   // no robot could take it this tick: put the items back
            return;
        }
    }
}

// ---------------------------------------------------------------------------
// FulfilmentListener: progress reports from the robots
// ---------------------------------------------------------------------------

void JobManager::onUnitsPicked(const std::vector<PickUnit>& units, Tick /*now*/)
{
    for (const PickUnit& u : units) {
        OrderRecord& rec = records_.at(u.order);
        ++rec.unitsPicked;
        ++metrics_.unitsPicked;
        if (rec.unitsPicked == rec.unitsTotal) rec.status = OrderStatus::Sorting;
    }
}

void JobManager::onUnitsDelivered(int sorterIndex, const std::vector<PickUnit>& units, Tick /*now*/)
{
    sorters_.at(static_cast<std::size_t>(sorterIndex))->induct(units);
}

void JobManager::onUnitSorted(const PickUnit& unit, Tick /*now*/)
{
    OrderRecord& rec = records_.at(unit.order);
    ++rec.unitsSorted;
    ++metrics_.unitsSorted;
    if (rec.unitsSorted == rec.unitsTotal) rec.status = OrderStatus::Packing;
}

void JobManager::onOrderPacked(OrderId order, const std::vector<PickUnit>& packingList, int sorterIndex, Tick now)
{
    OrderRecord& rec = records_.at(order);
    rec.status = OrderStatus::Completed;
    rec.completed = now;
    rec.packingList.clear();
    for (const PickUnit& u : packingList) rec.packingList.push_back(catalogue_.at(u.productIndex).name);
    --openOrders_;
    ++metrics_.ordersCompleted;
    metrics_.sumLeadTicks += now - rec.submitted;
    metrics_.lastCompletion = now;
    log_.add(now, Severity::Success, "SRT-" + std::to_string(sorterIndex + 1),
             "Order #" + std::to_string(order) + " packed (" + std::to_string(now - rec.submitted) + " ticks after ordering)");
}

const OrderRecord* JobManager::record(OrderId id) const
{
    const auto it = records_.find(id);
    return it == records_.end() ? nullptr : &it->second;
}

} // namespace wh
