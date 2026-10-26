#pragma once
/**
 * @file PickingRobot.hpp
 * @brief A mobile robot that fetches items from shelves and brings them to a sorter.
 *
 * Finite-state machine (each arrow is one `if` in PickingRobot::tick):
 *
 *   IDLE --assignTrip--> TO_SHELF --arrive--> PICKING --done--> TO_SHELF  (more shelves)
 *                                                         \---> TO_SORTER (last shelf done)
 *   TO_SORTER --arrive--> DROPPING --done--> RETURNING --arrive home--> IDLE
 *   (almost any state) --sendToDock--> TO_DOCK --arrive--> CHARGING --full--> resume the trip
 *
 * What the robot does NOT decide (single responsibility):
 *   - WHERE to go next and which items: the JobManager gives it a trip.
 *   - HOW to move without hitting anything: the TrafficController moves all
 *     robots together and calls moveTo() / recordWait().
 * The robot only knows its own state, battery, route and cargo.
 */
#include <string>
#include <vector>

#include "core/Robot.hpp"
#include "ds/LinkedQueue.hpp"
#include "picking/Battery.hpp"
#include "picking/DockingStation.hpp"

namespace wh {

class PickingRobot : public Robot {
public:
    enum class State { Idle, ToShelf, Picking, ToSorter, Dropping, ToDock, Charging, Returning };
    static const char* toString(State s);

    PickingRobot(RobotId id, int number, CellIndex home, double initialBattery);

    // ---- Commands from the JobManager -----------------------------------------
    /// Start a trip: visit `stops` in order, then deliver to the sorter whose drop-off cell is `dropCell`.
    /// Precondition: isAvailable(). Throws std::logic_error otherwise.
    void assignTrip(const std::vector<PickStop>& stops, CellIndex dropCell, int sorterIndex, int tripId);
    /// Go and charge at `dock` (already reserved). A trip in progress is paused and resumed afterwards.
    void sendToDock(DockingStation& dock);

    // ---- Commands from the TrafficController ----------------------------------
    void moveTo(CellIndex next, double batteryUsed);
    void recordWait() { ++waitStreak_; ++totalWaits_; }

    // ---- Questions --------------------------------------------------------------
    State state() const { return state_; }
    /// The cell the robot is driving to right now, or kNoCell if it is not driving.
    CellIndex goal() const;
    bool wantsToMove() const { return goal() != kNoCell && goal() != cell_ && !battery_.isEmpty(); }
    /// Free for a new trip (idle, or driving home empty).
    bool isAvailable() const { return (state_ == State::Idle || state_ == State::Returning) && route_.empty() && cargo_.empty(); }
    bool hasWork() const { return !route_.empty() || !cargo_.empty(); }
    bool isCharging() const { return state_ == State::ToDock || state_ == State::Charging; }
    double battery() const { return battery_.percent(); }
    CellIndex home() const { return home_; }
    int waitStreak() const { return waitStreak_; }
    long totalWaits() const { return totalWaits_; }
    long moves() const { return moves_; }
    int tripId() const { return tripId_; }
    int sorterIndex() const { return sorterIndex_; }
    int stopsLeft() const { return route_.size(); }
    /// Items still to pick plus items being carried.
    int unitsOnTrip() const;
    /// Shelf cells still to visit on this trip, in order (for the map display).
    std::vector<CellIndex> shelvesLeft() const;
    const std::vector<PickUnit>& cargo() const { return cargo_; }
    const DockingStation* dock() const { return dock_; }

    // ---- Robot interface ----------------------------------------------------------
    void tick(TickContext& ctx) override;
    std::string stateLabel() const override;
    std::string activity() const override;
    bool isBusy() const override { return hasWork() || isCharging(); }

private:
    void resumeAfterCharging(TickContext& ctx);

    State state_ = State::Idle;
    CellIndex home_;
    Battery battery_;
    LinkedQueue<PickStop> route_;     ///< Shelves still to visit, in order (FIFO).
    std::vector<PickUnit> cargo_;     ///< Items already picked on this trip.
    CellIndex dropCell_ = kNoCell;    ///< Where to hand the cargo to the sorter.
    int sorterIndex_ = -1;
    DockingStation* dock_ = nullptr;  ///< Not owned: the Simulation owns the docks.
    int dwell_ = 0;                   ///< Ticks left of the current pick/drop.
    int stalledTicks_ = 0;            ///< Ticks spent with an empty battery.
    int tripId_ = 0;
    int waitStreak_ = 0;              ///< Consecutive ticks it wanted to move but could not.
    long totalWaits_ = 0;
    long moves_ = 0;
    bool movedThisTick_ = false;
};

} // namespace wh
