#pragma once
/**
 * @file MinHeap.hpp
 * @brief Priority queue stored as a binary min-heap inside a std::vector.
 *
 * The heap is a complete binary tree kept in an array (no pointers):
 *
 *     index:   0   1   2   3   4   5   6
 *     tree:            [0]
 *                    /     \
 *                 [1]       [2]
 *                /   \     /   \
 *              [3]   [4] [5]   [6]
 *
 *     parent(i) = (i - 1) / 2,  left(i) = 2i + 1,  right(i) = 2i + 2
 *
 * Heap rule: every parent is <= its children, so the smallest item is always
 * at index 0.
 *
 * Complexity (n items): push O(log n), pop O(log n), top O(1).
 * (A sorted array would give O(1) pop but O(n) push; an unsorted array O(1)
 * push but O(n) pop. The heap is the balance between the two.)
 *
 * T must support operator< ("a < b" means a is MORE urgent / comes out first).
 *
 * Used by:
 *   navigation/Pathfinding.cpp  A* open list (smallest f = g + h first)
 *   fulfilment/JobManager       order backlog (most urgent order first)
 */
#include <stdexcept>
#include <utility>
#include <vector>

namespace wh {

template <typename T>
class MinHeap {
public:
    /// Adds an item. O(log n): append at the end, then "sift up" while smaller than the parent.
    void push(const T& item)
    {
        data_.push_back(item);
        int child = static_cast<int>(data_.size()) - 1;
        while (child > 0) {
            const int parent = (child - 1) / 2;
            if (!(data_[child] < data_[parent])) break;   // heap rule holds: stop
            std::swap(data_[child], data_[parent]);
            child = parent;
        }
    }

    /// Removes and returns the smallest item. O(log n). Throws std::underflow_error if empty.
    T pop()
    {
        if (data_.empty()) throw std::underflow_error("MinHeap::pop on an empty heap");
        T smallest = data_[0];
        data_[0] = data_.back();   // move the last leaf to the root ...
        data_.pop_back();
        siftDown(0);               // ... and push it down to where it belongs
        return smallest;
    }

    /// The smallest item, without removing it. O(1). Throws std::underflow_error if empty.
    const T& top() const
    {
        if (data_.empty()) throw std::underflow_error("MinHeap::top on an empty heap");
        return data_[0];
    }

    bool empty() const { return data_.empty(); }
    int size() const { return static_cast<int>(data_.size()); }
    void clear() { data_.clear(); }

    /// Checks the heap rule for every parent/child pair. O(n). Used by the unit tests.
    bool isValidHeap() const
    {
        for (std::size_t child = 1; child < data_.size(); ++child)
            if (data_[child] < data_[(child - 1) / 2]) return false;
        return true;
    }

    /// Items in internal (array) order, e.g. for display. Not sorted.
    const std::vector<T>& items() const { return data_; }

private:
    void siftDown(int parent)
    {
        const int n = static_cast<int>(data_.size());
        while (true) {
            const int left = 2 * parent + 1;
            const int right = left + 1;
            int smallest = parent;
            if (left < n && data_[left] < data_[smallest]) smallest = left;
            if (right < n && data_[right] < data_[smallest]) smallest = right;
            if (smallest == parent) return;   // both children are >= parent: done
            std::swap(data_[parent], data_[smallest]);
            parent = smallest;
        }
    }

    std::vector<T> data_;
};

} // namespace wh
