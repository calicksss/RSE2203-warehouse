#pragma once
/**
 * @file WorkItem.hpp
 * @brief The units of warehouse work that move between modules.
 *
 *   Customer Order (shop) --JobManager splits--> PickUnit (one physical item)
 *   PickUnits on the same shelf --grouped into--> PickStop (one shelf visit)
 *   A list of PickStops + a sorter = one picker trip.
 */
#include <vector>

#include "core/Types.hpp"

namespace wh {

/// One physical item that must be fetched from a shelf and brought to a sorter.
struct PickUnit {
    OrderId order = kNoOrder;
    int productIndex = 0;
    CellIndex shelf = kNoCell;
    double weightKg = 0.0;
    Tick priorityKey = 0;   ///< Smaller = more urgent (see SimConfig::expressHeadStart).
};

/// One shelf visit inside a picker trip.
struct PickStop {
    CellIndex shelf = kNoCell;    ///< The shelf cell (robots cannot stand on it).
    CellIndex access = kNoCell;   ///< The floor cell next to the shelf where the robot stops.
    std::vector<PickUnit> units;  ///< Items to take from this shelf.
};

} // namespace wh
