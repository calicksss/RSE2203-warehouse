#pragma once
/**
 * @file Config.hpp
 * @brief Every tunable number in one place (no "magic numbers" hidden in the logic).
 *
 * The two method switches (RoutingMethod, AssignmentMethod) exist so the
 * experiments program can compare a simple baseline with a better algorithm
 * on exactly the same workload.
 */
#include <string>

#include "core/Types.hpp"

namespace wh {

/// How a picker orders the shelves it visits on one trip (navigation/RoutePlanner).
enum class RoutingMethod {
    AsGiven,       ///< Visit shelves in the order the items were taken from the queue (baseline).
    Greedy,        ///< Nearest-neighbour: always go to the closest unvisited shelf. O(k^2).
    Backtracking   ///< Try every order recursively, prune bad branches. Exact but O(k!) worst case.
};

/// How the job manager chooses a picker for a trip (fulfilment/JobManager).
enum class AssignmentMethod {
    FirstAvailable, ///< Lowest-numbered idle robot (baseline).
    Nearest         ///< Idle robot closest to the trip's first shelf (greedy).
};

std::string toString(RoutingMethod m);
std::string toString(AssignmentMethod m);

struct SimConfig {
    // ---- Fleet --------------------------------------------------------------
    int maxPickers = 0;            ///< 0 = one picker per 'T' cell in the map.
    int tripCapacityUnits = 8;     ///< Items a picker can carry in one trip.
    double tripPayloadKg = 12.0;   ///< Weight a picker can carry in one trip.
    int sorterBins = 4;            ///< Bins per sorting robot = orders it can sort at the same time.

    // ---- Battery (percent) --------------------------------------------------
    double drainPerMove = 0.30;
    double drainPerIdleTick = 0.01;
    double chargePerTick = 2.5;
    double lowBattery = 30.0;      ///< Idle robots below this go and charge.
    double batteryReserve = 8.0;   ///< Never start a trip that would end below this.
    double chargeTarget = 95.0;    ///< Leave the dock once charged to this level.

    // ---- How long each job takes (ticks) -----------------------------------
    int pickTicksPerUnit = 1;
    int dropTicks = 2;
    int sortTicksPerUnit = 1;
    int packTicks = 4;

    // ---- Temporary obstacles (spills, boxes, people) -----------------------
    bool randomObstacles = true;
    double obstacleSpawnChance = 0.03;  ///< Chance per tick that a new obstacle appears.
    int maxObstacles = 3;
    int obstacleMinLife = 25;
    int obstacleMaxLife = 70;

    // ---- Traffic rules (picking/TrafficController) -------------------------
    int replanAfterWaits = 3;      ///< Blocked this many ticks: plan a detour around other robots.
    int sidestepAfterWaits = 6;    ///< Still blocked by a robot that is ALSO stuck: step aside.

    // ---- Scheduling (fulfilment/JobManager) --------------------------------
    /// Express orders are treated as if they arrived this many ticks earlier.
    /// The bonus is a constant, so an old standard order is never overtaken
    /// forever (no starvation).
    Tick expressHeadStart = 120;
    RoutingMethod routing = RoutingMethod::Backtracking;
    int exactRouteLimit = 8;       ///< Backtracking is used up to this many shelves, greedy above.
    AssignmentMethod assignment = AssignmentMethod::Nearest;

    unsigned seed = 2026;          ///< Same seed = same run (reproducible experiments).
};

} // namespace wh
