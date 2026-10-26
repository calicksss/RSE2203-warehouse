#pragma once
/**
 * @file Sorting.hpp
 * @brief The sorting algorithms from Lecture 4, written for std::vector with a comparator.
 *
 * Every function takes `less(a, b)`, which returns true when `a` must come
 * BEFORE `b`. Examples:
 *
 *     mergeSort(numbers);                                           // ascending (uses <)
 *     mergeSort(units, [](const PickUnit& a, const PickUnit& b) {   // heaviest first
 *         return a.weightKg > b.weightKg;
 *     });
 *
 * | Algorithm | Best       | Average    | Worst      | Extra memory | Stable? |
 * |-----------|------------|------------|------------|--------------|---------|
 * | bubble    | O(n)       | O(n^2)     | O(n^2)     | O(1)         | yes     |
 * | selection | O(n^2)     | O(n^2)     | O(n^2)     | O(1)         | no      |
 * | insertion | O(n)       | O(n^2)     | O(n^2)     | O(1)         | yes     |
 * | merge     | O(n log n) | O(n log n) | O(n log n) | O(n)         | yes     |
 * | quick     | O(n log n) | O(n log n) | O(n^2)     | O(log n)*    | no      |
 *   * stack depth, because we always recurse into the smaller half first.
 *
 * Stable = items that compare equal keep their original order. The sorting
 * robot needs this: it packs "heaviest first", and two items of equal weight
 * must stay in the order they were sorted into the bin. That is why the
 * program uses mergeSort there, not quickSort (see the ds tests).
 *
 * Where they are used: mergeSort in shop/Catalogue (names for binary search)
 * and fulfilment/SortingRobot (packing list); insertionSort in
 * picking/TrafficController (ordering at most a handful of robots, which are
 * already almost sorted from the previous tick: insertion sort's best case).
 * The other algorithms are compared in the experiments program.
 */
#include <functional>
#include <utility>
#include <vector>

