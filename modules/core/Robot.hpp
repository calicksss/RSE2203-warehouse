#pragma once
/**
 * @file Robot.hpp
 * @brief Abstract base class for all robots, plus the interface robots use to report progress.
 *
 * Lecture 1b (OO concepts):
 *  - Abstraction: Robot says WHAT every robot can do (tick, stateLabel, isBusy).
 *  - Inheritance: PickingRobot and SortingRobot derive from Robot.
 *  - Polymorphism: the Simulation keeps a std::vector<Robot*> and calls
 *    robot->tick() on each; C++ runs the PickingRobot or SortingRobot version.
 *  - Encapsulation: position and id are private/protected; changed only via methods.
 *
 * Why FulfilmentListener? Robots must tell the JobManager "I picked these items"
 * or "order 7 is packed". If robots called JobManager directly, the robot code
 * would depend on JobManager AND JobManager on the robots (a circular
 * dependency). Instead robots talk to this small abstract class, and JobManager
 * implements it. (This is the "Observer" design pattern.)
 */
#include <string>
#include <vector>

#include "core/Config.hpp"
#include "core/EventLog.hpp"
#include "core/WorkItem.hpp"

namespace wh {

class FulfilmentListener {
public:
    virtual ~FulfilmentListener() = default;
    virtual void onUnitsPicked(const std::vector<PickUnit>& units, Tick now) = 0;
    /// A picker has handed `units` to sorter number `sorterIndex`.
    virtual void onUnitsDelivered(int sorterIndex, const std::vector<PickUnit>& units, Tick now) = 0;
    virtual void onUnitSorted(const PickUnit& unit, Tick now) = 0;
    /// `packingList` is in packing order (heaviest first).
    virtual void onOrderPacked(OrderId order, const std::vector<PickUnit>& packingList, int sorterIndex, Tick now) = 0;
};

/// Everything a robot may use during one tick.
struct TickContext {
    Tick now;
    const SimConfig& config;
    EventLog& log;
    FulfilmentListener& listener;
};

class Robot {
public:
    Robot(RobotId id, const std::string& name, CellIndex cell) : cell_(cell), id_(id), name_(name) {}
    virtual ~Robot() = default;   // virtual: we delete robots through Robot* / unique_ptr<Robot>

    // A robot is a physical object with an identity: copying one makes no sense.
    Robot(const Robot&) = delete;
    Robot& operator=(const Robot&) = delete;

    /// Advance this robot by one tick (arrive, pick, sort, charge ...). Movement itself is
    /// done by the TrafficController, not here.
    virtual void tick(TickContext& ctx) = 0;

    /// Short upper-case state for tables, e.g. "PICKING".
    virtual std::string stateLabel() const = 0;
    /// One line describing what the robot is doing right now.
    virtual std::string activity() const = 0;
    virtual bool isBusy() const = 0;

    RobotId id() const { return id_; }
    const std::string& name() const { return name_; }
    CellIndex cell() const { return cell_; }

protected:
    void log(TickContext& ctx, Severity severity, const std::string& text) const { ctx.log.add(ctx.now, severity, name_, text); }

    CellIndex cell_;

private:
    RobotId id_;
    std::string name_;
};

} // namespace wh
