/**
 * @file test_ds.cpp
 * @brief Tests for the ds module: LinkedQueue, MinHeap, sorting and binary search.
 *
 * Validation idea used throughout: compare our implementation against a
 * trusted reference (std::queue, std::sort, std::stable_sort) on many random
 * inputs. If both always agree, our version is very likely correct.
 */
#include <algorithm>
#include <cmath>
#include <queue>
#include <random>
#include <string>
#include <vector>

#include "TestFramework.hpp"
#include "ds/LinkedQueue.hpp"
#include "ds/MinHeap.hpp"
#include "ds/Search.hpp"
#include "ds/Sorting.hpp"

using namespace wh;

namespace {

std::vector<int> randomInts(int n, int maxValue, unsigned seed)
{
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(0, maxValue);
    std::vector<int> v(n);
    for (int& x : v) x = dist(rng);
    return v;
}

/// An item with a key (what we sort by) and a label (to check stability).
struct Tagged {
    int key;
    char label;
};
bool byKey(const Tagged& a, const Tagged& b) { return a.key < b.key; }

} // namespace

// ---------------------------------------------------------------------------
// LinkedQueue
// ---------------------------------------------------------------------------

TEST_CASE("LinkedQueue: lecture 2 example keeps first-in-first-out order")
{
    LinkedQueue<std::string> q;
    q.enqueue("apple");
    q.enqueue("pear");
    q.enqueue("grape");
    CHECK(q.dequeue() == "apple");
    q.enqueue("banana");
    CHECK(q.dequeue() == "pear");
    CHECK(q.size() == 2);
    CHECK(q.front() == "grape");
    CHECK(q.dequeue() == "grape");
    CHECK(q.dequeue() == "banana");
    CHECK(q.empty());
}

TEST_CASE("LinkedQueue: dequeue and front on an empty queue throw")
{
    LinkedQueue<int> q;
    CHECK_THROWS(q.dequeue());
    CHECK_THROWS(q.front());
    q.enqueue(1);
    q.dequeue();
    CHECK_THROWS(q.dequeue());   // empty again after removing the last node (tail reset)
    q.enqueue(2);                // and still usable
    CHECK(q.front() == 2);
}

TEST_CASE("LinkedQueue: behaves exactly like std::queue over 10,000 random operations")
{
    LinkedQueue<int> mine;
    std::queue<int> reference;
    std::mt19937 rng(7);
    for (int step = 0; step < 10000; ++step) {
        if (reference.empty() || rng() % 3 != 0) {
            const int v = static_cast<int>(rng() % 1000);
            mine.enqueue(v);
            reference.push(v);
        } else {
            REQUIRE(mine.front() == reference.front());
            CHECK(mine.dequeue() == reference.front());
            reference.pop();
        }
        REQUIRE(mine.size() == static_cast<int>(reference.size()));
    }
}

TEST_CASE("LinkedQueue: copies are independent (deep copy, rule of three)")
{
    LinkedQueue<int> a;
    for (int i = 1; i <= 3; ++i) a.enqueue(i);
    LinkedQueue<int> b = a;        // copy constructor
    b.dequeue();
    CHECK(a.size() == 3);
    CHECK(a.front() == 1);
    LinkedQueue<int> c;
    c.enqueue(99);
    c = a;                         // copy assignment replaces old contents
    CHECK(c.size() == 3);
    std::vector<int> seen;
    c.forEach([&](int v) { seen.push_back(v); });
    CHECK((seen == std::vector<int>{1, 2, 3}));
}

// ---------------------------------------------------------------------------
// MinHeap
// ---------------------------------------------------------------------------

TEST_CASE("MinHeap: pops items in ascending order (checked against std::sort)")
{
    for (unsigned seed = 1; seed <= 20; ++seed) {
        std::vector<int> values = randomInts(200, 50, seed);   // many duplicates on purpose
        MinHeap<int> heap;
        for (int v : values) {
            heap.push(v);
            REQUIRE(heap.isValidHeap());
        }
        std::sort(values.begin(), values.end());
        for (int expected : values) {
            REQUIRE(heap.top() == expected);
            CHECK(heap.pop() == expected);
            REQUIRE(heap.isValidHeap());
        }
        CHECK(heap.empty());
    }
}

