#include "sim/Simulation.hpp"

#include <algorithm>
#include <cstdio>
#include <set>

#include "core/Errors.hpp"

namespace wh {

namespace {
std::vector<CellIndex> protectedCellsOf(const GridMap& map)
{
    // Cells robots must be able to stand on to work: obstacles never appear here.
    std::vector<CellIndex> cells;
    for (CellIndex s : map.shelves())
        for (CellIndex a : map.accessCells(s)) cells.push_back(a);
    for (CellIndex r : map.sorters()) cells.push_back(map.inductionCellFor(r));
    cells.insert(cells.end(), map.docks().begin(), map.docks().end());
    cells.insert(cells.end(), map.homes().begin(), map.homes().end());
    return cells;
}
} // namespace

Simulation::Simulation(const GridMap& map, const Catalogue& catalogue, const SimConfig& config)
    : config_(config),
      map_(map),
      catalogue_(catalogue),
      shelves_(map_, catalogue_),
      distances_(map_),
      rng_(config_.seed),
      obstacles_(map_, protectedCellsOf(map_)),
      traffic_(map_, config_)
{
    for (int i = 0; i < catalogue_.size(); ++i) {
        if (catalogue_.at(i).weightKg > config_.tripPayloadKg) {
            char buf[48];
            std::snprintf(buf, sizeof buf, "%.2f kg", config_.tripPayloadKg);
            throw DataError("products", i + 1, "'" + catalogue_.at(i).name + "' is heavier than a picker can carry (" + buf + ")");
        }
    }

    for (std::size_t i = 0; i < map_.docks().size(); ++i) docks_.emplace_back(static_cast<int>(i), map_.docks()[i], config_.chargePerTick);

    RobotId nextId = 1;
    for (std::size_t i = 0; i < map_.sorters().size(); ++i) {
        const CellIndex cell = map_.sorters()[i];
        sorterStore_.push_back(std::make_unique<SortingRobot>(nextId++, static_cast<int>(i), cell, map_.inductionCellFor(cell), config_.sorterBins));
    }

    std::size_t pickerCount = map_.homes().size();
    if (config_.maxPickers > 0) pickerCount = std::min(pickerCount, static_cast<std::size_t>(config_.maxPickers));
    std::uniform_real_distribution<double> initialCharge(60.0, 100.0);
    for (std::size_t i = 0; i < pickerCount; ++i)
        pickerStore_.push_back(std::make_unique<PickingRobot>(nextId++, static_cast<int>(i + 1), map_.homes()[i], initialCharge(rng_)));

    for (auto& s : sorterStore_) {
        sorterPtrs_.push_back(s.get());
        fleet_.push_back(s.get());
    }
    for (auto& p : pickerStore_) {
        pickerPtrs_.push_back(p.get());
        fleet_.push_back(p.get());
    }

    jobs_ = std::make_unique<JobManager>(config_, catalogue_, shelves_, map_, distances_, sorterPtrs_, log_, metrics_);

    log_.add(0, Severity::Info, "SYSTEM",
             "Warehouse online: " + std::to_string(pickerPtrs_.size()) + " pickers, " + std::to_string(sorterPtrs_.size()) +
                 " sorters, " + std::to_string(docks_.size()) + " docks, " + std::to_string(map_.shelves().size()) + " shelves");
}

std::vector<CellIndex> Simulation::robotCells() const
{
    std::vector<CellIndex> cells;
    for (const PickingRobot* p : pickerPtrs_) cells.push_back(p->cell());
    return cells;
}

void Simulation::step()
{
    ++now_;
    const std::vector<CellIndex> before = robotCells();
    if (obstacles_.update(now_, config_, rng_, before, log_)) ++metrics_.obstaclesSpawned;
    jobs_->update(now_, pickerPtrs_, docks_);
    traffic_.step(pickerPtrs_, obstacles_, now_, log_, metrics_);

    TickContext ctx{now_, config_, log_, *jobs_};
    for (Robot* robot : fleet_) robot->tick(ctx);   // virtual call: PickingRobot::tick or SortingRobot::tick

    checkInvariants(before);
}

bool Simulation::spawnObstacle()
{
    const bool ok = obstacles_.spawnRandom(now_, config_, rng_, robotCells(), log_);
    if (ok) ++metrics_.obstaclesSpawned;
    return ok;
}

bool Simulation::isQuiescent() const
{
    if (!jobs_->allWorkDone()) return false;
    for (const PickingRobot* p : pickerPtrs_)
        if (p->wantsToMove()) return false;
    return true;
}

void Simulation::checkInvariants(const std::vector<CellIndex>& before)
{
    std::set<CellIndex> seen;
    for (std::size_t i = 0; i < pickerPtrs_.size(); ++i) {
        const PickingRobot& p = *pickerPtrs_[i];
        bool bad = !seen.insert(p.cell()).second || !map_.isWalkable(p.cell()) || obstacles_.isBlocked(p.cell());
        for (std::size_t j = 0; j < pickerPtrs_.size(); ++j)   // two robots swapped cells this tick
            if (i != j && p.cell() == before[j] && pickerPtrs_[j]->cell() == before[i] && p.cell() != before[i]) bad = true;
        if (bad) {
            ++metrics_.collisions;
            log_.add(now_, Severity::Alert, p.name(), "SAFETY: invalid position " + toString(map_.pos(p.cell())));
        }
    }
}

} // namespace wh