namespace wh {

// ---------------------------------------------------------------------------
// Bubble sort: swap neighbours that are in the wrong order; stop early when a
// whole pass makes no swap (already sorted -> one pass -> O(n)).
// ---------------------------------------------------------------------------
template <typename T, typename Less>
void bubbleSort(std::vector<T>& a, Less less)
{
    const int n = static_cast<int>(a.size());
    for (int pass = 0; pass < n - 1; ++pass) {
        bool swapped = false;
        for (int i = 0; i < n - 1 - pass; ++i) {   // the last `pass` items are already in place
            if (less(a[i + 1], a[i])) {
                std::swap(a[i], a[i + 1]);
                swapped = true;
            }
        }
        if (!swapped) return;
    }
}

// ---------------------------------------------------------------------------
// Selection sort: find the smallest item of the unsorted part and swap it to
// the front of that part. Few swaps (at most n-1), but always O(n^2) compares.
// ---------------------------------------------------------------------------
template <typename T, typename Less>
void selectionSort(std::vector<T>& a, Less less)
{
    const int n = static_cast<int>(a.size());
    for (int i = 0; i < n - 1; ++i) {
        int smallest = i;
        for (int j = i + 1; j < n; ++j)
            if (less(a[j], a[smallest])) smallest = j;
        if (smallest != i) std::swap(a[i], a[smallest]);   // this long-distance swap makes it unstable
    }
}

// ---------------------------------------------------------------------------
// Insertion sort: take the next item and SHIFT bigger items one place right
// until its slot is found. Shifting (one assignment) is cheaper than swapping
// (three assignments). Very fast for small or nearly-sorted arrays.
// ---------------------------------------------------------------------------
template <typename T, typename Less>
void insertionSort(std::vector<T>& a, Less less)
{
    const int n = static_cast<int>(a.size());
    for (int i = 1; i < n; ++i) {
        T key = a[i];
        int j = i - 1;
        while (j >= 0 && less(key, a[j])) {   // strict "less": equal items are not passed -> stable
            a[j + 1] = a[j];
            --j;
        }
        a[j + 1] = key;
    }
}

// ---------------------------------------------------------------------------
// Merge sort (divide and conquer, Lecture 4 + 5b):
//   divide [lo, hi) in half, sort each half recursively, merge the halves.
//   T(n) = 2T(n/2) + O(n)  ->  O(n log n)   (Master method case 2, Lecture 5a)
// ---------------------------------------------------------------------------
namespace detail {

template <typename T, typename Less>
void mergeSortRange(std::vector<T>& a, std::vector<T>& buffer, int lo, int hi, Less& less)
{
    if (hi - lo <= 1) return;               // base case: 0 or 1 item is already sorted
    const int mid = lo + (hi - lo) / 2;
    mergeSortRange(a, buffer, lo, mid, less);
    mergeSortRange(a, buffer, mid, hi, less);

    // Merge a[lo, mid) and a[mid, hi) into buffer, then copy back.
    int i = lo, j = mid, k = lo;
    while (i < mid && j < hi) {
        // Take from the RIGHT half only if it is strictly smaller. On a tie we
        // take from the LEFT half, which keeps equal items in their original
        // order: this one comparison is what makes merge sort stable.
        if (less(a[j], a[i])) buffer[k++] = a[j++];
        else                  buffer[k++] = a[i++];
    }
    while (i < mid) buffer[k++] = a[i++];   // copy whatever is left
    while (j < hi)  buffer[k++] = a[j++];
    for (k = lo; k < hi; ++k) a[k] = buffer[k];
}

} // namespace detail

template <typename T, typename Less>
void mergeSort(std::vector<T>& a, Less less)
{
    std::vector<T> buffer(a.size());        // the O(n) extra memory
    detail::mergeSortRange(a, buffer, 0, static_cast<int>(a.size()), less);
}

// ---------------------------------------------------------------------------
// Quick sort (Lecture 4), using the lecture's partition scheme:
//   pivot = a[lo]; pivotIndex walks right and collects every item < pivot;
//   finally the pivot is swapped into a[pivotIndex].
// Choice of pivot (Lecture 4, slide "Choice of Pivot"): the first item gives
// O(n^2) on already-sorted input, so we first move the MEDIAN of the first,
// middle and last items into a[lo].
// ---------------------------------------------------------------------------
namespace detail {

template <typename T, typename Less>
int partition(std::vector<T>& a, int lo, int hi, Less& less)
{
    const int mid = lo + (hi - lo) / 2;
    // Median of three: order a[lo], a[mid], a[hi], then use the middle one.
    if (less(a[mid], a[lo])) std::swap(a[mid], a[lo]);
    if (less(a[hi], a[lo]))  std::swap(a[hi], a[lo]);
    if (less(a[hi], a[mid])) std::swap(a[hi], a[mid]);
    std::swap(a[lo], a[mid]);               // median is now the pivot at a[lo]

    const T pivot = a[lo];
    int pivotIndex = lo;
    for (int index = lo + 1; index <= hi; ++index) {
        if (less(a[index], pivot)) {        // belongs in the left half
            ++pivotIndex;
            std::swap(a[pivotIndex], a[index]);
        }
    }
    std::swap(a[lo], a[pivotIndex]);        // move the pivot into its final place
    return pivotIndex;
}

template <typename T, typename Less>
void quickSortRange(std::vector<T>& a, int lo, int hi, Less& less)
{
    // Recurse into the SMALLER half and loop on the larger one, so the
    // recursion depth stays O(log n) even in a bad case.
    while (lo < hi) {
        const int p = partition(a, lo, hi, less);
        if (p - lo < hi - p) {
            quickSortRange(a, lo, p - 1, less);
            lo = p + 1;
        } else {
            quickSortRange(a, p + 1, hi, less);
            hi = p - 1;
        }
    }
}

} // namespace detail

template <typename T, typename Less>
void quickSort(std::vector<T>& a, Less less)
{
    if (a.size() > 1) detail::quickSortRange(a, 0, static_cast<int>(a.size()) - 1, less);
}

// ---------------------------------------------------------------------------
// Convenience overloads: ascending order using operator<.
// ---------------------------------------------------------------------------
template <typename T> void bubbleSort(std::vector<T>& a)    { bubbleSort(a, std::less<T>()); }
template <typename T> void selectionSort(std::vector<T>& a) { selectionSort(a, std::less<T>()); }
template <typename T> void insertionSort(std::vector<T>& a) { insertionSort(a, std::less<T>()); }
template <typename T> void mergeSort(std::vector<T>& a)     { mergeSort(a, std::less<T>()); }
template <typename T> void quickSort(std::vector<T>& a)     { quickSort(a, std::less<T>()); }

} // namespace wh