TEST_CASE("MinHeap: pop and top on an empty heap throw")
{
    MinHeap<int> heap;
    CHECK_THROWS(heap.pop());
    CHECK_THROWS(heap.top());
}

TEST_CASE("MinHeap: works with a struct that defines operator<")
{
    struct Job {
        int priority;
        int id;
        bool operator<(const Job& o) const { return priority != o.priority ? priority < o.priority : id < o.id; }
    };
    MinHeap<Job> heap;
    heap.push({5, 1});
    heap.push({1, 2});
    heap.push({5, 0});
    heap.push({3, 3});
    CHECK(heap.pop().id == 2);
    CHECK(heap.pop().id == 3);
    CHECK(heap.pop().id == 0);   // tie on priority 5 broken by smaller id
    CHECK(heap.pop().id == 1);
}

// ---------------------------------------------------------------------------
// Sorting
// ---------------------------------------------------------------------------

TEST_CASE("Sorting: every algorithm sorts the lecture 4 quick-sort example")
{
    const std::vector<int> input{8, 3, 7, 11, 9, 5, 13, 2, 20, 1};
    const std::vector<int> expected{1, 2, 3, 5, 7, 8, 9, 11, 13, 20};
    std::vector<int> a = input, b = input, c = input, d = input, e = input;
    bubbleSort(a);
    selectionSort(b);
    insertionSort(c);
    mergeSort(d);
    quickSort(e);
    CHECK(a == expected);
    CHECK(b == expected);
    CHECK(c == expected);
    CHECK(d == expected);
    CHECK(e == expected);
}

TEST_CASE("Sorting: all algorithms agree with std::sort on random, sorted, reversed and duplicate inputs")
{
    std::vector<std::vector<int>> inputs;
    inputs.push_back({});
    inputs.push_back({42});
    inputs.push_back({2, 1});
    for (unsigned seed = 1; seed <= 10; ++seed) inputs.push_back(randomInts(300, 1000, seed));
    std::vector<int> sorted = randomInts(300, 1000, 99);
    std::sort(sorted.begin(), sorted.end());
    inputs.push_back(sorted);
    inputs.push_back(std::vector<int>(sorted.rbegin(), sorted.rend()));
    inputs.push_back(randomInts(300, 3, 5));          // only 4 distinct values
    inputs.push_back(std::vector<int>(300, 7));       // all equal

    for (const auto& input : inputs) {
        std::vector<int> expected = input;
        std::sort(expected.begin(), expected.end());
        std::vector<int> a = input, b = input, c = input, d = input, e = input;
        bubbleSort(a);
        selectionSort(b);
        insertionSort(c);
        mergeSort(d);
        quickSort(e);
        CHECK(a == expected);
        CHECK(b == expected);
        CHECK(c == expected);
        CHECK(d == expected);
        CHECK(e == expected);
    }
}

TEST_CASE("Sorting: a custom comparator sorts in descending order")
{
    std::vector<double> weights{0.2, 2.5, 1.0, 0.15, 2.5, 0.7};
    mergeSort(weights, [](double a, double b) { return a > b; });
    CHECK((weights == std::vector<double>{2.5, 2.5, 1.0, 0.7, 0.2, 0.15}));
}

TEST_CASE("Sorting: merge, insertion and bubble sort are stable (equal keys keep their order)")
{
    for (unsigned seed = 1; seed <= 20; ++seed) {
        std::mt19937 rng(seed);
        std::vector<Tagged> input;
        for (int i = 0; i < 100; ++i) input.push_back({static_cast<int>(rng() % 5), static_cast<char>('A' + i % 26)});
        std::vector<Tagged> expected = input;
        std::stable_sort(expected.begin(), expected.end(), byKey);   // the trusted stable reference

        std::vector<Tagged> m = input, ins = input, bub = input;
        mergeSort(m, byKey);
        insertionSort(ins, byKey);
        bubbleSort(bub, byKey);
        for (std::size_t i = 0; i < expected.size(); ++i) {
            CHECK(m[i].key == expected[i].key && m[i].label == expected[i].label);
            CHECK(ins[i].key == expected[i].key && ins[i].label == expected[i].label);
            CHECK(bub[i].key == expected[i].key && bub[i].label == expected[i].label);
        }
    }
}

