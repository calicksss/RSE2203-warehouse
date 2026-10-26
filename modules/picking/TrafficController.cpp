#include "picking/TrafficController.hpp"

#include <algorithm>

#include "ds/Sorting.hpp"
#include "navigation/Pathfinding.hpp"

namespace wh {

TrafficController::TrafficController(const GridMap& map, const SimConfig& config)
    : map_(map), config_(config), rng_(config.seed + 17), occupant_(static_cast<std::size_t>(map.cellCount()), -1)
{}

std::vector<CellIndex> TrafficController::plannedPath(int pickerIndex) const
{
    if (pickerIndex < 0 || pickerIndex >= static_cast<int>(plans_.size())) return {};
    const Plan& plan = plans_[static_cast<std::size_t>(pickerIndex)];
    if (plan.next >= plan.path.size()) return {};
    return std::vector<CellIndex>(plan.path.begin() + static_cast<long>(plan.next), plan.path.end());
}

bool TrafficController::isFreeCell(CellIndex c, const ObstacleManager& obstacles) const
{
    return map_.isWalkable(c) && !obstacles.isBlocked(c) && occupant_[static_cast<std::size_t>(c)] == -1;
}

bool TrafficController::needsNewPlan(const Plan& plan, const PickingRobot& robot, const ObstacleManager& obstacles,
                                     CellIndex& obstacleHit) const
{
    obstacleHit = kNoCell;
    if (plan.goal != robot.goal() || plan.next >= plan.path.size()) return true;
    if (manhattan(map_, robot.cell(), plan.path[plan.next]) != 1) return true;   // pushed off the path
    for (std::size_t k = plan.next; k < plan.path.size(); ++k) {
        if (obstacles.isBlocked(plan.path[k])) {   // a new obstacle appeared on the route
            obstacleHit = plan.path[k];
            return true;
        }
    }
    return false;
}

bool TrafficController::replan(int i, const PickingRobot& robot, const ObstacleManager& obstacles, bool avoidRobots,
                               Metrics& metrics)
{
    std::vector<char> blocked = obstacles.mask();
    if (avoidRobots) {
        for (std::size_t c = 0; c < occupant_.size(); ++c)
            if (occupant_[c] != -1 && occupant_[c] != i) blocked[c] = 1;   // other robots count as walls
    }
    const PathResult result = aStarPath(map_, robot.cell(), robot.goal(), blocked);
    ++metrics.astarCalls;
    metrics.astarExpanded += result.expanded;
    if (!result.found) return false;   // keep the old plan (if any) and wait

    Plan& plan = plans_[static_cast<std::size_t>(i)];
    plan.goal = robot.goal();
    plan.path = result.path;
    plan.next = 0;
    return true;
}

void TrafficController::move(int i, PickingRobot& robot, CellIndex next, Metrics& metrics)
{
    occupant_[static_cast<std::size_t>(robot.cell())] = -1;
    occupant_[static_cast<std::size_t>(next)] = i;
    robot.moveTo(next, config_.drainPerMove);
    ++metrics.moves;
}

bool TrafficController::nudgeParkedRobot(int parked, int mover, const std::vector<PickingRobot*>& pickers,
                                         const ObstacleManager& obstacles, Metrics& metrics)
{
    PickingRobot& other = *pickers[static_cast<std::size_t>(parked)];
    // Cells the moving robot still needs: try not to park the other robot on them.
    const std::vector<CellIndex> needed = plannedPath(mover);

    CellIndex nb[4];
    const int n = map_.neighbours(other.cell(), nb);
    CellIndex best = kNoCell;
    for (int k = 0; k < n; ++k) {
        if (!isFreeCell(nb[k], obstacles)) continue;
        const bool onPath = std::find(needed.begin(), needed.end(), nb[k]) != needed.end();
        if (!onPath) {             // ideal: out of the mover's way
            best = nb[k];
            break;
        }
        if (best == kNoCell) best = nb[k];   // acceptable: at least the blocking cell is freed
    }
    if (best == kNoCell) return false;
    move(parked, other, best, metrics);
    ++metrics.nudges;
    return true;
}

bool TrafficController::sidestep(int i, int blocker, PickingRobot& robot, const ObstacleManager& obstacles, Metrics& metrics)
{
    // Free neighbour cells. Prefer ones the blocking robot does NOT need next
    // (stepping into its path would just block it again, e.g. backwards along
    // a corridor instead of into a side pocket).
    const std::vector<CellIndex> blockerPath = plannedPath(blocker);
    CellIndex nb[4];
    const int n = map_.neighbours(robot.cell(), nb);
    std::vector<CellIndex> good, any;
    for (int k = 0; k < n; ++k) {
        if (!isFreeCell(nb[k], obstacles)) continue;
        any.push_back(nb[k]);
        if (std::find(blockerPath.begin(), blockerPath.end(), nb[k]) == blockerPath.end()) good.push_back(nb[k]);
    }
    const std::vector<CellIndex>& choices = good.empty() ? any : good;
    if (choices.empty()) return false;
    std::uniform_int_distribution<std::size_t> pick(0, choices.size() - 1);   // random: breaks symmetric jams
    move(i, robot, choices[pick(rng_)], metrics);
    plans_[static_cast<std::size_t>(i)].path.clear();                         // plan again from the new cell
    ++metrics.sidesteps;
    return true;
}

void TrafficController::step(const std::vector<PickingRobot*>& pickers, const ObstacleManager& obstacles, Tick now, EventLog& log,
                             Metrics& metrics)
{
    const int count = static_cast<int>(pickers.size());
    plans_.resize(pickers.size());

    // Who stands where (O(cells) to clear, O(robots) to fill).
    std::fill(occupant_.begin(), occupant_.end(), -1);
    for (int i = 0; i < count; ++i) occupant_[static_cast<std::size_t>(pickers[static_cast<std::size_t>(i)]->cell())] = i;

    // Rule 1: robots that want to move, longest-waiting first, then by id.
    std::vector<int> movers;
    for (int i = 0; i < count; ++i)
        if (pickers[static_cast<std::size_t>(i)]->wantsToMove()) movers.push_back(i);
    insertionSort(movers, [&pickers](int a, int b) {
        const PickingRobot& ra = *pickers[static_cast<std::size_t>(a)];
        const PickingRobot& rb = *pickers[static_cast<std::size_t>(b)];
        if (ra.waitStreak() != rb.waitStreak()) return ra.waitStreak() > rb.waitStreak();
        return ra.id() < rb.id();
    });

    for (int i : movers) {
        PickingRobot& robot = *pickers[static_cast<std::size_t>(i)];
        Plan& plan = plans_[static_cast<std::size_t>(i)];
        if (!robot.wantsToMove()) continue;   // could have changed if it was nudged earlier this tick

        // Rule 2a: make sure we have a valid A* path.
        CellIndex obstacleHit = kNoCell;
        if (needsNewPlan(plan, robot, obstacles, obstacleHit)) {
            if (obstacleHit != kNoCell) {
                ++metrics.detours;
                log.add(now, Severity::Warning, robot.name(), "Obstacle on route at " + toString(map_.pos(obstacleHit)) + " - rerouting");
            }
            if (!replan(i, robot, obstacles, false, metrics)) {
                robot.recordWait();
                ++metrics.waits;
                continue;
            }
        }

        const CellIndex next = plan.path[plan.next];
        const int blocker = occupant_[static_cast<std::size_t>(next)];

        // Rule 2b: the next cell is free -> move.
        if (blocker == -1 && !obstacles.isBlocked(next)) {
            move(i, robot, next, metrics);
            ++plan.next;
            continue;
        }

        // Rule 2c: a parked robot is in the way -> ask it to move aside, then move.
        if (blocker != -1 && pickers[static_cast<std::size_t>(blocker)]->state() == PickingRobot::State::Idle &&
            nudgeParkedRobot(blocker, i, pickers, obstacles, metrics)) {
            move(i, robot, next, metrics);
            ++plan.next;
            continue;
        }

        // Rule 2d: blocked by a robot that is working, driving or stuck.
        const int waited = robot.waitStreak() + 1;   // including this tick
        if (waited >= config_.replanAfterWaits && replan(i, robot, obstacles, true, metrics)) {
            // Detour that treats the other robots as walls. Its first cell is free
            // unless it is the goal itself (the goal is never treated as blocked).
            const CellIndex detourNext = plan.path[plan.next];
            if (occupant_[static_cast<std::size_t>(detourNext)] == -1) {
                ++metrics.detours;
                move(i, robot, detourNext, metrics);
                ++plan.next;
                continue;
            }
        }
        robot.recordWait();
        ++metrics.waits;
        metrics.longestWait = std::max(metrics.longestWait, robot.waitStreak());

        // Both robots are waiting for each other (a jam): one random step aside breaks it.
        const bool blockerAlsoStuck = blocker != -1 && pickers[static_cast<std::size_t>(blocker)]->waitStreak() > 0;
        if (waited >= config_.sidestepAfterWaits && blockerAlsoStuck) sidestep(i, blocker, robot, obstacles, metrics);
    }
}

} // namespace wh
