#include "picking/PickingRobot.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace wh {

PickingRobot::PickingRobot(RobotId id, int number, CellIndex home, double initialBattery)
    : Robot(id, "PCK-" + std::to_string(number), home), home_(home), battery_(initialBattery)
{}

const char* PickingRobot::toString(State s)
{
    switch (s) {
        case State::Idle:      return "IDLE";
        case State::ToShelf:   return "TO SHELF";
        case State::Picking:   return "PICKING";
        case State::ToSorter:  return "TO SORTER";
        case State::Dropping:  return "DROPPING";
        case State::ToDock:    return "TO DOCK";
        case State::Charging:  return "CHARGING";
        case State::Returning: return "RETURNING";
    }
    return "?";
}

std::string PickingRobot::stateLabel() const
{
    return battery_.isEmpty() ? "STRANDED" : toString(state_);
}

CellIndex PickingRobot::goal() const
{
    switch (state_) {
        case State::ToShelf:   return route_.empty() ? kNoCell : route_.front().access;
        case State::ToSorter:  return dropCell_;
        case State::ToDock:    return dock_ != nullptr ? dock_->cell() : kNoCell;
        case State::Returning: return home_;
        default:               return kNoCell;   // Idle, Picking, Dropping, Charging: not driving
    }
}

int PickingRobot::unitsOnTrip() const
{
    int n = static_cast<int>(cargo_.size());
    route_.forEach([&n](const PickStop& stop) { n += static_cast<int>(stop.units.size()); });
    return n;
}

std::vector<CellIndex> PickingRobot::shelvesLeft() const
{
    std::vector<CellIndex> shelves;
    route_.forEach([&shelves](const PickStop& stop) { shelves.push_back(stop.shelf); });
    return shelves;
}

void PickingRobot::assignTrip(const std::vector<PickStop>& stops, CellIndex dropCell, int sorterIndex, int tripId)
{
    if (!isAvailable()) throw std::logic_error(name() + ": assignTrip while busy");
    if (stops.empty()) throw std::invalid_argument(name() + ": a trip needs at least one shelf");
    for (const PickStop& stop : stops) route_.enqueue(stop);
    dropCell_ = dropCell;
    sorterIndex_ = sorterIndex;
    tripId_ = tripId;
    state_ = State::ToShelf;
}

void PickingRobot::sendToDock(DockingStation& dock)
{
    if (state_ == State::Picking || state_ == State::Dropping || isCharging())
        throw std::logic_error(name() + ": cannot go to a dock while " + toString(state_));
    dock_ = &dock;
    state_ = State::ToDock;
}

void PickingRobot::moveTo(CellIndex next, double batteryUsed)
{
    if (state_ == State::Idle) state_ = State::Returning;   // pushed out of the way: drive back home later
    cell_ = next;
    battery_.drain(batteryUsed);
    ++moves_;
    waitStreak_ = 0;
    movedThisTick_ = true;
}

void PickingRobot::resumeAfterCharging(TickContext& ctx)
{
    if (!route_.empty()) {
        state_ = State::ToShelf;
        log(ctx, Severity::Info, "Charged - resuming trip #" + std::to_string(tripId_));
    } else if (!cargo_.empty()) {
        state_ = State::ToSorter;
        log(ctx, Severity::Info, "Charged - resuming delivery");
    } else {
        state_ = State::Returning;
    }
}

void PickingRobot::tick(TickContext& ctx)
{
    const bool moved = movedThisTick_;
    movedThisTick_ = false;
    if (!moved && state_ != State::Charging) battery_.drain(ctx.config.drainPerIdleTick);

    // Safety net: a flat battery would freeze this robot (and its orders) for
    // ever, so we model a technician swapping the battery after 30 ticks.
    if (battery_.isEmpty()) {
        if (++stalledTicks_ >= 30) {
            battery_.charge(25.0);
            stalledTicks_ = 0;
            log(ctx, Severity::Alert, "Battery empty - swapped by technician");
        }
        return;
    }

    switch (state_) {
        case State::Idle:
            break;

        case State::ToShelf:
            if (cell_ == route_.front().access) {
                state_ = State::Picking;
                dwell_ = std::max(1, ctx.config.pickTicksPerUnit * static_cast<int>(route_.front().units.size()));
            }
            break;

        case State::Picking:
            if (--dwell_ <= 0) {
                const PickStop stop = route_.dequeue();
                cargo_.insert(cargo_.end(), stop.units.begin(), stop.units.end());
                ctx.listener.onUnitsPicked(stop.units, ctx.now);
                state_ = route_.empty() ? State::ToSorter : State::ToShelf;
            }
            break;

        case State::ToSorter:
            if (cell_ == dropCell_) {
                state_ = State::Dropping;
                dwell_ = std::max(1, ctx.config.dropTicks);
            }
            break;

        case State::Dropping:
            if (--dwell_ <= 0) {
                ctx.listener.onUnitsDelivered(sorterIndex_, cargo_, ctx.now);
                log(ctx, Severity::Success, "Delivered " + std::to_string(cargo_.size()) + " item(s) to SRT-" +
                                                std::to_string(sorterIndex_ + 1));
                cargo_.clear();
                sorterIndex_ = -1;
                state_ = State::Returning;
            }
            break;

        case State::ToDock:
            if (dock_ != nullptr && cell_ == dock_->cell()) {
                state_ = State::Charging;
                log(ctx, Severity::Info, "Docked at C" + std::to_string(dock_->id() + 1) + " (" +
                                             std::to_string(static_cast<int>(std::lround(battery_.percent()))) + "%)");
            }
            break;

        case State::Charging:
            battery_.charge(dock_->powerPerTick());
            if (battery_.percent() >= ctx.config.chargeTarget) {
                dock_->release(id());
                dock_ = nullptr;
                resumeAfterCharging(ctx);
            }
            break;

        case State::Returning:
            if (cell_ == home_) state_ = State::Idle;
            break;
    }
}

std::string PickingRobot::activity() const
{
    switch (state_) {
        case State::Idle:      return "Parked at home";
        case State::ToShelf:   return "Trip #" + std::to_string(tripId_) + ": " + std::to_string(route_.size()) + " shelf/shelves left";
        case State::Picking:   return "Picking " + std::to_string(route_.empty() ? 0 : route_.front().units.size()) + " item(s)";
        case State::ToSorter:  return "Delivering " + std::to_string(cargo_.size()) + " item(s) to SRT-" + std::to_string(sorterIndex_ + 1);
        case State::Dropping:  return "Handing over " + std::to_string(cargo_.size()) + " item(s)";
        case State::ToDock:    return "Heading to dock C" + std::to_string(dock_ != nullptr ? dock_->id() + 1 : 0);
        case State::Charging:  return "Charging at C" + std::to_string(dock_ != nullptr ? dock_->id() + 1 : 0);
        case State::Returning: return "Returning home";
    }
    return "";
}

} // namespace wh
