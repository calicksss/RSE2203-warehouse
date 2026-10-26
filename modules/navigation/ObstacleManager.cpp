#include "navigation/ObstacleManager.hpp"

#include <algorithm>

namespace wh {

ObstacleManager::ObstacleManager(const GridMap& map, const std::vector<CellIndex>& protectedCells)
    : map_(map),
      allowed_(static_cast<std::size_t>(map.cellCount()), 0),
      mask_(static_cast<std::size_t>(map.cellCount()), 0)
{
    for (CellIndex c = 0; c < map.cellCount(); ++c)
        allowed_[static_cast<std::size_t>(c)] = map.tile(c) == Tile::Floor ? 1 : 0;
    for (CellIndex c : protectedCells) allowed_[static_cast<std::size_t>(c)] = 0;
}

bool ObstacleManager::update(Tick now, const SimConfig& config, std::mt19937& rng, const std::vector<CellIndex>& robotCells,
                             EventLog& log)
{
    // Remove expired obstacles. We copy the ones we keep into a new list: O(n).
    std::vector<Obstacle> keep;
    for (const Obstacle& o : active_) {
        if (o.expires > now) {
            keep.push_back(o);
        } else {
            mask_[static_cast<std::size_t>(o.cell)] = 0;
            log.add(now, Severity::Info, "FLOOR", "Obstacle cleared at " + toString(map_.pos(o.cell)));
        }
    }
    active_ = keep;

    if (!config.randomObstacles || static_cast<int>(active_.size()) >= config.maxObstacles) return false;
    std::bernoulli_distribution chance(config.obstacleSpawnChance);
    return chance(rng) && spawnRandom(now, config, rng, robotCells, log);
}

bool ObstacleManager::spawnRandom(Tick now, const SimConfig& config, std::mt19937& rng, const std::vector<CellIndex>& robotCells,
                                  EventLog& log)
{
    std::vector<CellIndex> candidates;
    for (CellIndex c = 0; c < map_.cellCount(); ++c) {
        if (!allowed_[static_cast<std::size_t>(c)] || mask_[static_cast<std::size_t>(c)]) continue;
        if (std::find(robotCells.begin(), robotCells.end(), c) != robotCells.end()) continue;   // never on a robot
        candidates.push_back(c);
    }
    if (candidates.empty()) return false;

    std::uniform_int_distribution<std::size_t> pick(0, candidates.size() - 1);
    std::uniform_int_distribution<int> life(config.obstacleMinLife, std::max(config.obstacleMinLife, config.obstacleMaxLife));
    const CellIndex cell = candidates[pick(rng)];
    active_.push_back({cell, now + life(rng)});
    mask_[static_cast<std::size_t>(cell)] = 1;
    log.add(now, Severity::Warning, "FLOOR", "Obstacle detected at " + toString(map_.pos(cell)));
    return true;
}

} // namespace wh