TEST_CASE("Sorting: selection sort is NOT stable (lecture 4 claim, shown on a tiny example)")
{
    // keys: 2a 2b 1c. Pass 1 swaps 1c with 2a -> 1c 2b 2a: the two 2s changed order.
    std::vector<Tagged> v{{2, 'a'}, {2, 'b'}, {1, 'c'}};
    selectionSort(v, byKey);
    CHECK(v[0].label == 'c');
    CHECK(v[1].label == 'b');   // 'b' now comes before 'a'
    CHECK(v[2].label == 'a');
}

TEST_CASE("Sorting: comparison counts match the best/worst cases from lecture 4")
{
    const int n = 1000;
    std::vector<int> sorted(n);
    for (int i = 0; i < n; ++i) sorted[i] = i;

    long compares = 0;
    auto counting = [&compares](int a, int b) { ++compares; return a < b; };

    std::vector<int> v = sorted;
    insertionSort(v, counting);
    CHECK(compares == n - 1);                   // best case: already sorted -> O(n)

    compares = 0;
    v = sorted;
    bubbleSort(v, counting);
    CHECK(compares == n - 1);                   // one pass, no swaps, stop early

    compares = 0;
    v = std::vector<int>(sorted.rbegin(), sorted.rend());
    insertionSort(v, counting);
    CHECK(compares == static_cast<long>(n) * (n - 1) / 2);   // worst case: reversed -> n(n-1)/2

    compares = 0;
    v = randomInts(n, 100000, 3);
    mergeSort(v, counting);
    CHECK(compares <= static_cast<long>(n * std::log2(n)));  // never more than n log2 n
}

// ---------------------------------------------------------------------------
// Binary search
// ---------------------------------------------------------------------------

namespace {
int compareInt(int element, int key) { return element < key ? -1 : (element > key ? 1 : 0); }
}

TEST_CASE("Binary search: lecture 5a example finds 51 at index 6")
{
    const std::vector<int> a{6, 13, 14, 25, 33, 43, 51, 53, 64, 72};
    CHECK(binarySearch(a, 51, compareInt) == 6);
    CHECK(binarySearchRecursive(a, 51, compareInt) == 6);
    CHECK(binarySearch(a, 6, compareInt) == 0);     // first element
    CHECK(binarySearch(a, 72, compareInt) == 9);    // last element
    CHECK(binarySearch(a, 50, compareInt) == -1);   // missing, in the middle
    CHECK(binarySearch(a, 1, compareInt) == -1);    // smaller than everything
    CHECK(binarySearch(a, 99, compareInt) == -1);   // larger than everything
}

TEST_CASE("Binary search: empty vector returns -1")
{
    const std::vector<int> empty;
    CHECK(binarySearch(empty, 5, compareInt) == -1);
    CHECK(binarySearchRecursive(empty, 5, compareInt) == -1);
}

TEST_CASE("Binary search: iterative and recursive versions agree with a linear search")
{
    std::vector<int> a = randomInts(500, 2000, 11);
    mergeSort(a);
    a.erase(std::unique(a.begin(), a.end()), a.end());   // distinct values, so the index is unique
    for (int key = -5; key <= 2005; ++key) {
        const auto it = std::find(a.begin(), a.end(), key);
        const int expected = it == a.end() ? -1 : static_cast<int>(it - a.begin());
        REQUIRE(binarySearch(a, key, compareInt) == expected);
        REQUIRE(binarySearchRecursive(a, key, compareInt) == expected);
    }
}
