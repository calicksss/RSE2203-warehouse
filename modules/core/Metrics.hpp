#pragma once
/**
 * @file Metrics.hpp
 * @brief Counters collected while the simulation runs (shown in the UI and used by experiments).
 */
#include "core/Types.hpp"

namespace wh {

struct Metrics {
    // Orders
    int ordersSubmitted = 0;
    int ordersCompleted = 0;
    long unitsPicked = 0;
    long unitsSorted = 0;
    long tripsDispatched = 0;
    Tick sumLeadTicks = 0;     ///< Sum over completed orders of (packed tick - submitted tick).
    Tick lastCompletion = 0;   ///< Tick the most recent order was packed.

    // Robots and traffic
    long moves = 0;            ///< Cells travelled by all pickers together.
    long waits = 0;            ///< Ticks a picker wanted to move but had to wait.
    long detours = 0;          ///< A* re-plans (because of an obstacle or a blocking robot).
    long sidesteps = 0;        ///< Times a stuck robot stepped aside to break a jam.
    long nudges = 0;           ///< Times a parked robot was asked to move out of the way.
    int longestWait = 0;       ///< Longest run of consecutive waits by one robot.
    long chargeSessions = 0;
    long obstaclesSpawned = 0;
    long collisions = 0;       ///< Two robots in one cell, or a robot on a wall/obstacle. MUST stay 0.

    // Path planning cost
    long astarCalls = 0;
    long astarExpanded = 0;    ///< Nodes taken out of the A* heap, summed over all calls.

    double avgLeadTime() const { return ordersCompleted ? double(sumLeadTicks) / ordersCompleted : 0.0; }
    /// Orders completed per 100 ticks.
    double throughput(Tick now) const { return now > 0 ? 100.0 * ordersCompleted / double(now) : 0.0; }
    /// Share of move attempts that had to wait (0 = free-flowing traffic).
    double congestion() const { return moves + waits ? double(waits) / double(moves + waits) : 0.0; }
};

} // namespace wh
