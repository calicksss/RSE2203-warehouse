#pragma once
/**
 * @file Simulation.hpp
 * @brief The whole warehouse: owns the robots, docks and controllers, and advances time.
 *
 * One call to step() = one tick:
 *
 *     1. ObstacleManager   remove old obstacles, maybe add a new one
 *     2. JobManager        release orders, manage batteries, dispatch trips
 *     3. TrafficController move every picker at most one cell (no collisions)
 *     4. every robot       robot->tick(): arrive, pick, drop, sort, pack, charge
 *                          (polymorphism: one loop over std::vector<Robot*>)
 *     5. checkInvariants   no two robots in a cell, none on a wall or obstacle, no swaps
 *
 * The Simulation never prints and never sleeps, so the same code runs in the
 * live UI (a few ticks per second), in experiments (as fast as possible) and
 * in unit tests (tick by tick).
 *
 * Ownership: robots are created with std::make_unique and owned by the
 * vectors of unique_ptr below; everybody else gets plain (non-owning)
 * pointers. Simulation cannot be copied because its members point at each other.
 */
#include <memory>
#include <random>
#include <vector>

#include "core/Config.hpp"
#include "core/EventLog.hpp"
#include "core/Metrics.hpp"
#include "fulfilment/JobManager.hpp"
#include "fulfilment/ShelfRegistry.hpp"
#include "fulfilment/SortingRobot.hpp"
#include "navigation/DistanceTable.hpp"
#include "navigation/GridMap.hpp"
#include "navigation/ObstacleManager.hpp"
#include "picking/DockingStation.hpp"
#include "picking/PickingRobot.hpp"
#include "picking/TrafficController.hpp"
#include "shop/Catalogue.hpp"
#include "shop/Order.hpp"

namespace wh {

class Simulation {
public:
    /// `catalogue` must outlive the Simulation (it is used for product names).
    /// Throws DataError if the products do not match the map's shelves, or a
    /// product is heavier than a picker can carry.
    Simulation(const GridMap& map, const Catalogue& catalogue, const SimConfig& config);
    Simulation(const Simulation&) = delete;
    Simulation& operator=(const Simulation&) = delete;

    void submit(const Order& order) { jobs_->submit(order, now_); }
    void step();

    /// Drops an obstacle on a random free cell now (the "O" key in the UI). False if impossible.
    bool spawnObstacle();
    void setRandomObstacles(bool enabled) { config_.randomObstacles = enabled; }
    /// Switch algorithms while running (the JobManager reads the config every tick).
    void setRouting(RoutingMethod method) { config_.routing = method; }
    void setAssignment(AssignmentMethod method) { config_.assignment = method; }

    Tick now() const { return now_; }
    bool allWorkDone() const { return jobs_->allWorkDone(); }
    /// No orders left AND no picker still driving (e.g. home).
    bool isQuiescent() const;

    const SimConfig& config() const { return config_; }
    const GridMap& map() const { return map_; }
    const Catalogue& catalogue() const { return catalogue_; }
    const std::vector<PickingRobot*>& pickers() const { return pickerPtrs_; }
    const std::vector<SortingRobot*>& sorters() const { return sorterPtrs_; }
    const std::vector<DockingStation>& docks() const { return docks_; }
    const JobManager& jobs() const { return *jobs_; }
    const EventLog& events() const { return log_; }
    const Metrics& metrics() const { return metrics_; }
    const ObstacleManager& obstacles() const { return obstacles_; }
    const TrafficController& traffic() const { return traffic_; }

private:
    std::vector<CellIndex> robotCells() const;
    void checkInvariants(const std::vector<CellIndex>& before);

    // Members are constructed in this order; later ones use earlier ones.
    SimConfig config_;
    GridMap map_;
    const Catalogue& catalogue_;
    ShelfRegistry shelves_;
    DistanceTable distances_;
    EventLog log_;
    Metrics metrics_;
    std::mt19937 rng_;
    std::vector<DockingStation> docks_;
    std::vector<std::unique_ptr<SortingRobot>> sorterStore_;   ///< owns the sorting robots
    std::vector<std::unique_ptr<PickingRobot>> pickerStore_;   ///< owns the picking robots
    std::vector<SortingRobot*> sorterPtrs_;
    std::vector<PickingRobot*> pickerPtrs_;
    std::vector<Robot*> fleet_;                                ///< every robot, ticked through the base class
    ObstacleManager obstacles_;
    TrafficController traffic_;
    std::unique_ptr<JobManager> jobs_;                         ///< created after the sorters exist
    Tick now_ = 0;
};

} // namespace wh
