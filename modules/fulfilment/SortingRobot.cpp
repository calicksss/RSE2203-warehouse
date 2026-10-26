#include "fulfilment/SortingRobot.hpp"

#include <algorithm>
#include <stdexcept>

#include "ds/Sorting.hpp"

namespace wh {

SortingRobot::SortingRobot(RobotId id, int sorterIndex, CellIndex cell, CellIndex inductionCell, int binCount)
    : Robot(id, "SRT-" + std::to_string(sorterIndex + 1), cell),
      index_(sorterIndex),
      induction_(inductionCell),
      bins_(static_cast<std::size_t>(std::max(1, binCount)))
{}

int SortingRobot::freeBins() const
{
    int n = 0;
    for (const Bin& b : bins_)
        if (b.order == kNoOrder) ++n;
    return n;
}

int SortingRobot::findBin(OrderId order) const
{
    for (std::size_t i = 0; i < bins_.size(); ++i)
        if (bins_[i].order == order) return static_cast<int>(i);
    return -1;
}

void SortingRobot::openBin(OrderId order, int expectedUnits)
{
    const int free = findBin(kNoOrder);
    if (free < 0) throw std::logic_error(name() + ": openBin called but every bin is in use");
    Bin& bin = bins_[static_cast<std::size_t>(free)];
    bin.order = order;
    bin.expected = expectedUnits;
    bin.contents.clear();
    bin.packTicksLeft = 0;
}

void SortingRobot::induct(const std::vector<PickUnit>& units)
{
    for (const PickUnit& u : units) conveyor_.enqueue(u);
}

bool SortingRobot::isBusy() const
{
    if (!conveyor_.empty()) return true;
    for (const Bin& b : bins_)
        if (b.packTicksLeft > 0) return true;
    return false;
}

std::string SortingRobot::stateLabel() const
{
    if (!conveyor_.empty()) return "SORTING";
    if (isBusy()) return "PACKING";
    return activeOrders() > 0 ? "WAITING" : "IDLE";
}

std::string SortingRobot::activity() const
{
    if (!conveyor_.empty())
        return "Sorting order #" + std::to_string(conveyor_.front().order) + " (" + std::to_string(conveyor_.size()) + " queued)";
    if (activeOrders() > 0) return std::to_string(activeOrders()) + " order(s) open";
    return "Waiting for work";
}

void SortingRobot::finishPacking(TickContext& ctx, Bin& bin)
{
    std::vector<PickUnit> packing = bin.contents;
    mergeSort(packing, [](const PickUnit& a, const PickUnit& b) { return a.weightKg > b.weightKg; });   // heaviest first, stable
    const OrderId order = bin.order;
    bin = Bin{};   // the bin is free again
    ctx.listener.onOrderPacked(order, packing, index_, ctx.now);
}

void SortingRobot::tick(TickContext& ctx)
{
    // 1) Packing: count down bins that are complete.
    for (Bin& bin : bins_) {
        if (bin.packTicksLeft <= 0) continue;
        if (--bin.packTicksLeft == 0) finishPacking(ctx, bin);
    }

    // 2) Sorting: move the front item of the conveyor into its order's bin.
    if (conveyor_.empty()) {
        progress_ = 0;
        return;
    }
    if (++progress_ < ctx.config.sortTicksPerUnit) return;
    progress_ = 0;

    const PickUnit unit = conveyor_.dequeue();
    const int b = findBin(unit.order);
    if (b < 0) throw std::logic_error(name() + ": item for order #" + std::to_string(unit.order) + " has no bin");
    Bin& bin = bins_[static_cast<std::size_t>(b)];
    bin.contents.push_back(unit);
    ++unitsSorted_;
    ctx.listener.onUnitSorted(unit, ctx.now);

    if (static_cast<int>(bin.contents.size()) == bin.expected) {
        bin.packTicksLeft = std::max(1, ctx.config.packTicks);
        log(ctx, Severity::Info, "Order #" + std::to_string(bin.order) + " complete - packing");
    }
}

} // namespace wh
