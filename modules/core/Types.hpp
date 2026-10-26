#pragma once
/**
 * @file Types.hpp
 * @brief Small vocabulary types used by every module.
 *
 * A grid cell is stored as ONE integer, `CellIndex = row * cols + col`.
 * This lets every per-cell table (distances, obstacles, who stands where) be a
 * plain std::vector indexed by the cell, which is an array: O(1) access
 * (Lecture 2, arrays). GridMap::pos() converts back to (row, col).
 */
#include <string>

namespace wh {

using Tick = long long;   ///< One simulation step. A robot moves at most one cell per tick.
using RobotId = int;
using OrderId = int;
using CellIndex = int;    ///< row * cols + col

const CellIndex kNoCell = -1;
const RobotId kNoRobot = -1;
const OrderId kNoOrder = -1;

/// "Infinite" distance. Small enough that adding two of them cannot overflow an int.
const int kUnreachable = 1000000000 / 4;

/// A (row, col) coordinate. Rows grow downwards, columns grow to the right.
struct GridPos {
    int row = 0;
    int col = 0;
};

inline bool operator==(GridPos a, GridPos b) { return a.row == b.row && a.col == b.col; }
inline bool operator!=(GridPos a, GridPos b) { return !(a == b); }

inline std::string toString(GridPos p) { return "(" + std::to_string(p.row) + "," + std::to_string(p.col) + ")"; }

} // namespace wh
