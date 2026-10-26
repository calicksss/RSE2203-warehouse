#pragma once
/**
 * @file DockingStation.hpp
 * @brief A charging dock that serves one robot at a time.
 *
 * The dock is RESERVED when the job manager sends a robot to it, not when the
 * robot arrives. Otherwise two robots could drive to the same free dock and
 * one would find it taken.
 */
#include "core/Types.hpp"

namespace wh {

class DockingStation {
public:
    DockingStation(int id, CellIndex cell, double powerPerTick) : id_(id), cell_(cell), powerPerTick_(powerPerTick) {}

    int id() const { return id_; }
    CellIndex cell() const { return cell_; }
    double powerPerTick() const { return powerPerTick_; }

    bool isFree() const { return holder_ == kNoRobot; }
    RobotId holder() const { return holder_; }

    /// Returns false if another robot already holds the dock.
    bool reserve(RobotId robot)
    {
        if (!isFree() && holder_ != robot) return false;
        holder_ = robot;
        return true;
    }
    void release(RobotId robot)
    {
        if (holder_ == robot) holder_ = kNoRobot;
    }

private:
    int id_;
    CellIndex cell_;
    double powerPerTick_;
    RobotId holder_ = kNoRobot;
};

} // namespace wh
