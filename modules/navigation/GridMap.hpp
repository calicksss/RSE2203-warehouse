#pragma once
/**
 * @file GridMap.hpp
 * @brief The warehouse floor plan, loaded from data/Map.txt. It never changes after loading.
 *
 * Map symbols (spaces between symbols are ignored):
 *     W  wall               .  floor
 *     S  shelf (1 product)  C  charging dock (a robot parks ON it)
 *     R  sorting robot      T  picking robot home (floor)
 *
 * Graph view (Lecture: graph algorithms): every walkable cell (floor or dock)
 * is a vertex; two walkable cells that touch up/down/left/right share an edge
 * of length 1. Shelves, sorters and walls are not vertices, so robots go
 * around them. BFS and A* (Pathfinding.hpp) search this graph.
 *
 * Storage: one std::vector<Tile> of rows*cols entries, indexed by
 * CellIndex = row * cols + col (a 2D grid flattened into a 1D array).
 */
#include <iosfwd>
#include <string>
#include <vector>

#include "core/Types.hpp"

namespace wh {

enum class Tile { Wall, Floor, Shelf, Dock, Sorter };

class GridMap {
public:
    /// Reads and validates a map. Throws DataError with the line number if something is wrong.
    static GridMap parse(std::istream& in, const std::string& sourceName = "map");
    static GridMap loadFromFile(const std::string& path);

    int rows() const { return rows_; }
    int cols() const { return cols_; }
    int cellCount() const { return rows_ * cols_; }

    bool inBounds(GridPos p) const { return p.row >= 0 && p.row < rows_ && p.col >= 0 && p.col < cols_; }
    CellIndex index(GridPos p) const { return p.row * cols_ + p.col; }
    GridPos pos(CellIndex c) const { return {c / cols_, c % cols_}; }

    Tile tile(CellIndex c) const { return tiles_[static_cast<std::size_t>(c)]; }
    /// Robots can stand on floor and docks.
    bool isWalkable(CellIndex c) const { return tile(c) == Tile::Floor || tile(c) == Tile::Dock; }

    /// Writes the walkable up/down/left/right neighbours of `c` into out[] and returns how many (0..4).
    int neighbours(CellIndex c, CellIndex out[4]) const;

    const std::vector<CellIndex>& shelves() const { return shelves_; }
    const std::vector<CellIndex>& docks() const { return docks_; }
    const std::vector<CellIndex>& sorters() const { return sorters_; }
    const std::vector<CellIndex>& homes() const { return homes_; }   ///< 'T' cells, one per picker
    bool isHome(CellIndex c) const;

    /// Walkable cells next to a shelf or sorter, from which a robot can reach into it.
    std::vector<CellIndex> accessCells(CellIndex structure) const;

    /// The cell where pickers hand their items to the sorter at `sorterCell`.
    CellIndex inductionCellFor(CellIndex sorterCell) const;

    /// One character per cell, for debugging and tests.
    char symbol(CellIndex c) const;

private:
    GridMap() = default;
    void validate(const std::string& sourceName) const;

    int rows_ = 0;
    int cols_ = 0;
    std::vector<Tile> tiles_;
    std::vector<CellIndex> shelves_, docks_, sorters_, homes_;
};

} // namespace wh
