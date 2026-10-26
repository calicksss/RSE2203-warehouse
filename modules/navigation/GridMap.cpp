#include "navigation/GridMap.hpp"

#include <algorithm>
#include <fstream>

#include "core/Errors.hpp"
#include "navigation/Pathfinding.hpp"

namespace wh {

GridMap GridMap::parse(std::istream& in, const std::string& sourceName)
{
    GridMap map;
    std::vector<std::string> rowsText;
    std::vector<int> lineNumbers;

    std::string line;
    int lineNo = 0;
    while (std::getline(in, line)) {
        ++lineNo;
        std::string row;
        for (char ch : line)
            if (ch != ' ' && ch != '\t' && ch != '\r') row.push_back(ch);
        if (row.empty() || row[0] == '#') continue;   // blank line or comment
        if (!rowsText.empty() && row.size() != rowsText[0].size())
            throw DataError(sourceName, lineNo, "row has " + std::to_string(row.size()) + " cells, expected " +
                                                    std::to_string(rowsText[0].size()) + " (the map must be a rectangle)");
        rowsText.push_back(row);
        lineNumbers.push_back(lineNo);
    }
    if (rowsText.empty()) throw DataError(sourceName, 0, "map is empty");

    map.rows_ = static_cast<int>(rowsText.size());
    map.cols_ = static_cast<int>(rowsText[0].size());
    map.tiles_.assign(static_cast<std::size_t>(map.cellCount()), Tile::Wall);

    for (int r = 0; r < map.rows_; ++r) {
        for (int c = 0; c < map.cols_; ++c) {
            const CellIndex idx = map.index({r, c});
            Tile& t = map.tiles_[static_cast<std::size_t>(idx)];
            const char ch = rowsText[static_cast<std::size_t>(r)][static_cast<std::size_t>(c)];
            switch (ch) {
                case 'W': t = Tile::Wall; break;
                case '.': t = Tile::Floor; break;
                case 'S': t = Tile::Shelf;  map.shelves_.push_back(idx); break;
                case 'C': t = Tile::Dock;   map.docks_.push_back(idx); break;
                case 'R': t = Tile::Sorter; map.sorters_.push_back(idx); break;
                case 'T': t = Tile::Floor;  map.homes_.push_back(idx); break;
                default:
                    throw DataError(sourceName, lineNumbers[static_cast<std::size_t>(r)],
                                    std::string("unknown map symbol '") + ch + "'");
            }
        }
    }
    map.validate(sourceName);
    return map;
}

GridMap GridMap::loadFromFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file) throw DataError(path, 0, "cannot open map file");
    return parse(file, path);
}

int GridMap::neighbours(CellIndex c, CellIndex out[4]) const
{
    const GridPos p = pos(c);
    const GridPos candidates[4] = {{p.row - 1, p.col}, {p.row + 1, p.col}, {p.row, p.col - 1}, {p.row, p.col + 1}};
    int n = 0;
    for (const GridPos& q : candidates)
        if (inBounds(q) && isWalkable(index(q))) out[n++] = index(q);
    return n;
}

bool GridMap::isHome(CellIndex c) const
{
    return std::find(homes_.begin(), homes_.end(), c) != homes_.end();
}

std::vector<CellIndex> GridMap::accessCells(CellIndex structure) const
{
    CellIndex nb[4];
    const int n = neighbours(structure, nb);   // neighbours() already returns only walkable cells
    return std::vector<CellIndex>(nb, nb + n);
}

CellIndex GridMap::inductionCellFor(CellIndex sorterCell) const
{
    // Prefer a plain floor cell that is not a dock or a robot home, so robots
    // queueing for the sorter do not block parked robots. Among those, pick the
    // most open one (most neighbours) so the queue has room.
    CellIndex best = kNoCell;
    int bestDegree = -1;
    CellIndex nb[4];
    for (CellIndex cand : accessCells(sorterCell)) {
        if (tile(cand) == Tile::Dock || isHome(cand)) continue;
        const int degree = neighbours(cand, nb);
        if (degree > bestDegree) {
            best = cand;
            bestDegree = degree;
        }
    }
    if (best == kNoCell) {
        const std::vector<CellIndex> any = accessCells(sorterCell);
        if (!any.empty()) best = any[0];
    }
    return best;
}

char GridMap::symbol(CellIndex c) const
{
    switch (tile(c)) {
        case Tile::Wall:   return 'W';
        case Tile::Shelf:  return 'S';
        case Tile::Dock:   return 'C';
        case Tile::Sorter: return 'R';
        case Tile::Floor:  return isHome(c) ? 'T' : '.';
    }
    return '?';
}

void GridMap::validate(const std::string& sourceName) const
{
    if (homes_.empty()) throw DataError(sourceName, 0, "map needs at least one picker home 'T'");
    if (sorters_.empty()) throw DataError(sourceName, 0, "map needs at least one sorting robot 'R'");
    if (docks_.empty()) throw DataError(sourceName, 0, "map needs at least one charging dock 'C'");

    // Every place a robot must visit has to be reachable from the first home.
    // One BFS gives the distance to every cell; "unreachable" means a bad map.
    const std::vector<int> dist = bfsDistances(*this, homes_[0]);
    auto reachable = [&](CellIndex c) { return dist[static_cast<std::size_t>(c)] < kUnreachable; };

    for (CellIndex s : shelves_) {
        bool ok = false;
        for (CellIndex a : accessCells(s)) ok = ok || reachable(a);
        if (!ok) throw DataError(sourceName, pos(s).row + 1, "shelf at " + toString(pos(s)) + " cannot be reached from any side");
    }
    for (CellIndex r : sorters_) {
        const CellIndex in = inductionCellFor(r);
        if (in == kNoCell || !reachable(in))
            throw DataError(sourceName, pos(r).row + 1, "sorter at " + toString(pos(r)) + " has no reachable floor cell next to it");
    }
    for (CellIndex d : docks_)
        if (!reachable(d)) throw DataError(sourceName, pos(d).row + 1, "dock at " + toString(pos(d)) + " is not reachable");
    for (CellIndex h : homes_)
        if (!reachable(h)) throw DataError(sourceName, pos(h).row + 1, "home at " + toString(pos(h)) + " is not reachable");
}

} // namespace wh
