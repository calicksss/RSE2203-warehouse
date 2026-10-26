#pragma once
/**
 * @file TrafficController.hpp
 * @brief Moves all picking robots one step per tick without collisions, avoiding obstacles.
 *
 * Each robot follows its own A* path (navigation/Pathfinding). Robots do not
 * know about each other, so this class applies simple traffic rules every tick:
 *
 *   1. Order the robots that want to move: the one that has waited longest goes
 *      first (fairness), ties by id. (insertionSort: a handful of robots, and the
 *      order barely changes between ticks, which is insertion sort's best case.)
 *   2. For each robot in that order:
 *      a. (Re)plan with A* if it has no path, its goal changed, it was pushed off
 *         its path, or a NEW OBSTACLE now lies on its path (= obstacle avoidance).
 *      b. If the next cell on the path is EMPTY right now, move into it.
 *         Robots move one after another, so a robot can follow into a cell that
 *         another robot left earlier in the same tick.
 *      c. If a PARKED (idle) robot is in the way, ask it to step aside, then move.
 *      d. Otherwise WAIT. After `replanAfterWaits` waits, plan a detour that
 *         treats the other robots as walls. After `sidestepAfterWaits` waits, if
 *         the robot in the way is ALSO stuck (both are waiting for each other:
 *         a deadlock), step to a free neighbour cell, preferably one that is not
 *         on the other robot's path (random choice), so one of them can pass.
 *
 * Safety argument (why there are never collisions):
 *   A robot only ever moves into a cell that is empty at that moment (rule b/c),
 *   and occupancy is updated immediately after each move. So no two robots can
 *   end up in the same cell, and two robots can never swap cells (a swap would
 *   need each to move into the other's occupied cell). Obstacle cells are never
 *   entered either. Simulation::checkInvariants verifies this every tick.
 *
 * Limitation (be honest in the report): rule d is a heuristic. It breaks the
 * jams we have seen in testing, but it does not PROVE that every jam ends.
 * Multi-robot path finding with guarantees (e.g. CBS, PIBT) is beyond this
 * module; see docs/DESIGN_DECISIONS.md.
 */
#include <random>
#include <vector>

#include "core/Config.hpp"
#include "core/EventLog.hpp"
#include "core/Metrics.hpp"
#include "navigation/GridMap.hpp"
#include "navigation/ObstacleManager.hpp"
#include "picking/PickingRobot.hpp"

namespace wh {

class TrafficController {
public:
    TrafficController(const GridMap& map, const SimConfig& config);

    /// Moves every picker that wants to move by at most one cell. Call once per tick.
    void step(const std::vector<PickingRobot*>& pickers, const ObstacleManager& obstacles, Tick now, EventLog& log,
              Metrics& metrics);

    /// Remaining planned cells of picker number `pickerIndex` (for the map preview in the UI).
    std::vector<CellIndex> plannedPath(int pickerIndex) const;

private:
    struct Plan {
        CellIndex goal = kNoCell;
        std::vector<CellIndex> path;   ///< from A*: cells to step on, ending at the goal
        std::size_t next = 0;          ///< path[next] is the next cell to step on
    };

    bool needsNewPlan(const Plan& plan, const PickingRobot& robot, const ObstacleManager& obstacles, CellIndex& obstacleHit) const;
    bool replan(int i, const PickingRobot& robot, const ObstacleManager& obstacles, bool avoidRobots, Metrics& metrics);
    void move(int i, PickingRobot& robot, CellIndex next, Metrics& metrics);
    bool nudgeParkedRobot(int parked, int mover, const std::vector<PickingRobot*>& pickers, const ObstacleManager& obstacles,
                          Metrics& metrics);
    bool sidestep(int i, int blocker, PickingRobot& robot, const ObstacleManager& obstacles, Metrics& metrics);
    bool isFreeCell(CellIndex c, const ObstacleManager& obstacles) const;

    const GridMap& map_;
    const SimConfig& config_;
    std::mt19937 rng_;
    std::vector<Plan> plans_;      ///< one per picker, same order as the pickers list
    std::vector<int> occupant_;    ///< per cell: index of the picker standing there, or -1
};

} // namespace wh
