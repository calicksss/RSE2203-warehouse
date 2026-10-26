#pragma once
/**
 * @file ObstacleManager.hpp
 * @brief Temporary obstacles (a spill, a dropped box, a person) that robots must drive around.
 *
 * Obstacles appear at random on free floor cells and disappear after a while.
 * They are never placed on cells robots MUST stand on to work (next to
 * shelves, at sorters, docks and homes), so an obstacle can cause a detour or
 * a wait but never make a job impossible.
 *
 * `mask()` has one entry per cell (1 = blocked) and is passed to A* as the
 * `blocked` argument.
 */
#include <random>
#include <vector>

#include "core/Config.hpp"
#include "core/EventLog.hpp"
#include "navigation/GridMap.hpp"

namespace wh {

struct Obstacle {
    CellIndex cell = kNoCell;
    Tick expires = 0;
};

class ObstacleManager {
public:
    ObstacleManager(const GridMap& map, const std::vector<CellIndex>& protectedCells);

    /// Removes expired obstacles and (if enabled) maybe spawns one. Returns true if one spawned.
    bool update(Tick now, const SimConfig& config, std::mt19937& rng, const std::vector<CellIndex>& robotCells, EventLog& log);

    /// Places an obstacle on a random allowed cell right now. Returns false if there is none.
    bool spawnRandom(Tick now, const SimConfig& config, std::mt19937& rng, const std::vector<CellIndex>& robotCells, EventLog& log);

    bool isBlocked(CellIndex c) const { return mask_[static_cast<std::size_t>(c)] != 0; }
    const std::vector<char>& mask() const { return mask_; }
    const std::vector<Obstacle>& active() const { return active_; }

private:
    const GridMap& map_;
    std::vector<char> allowed_;   ///< 1 = an obstacle may appear here
    std::vector<char> mask_;      ///< 1 = blocked right now
    std::vector<Obstacle> active_;
};

} // namespace wh
