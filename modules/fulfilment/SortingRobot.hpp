#pragma once
/**
 * @file SortingRobot.hpp
 * @brief A fixed sorting arm with a "put wall": it splits mixed deliveries back into orders.
 *
 * A picker may bring items from SEVERAL orders in one trip. At the sorter:
 *   1. the items go onto a conveyor: a FIFO queue (ds::LinkedQueue);
 *   2. each tick (sortTicksPerUnit) the arm takes the front item and puts it
 *      into the bin reserved for that item's order;
 *   3. when a bin has all of its order's items, the order is packed: the
 *      items are sorted HEAVIEST FIRST so heavy items go at the bottom of the bag.
 *
 * Why merge sort for packing? It is stable: two items with the same weight
 * keep the order in which they were sorted into the bin, so the packing list
 * is predictable (and testable). Quick sort and selection sort are not stable.
 *
 * Finding an order's bin is a linear search over the bins: O(b). With b = 4
 * bins that is faster and simpler than a hash table.
 *
 * Bins are limited. The JobManager only sends an order to a sorter that has
 * a free bin, so every item that arrives always has a bin waiting.
 */
#include <string>
#include <vector>

#include "core/Robot.hpp"
#include "ds/LinkedQueue.hpp"

namespace wh {

class SortingRobot : public Robot {
public:
    struct Bin {
        OrderId order = kNoOrder;    ///< kNoOrder = free
        int expected = 0;            ///< items this order needs
        std::vector<PickUnit> contents;
        int packTicksLeft = 0;       ///< > 0 while being packed
    };

    SortingRobot(RobotId id, int sorterIndex, CellIndex cell, CellIndex inductionCell, int binCount);

    int index() const { return index_; }
    CellIndex inductionCell() const { return induction_; }
    int freeBins() const;
    int activeOrders() const { return static_cast<int>(bins_.size()) - freeBins(); }
    int queuedUnits() const { return conveyor_.size(); }
    long unitsSorted() const { return unitsSorted_; }
    const std::vector<Bin>& bins() const { return bins_; }

    /// Reserves a free bin for `order`. Throws std::logic_error if every bin is in use.
    void openBin(OrderId order, int expectedUnits);
    /// A picker hands over its items: they join the back of the conveyor.
    void induct(const std::vector<PickUnit>& units);

    void tick(TickContext& ctx) override;
    std::string stateLabel() const override;
    std::string activity() const override;
    bool isBusy() const override;

private:
    int findBin(OrderId order) const;   ///< index of the bin holding `order`, or -1
    void finishPacking(TickContext& ctx, Bin& bin);

    int index_;
    CellIndex induction_;
    LinkedQueue<PickUnit> conveyor_;
    int progress_ = 0;
    std::vector<Bin> bins_;
    long unitsSorted_ = 0;
};

} // namespace wh
