#pragma once
/**
 * @file Search.hpp
 * @brief Binary search on a sorted vector, iterative and recursive (Lecture 5a).
 *
 * `compare(element, key)` returns
 *     a negative number  if element comes BEFORE key  -> search the right half
 *     0                  if element matches key       -> found
 *     a positive number  if element comes AFTER key   -> search the left half
 *
 * Precondition: the vector is sorted in the same order that `compare` assumes.
 * Returns the index of a matching element, or -1 if there is none.
 *
 * Complexity: O(log n) comparisons. Each step halves [lo, hi]:
 *     T(n) = T(n/2) + O(1)  ->  O(log n).
 * The recursive version also uses O(log n) stack frames; the iterative one O(1).
 *
 * `mid = lo + (hi - lo) / 2` instead of `(lo + hi) / 2`: the same value, but it
 * cannot overflow when lo + hi is larger than the biggest int.
 */
#include <vector>

namespace wh {

template <typename T, typename Key, typename Compare>
int binarySearch(const std::vector<T>& a, const Key& key, Compare compare)
{
    int lo = 0;
    int hi = static_cast<int>(a.size()) - 1;
    while (lo <= hi) {
        const int mid = lo + (hi - lo) / 2;
        const int c = compare(a[mid], key);
        if (c == 0) return mid;
        if (c < 0) lo = mid + 1;   // middle element is too small: go right
        else       hi = mid - 1;   // middle element is too big: go left
    }
    return -1;                     // range is empty: not found
}

namespace detail {
template <typename T, typename Key, typename Compare>
int binarySearchRecursive(const std::vector<T>& a, const Key& key, Compare& compare, int lo, int hi)
{
    if (lo > hi) return -1;                       // base case: empty range
    const int mid = lo + (hi - lo) / 2;
    const int c = compare(a[mid], key);
    if (c == 0) return mid;                       // base case: found
    if (c < 0) return binarySearchRecursive(a, key, compare, mid + 1, hi);
    return binarySearchRecursive(a, key, compare, lo, mid - 1);
}
} // namespace detail

template <typename T, typename Key, typename Compare>
int binarySearchRecursive(const std::vector<T>& a, const Key& key, Compare compare)
{
    return detail::binarySearchRecursive(a, key, compare, 0, static_cast<int>(a.size()) - 1);
}

} // namespace wh
