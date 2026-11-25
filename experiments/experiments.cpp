/**
 * @file experiments.cpp
 * @brief Experiments for the report ("Experiment, analysis and insight discussion", 20%).
 *
 *   experiments sorting       E1  sorting algorithms: comparisons and time vs n       (WP-A)
 *   experiments search        E2  binary vs linear search on the catalogue            (WP-B)
 *   experiments pathfinding   E3  BFS vs A*: cells expanded and time                  (WP-C)
 *   experiments routing       E4  as-given vs greedy vs backtracking routes           (WP-C)
 *   experiments traffic       E5  robots 1..8: throughput and congestion              (WP-D)
 *   experiments policies      E6  routing x robot-choice policies, whole warehouse    (WP-E)
 *   experiments fulfilment    E7  trip filling: first-fit vs first-fit decreasing vs exact (WP-E)
 *   experiments trees         E8  BST vs LLRBT vs hash table: lookups vs insertion order (WP-B)
 *   experiments substring     E9  search box: linear scan vs suffix array (sorting + binary search) (WP-B)
 *   experiments improve       E10  greedy vs greedy + 2-opt vs exact routes (WP-C)
 *   experiments all           everything
 *
 * Every experiment prints a summary table and writes a CSV file into
 * experiments/results/ (or the folder given with --out DIR), ready for a chart.
 * Random inputs use fixed seeds, so counts (comparisons, cells, moves) repeat
 * exactly with the same compiler. (std::uniform_int_distribution is allowed to
 * differ between standard libraries, so Visual Studio gives slightly different
 * random orders than GCC/Clang.) Times (ms, us) depend on the computer: compare
 * them only within one run (Lecture 3: limits of empirical analysis).
 */
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "ds/Search.hpp"
#include "ds/Sorting.hpp"
#include "navigation/DistanceTable.hpp"
#include "navigation/Pathfinding.hpp"
#include "navigation/RoutePlanner.hpp"
#include "shop/Shop.hpp"
#include "sim/Simulation.hpp"

#ifndef WH_DATA_DIR
#define WH_DATA_DIR "data"
#endif
#ifndef WH_RESULTS_DIR
#define WH_RESULTS_DIR "results"
#endif

using namespace wh;
using Clock = std::chrono::steady_clock;

namespace {

std::string g_outDir = WH_RESULTS_DIR;

double msSince(Clock::time_point start) { return std::chrono::duration<double, std::milli>(Clock::now() - start).count(); }

std::ofstream openCsv(const std::string& name, const std::string& header)
{
    std::filesystem::create_directories(g_outDir);
    const std::string path = (std::filesystem::path(g_outDir) / name).string();
    std::ofstream out(path);
    out << header << '\n';
    std::cout << "  -> writing " << path << '\n';
    return out;
}

std::string fmt(double v, int decimals)
{
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.*f", decimals, v);
    return buf;
}

const GridMap& warehouseMap()
{
    static const GridMap map = GridMap::loadFromFile(std::string(WH_DATA_DIR) + "/Map.txt");
    return map;
}
const Catalogue& warehouseCatalogue()
{
    static const Catalogue catalogue = Catalogue::loadFromFile(std::string(WH_DATA_DIR) + "/Products.txt");
    return catalogue;
}

// ===========================================================================
// E1 Sorting
// ===========================================================================

std::vector<int> makeInput(const std::string& kind, int n, unsigned seed)
{
    std::mt19937 rng(seed);
    std::vector<int> v(static_cast<std::size_t>(n));
    if (kind == "random") {
        std::uniform_int_distribution<int> d(0, 1000000);
        for (int& x : v) x = d(rng);
    } else if (kind == "sorted") {
        for (int i = 0; i < n; ++i) v[static_cast<std::size_t>(i)] = i;
    } else if (kind == "reversed") {
        for (int i = 0; i < n; ++i) v[static_cast<std::size_t>(i)] = n - i;
    } else {   // "few-unique": only 4 different values (many duplicates)
        std::uniform_int_distribution<int> d(0, 3);
        for (int& x : v) x = d(rng);
    }
    return v;
}

void experimentSorting()
{
    std::cout << "\nE1  SORTING: comparisons and time (median of 3 runs)\n";
    using SortFn = std::function<void(std::vector<int>&, std::function<bool(int, int)>)>;
    struct Algo {
        std::string name;
        SortFn sort;
        bool quadratic;
    };
    const std::vector<Algo> algos{
        {"bubble", [](std::vector<int>& v, std::function<bool(int, int)> less) { bubbleSort(v, less); }, true},
        {"selection", [](std::vector<int>& v, std::function<bool(int, int)> less) { selectionSort(v, less); }, true},
        {"insertion", [](std::vector<int>& v, std::function<bool(int, int)> less) { insertionSort(v, less); }, true},
        {"merge", [](std::vector<int>& v, std::function<bool(int, int)> less) { mergeSort(v, less); }, false},
        {"quick", [](std::vector<int>& v, std::function<bool(int, int)> less) { quickSort(v, less); }, false},
        {"std::sort", [](std::vector<int>& v, std::function<bool(int, int)> less) { std::sort(v.begin(), v.end(), less); }, false},
    };
    const std::vector<int> sizes{100, 1000, 5000, 10000, 20000, 50000, 100000, 200000};
    const std::vector<std::string> kinds{"random", "sorted", "reversed", "few-unique"};

    std::ofstream csv = openCsv("e1_sorting.csv", "algorithm,input,n,comparisons,ms");
    std::cout << std::left << std::setw(11) << "algorithm" << std::setw(12) << "input" << std::right << std::setw(8) << "n"
              << std::setw(15) << "comparisons" << std::setw(12) << "ms" << std::setw(14) << "comp/(n lg n)" << '\n';
    for (const Algo& algo : algos) {
        for (const std::string& kind : kinds) {
            for (int n : sizes) {
                if (algo.quadratic && n > 20000) continue;                       // O(n^2): would take minutes
                if (algo.name == "quick" && kind == "few-unique" && n > 20000) continue;   // see README: duplicates hurt this partition
                long comparisons = 0;
                std::vector<double> times;
                for (int rep = 0; rep < 3; ++rep) {
                    std::vector<int> v = makeInput(kind, n, 42);
                    long count = 0;
                    const auto start = Clock::now();
                    algo.sort(v, [&count](int a, int b) { ++count; return a < b; });
                    times.push_back(msSince(start));
                    comparisons = count;
                    if (!std::is_sorted(v.begin(), v.end())) std::cout << "  !! " << algo.name << " did not sort " << kind << '\n';
                }
                std::sort(times.begin(), times.end());
                const double ms = times[1];
                csv << algo.name << ',' << kind << ',' << n << ',' << comparisons << ',' << fmt(ms, 4) << '\n';
                if (n == 10000 || n == 100000)
                    std::cout << std::left << std::setw(11) << algo.name << std::setw(12) << kind << std::right << std::setw(8) << n
                              << std::setw(15) << comparisons << std::setw(12) << fmt(ms, 3) << std::setw(14)
                              << fmt(comparisons / (n * std::log2(n)), 2) << '\n';
            }
        }
    }
}

// ===========================================================================
// E2 Searching the catalogue
// ===========================================================================

void experimentSearch()
{
    std::cout << "\nE2  SEARCH: binary vs linear search, comparisons per lookup\n";
    std::ofstream csv = openCsv("e2_search.csv", "n,linear_avg_comparisons,binary_avg_comparisons,binary_max_comparisons,log2n");
    std::cout << std::right << std::setw(9) << "n" << std::setw(14) << "linear avg" << std::setw(14) << "binary avg" << std::setw(14)
              << "binary max" << std::setw(10) << "log2 n" << '\n';
    for (int n : {91, 1000, 10000, 100000, 1000000}) {
        std::vector<int> a(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) a[static_cast<std::size_t>(i)] = 2 * i;   // sorted, even numbers
        std::mt19937 rng(5);
        std::uniform_int_distribution<int> pick(0, n - 1);
        const int lookups = 2000;
        long linearTotal = 0, binaryTotal = 0, binaryMax = 0;
        for (int q = 0; q < lookups; ++q) {
            const int key = 2 * pick(rng);
            long linear = 0;
            for (int x : a) {
                ++linear;
                if (x == key) break;
            }
            long binary = 0;
            const int idx = binarySearch(a, key, [&binary](int element, int k) { ++binary; return element < k ? -1 : (element > k ? 1 : 0); });
            if (a[static_cast<std::size_t>(idx)] != key) std::cout << "  !! binary search returned a wrong index\n";
            linearTotal += linear;
            binaryTotal += binary;
            binaryMax = std::max(binaryMax, binary);
        }
        const double lin = double(linearTotal) / lookups, bin = double(binaryTotal) / lookups;
        csv << n << ',' << fmt(lin, 2) << ',' << fmt(bin, 2) << ',' << binaryMax << ',' << fmt(std::log2(n), 2) << '\n';
        std::cout << std::setw(9) << n << std::setw(14) << fmt(lin, 1) << std::setw(14) << fmt(bin, 2) << std::setw(14) << binaryMax
                  << std::setw(10) << fmt(std::log2(n), 2) << '\n';
    }
}

// ===========================================================================
// E3 Path finding: BFS vs A*
// ===========================================================================

void experimentPathfinding()
{
    std::cout << "\nE3  PATH FINDING: BFS vs A* on the warehouse map (1000 random start/goal pairs)\n";
    const GridMap& map = warehouseMap();
    std::vector<CellIndex> cells;
    for (CellIndex c = 0; c < map.cellCount(); ++c)
        if (map.isWalkable(c)) cells.push_back(c);

    std::ofstream csv = openCsv("e3_pathfinding.csv", "obstacles,pair,path_length,bfs_expanded,astar_expanded,bfs_us,astar_us");
    for (int obstacleCount : {0, 20}) {
        std::mt19937 rng(7);
        std::uniform_int_distribution<std::size_t> pick(0, cells.size() - 1);
        std::vector<char> blocked(static_cast<std::size_t>(map.cellCount()), 0);
        for (int k = 0; k < obstacleCount; ++k) blocked[static_cast<std::size_t>(cells[pick(rng)])] = 1;

        long bfsTotal = 0, astarTotal = 0, mismatches = 0, pairs = 0;
        double bfsUs = 0, astarUs = 0;
        for (int i = 0; i < 1000; ++i) {
            const CellIndex a = cells[pick(rng)], b = cells[pick(rng)];
            if (blocked[static_cast<std::size_t>(a)] || blocked[static_cast<std::size_t>(b)]) continue;
            auto t0 = Clock::now();
            const PathResult viaBfs = bfsPath(map, a, b, blocked);
            const double tb = msSince(t0) * 1000.0;
            t0 = Clock::now();
            const PathResult viaAStar = aStarPath(map, a, b, blocked);
            const double ta = msSince(t0) * 1000.0;
            if (viaBfs.found != viaAStar.found || viaBfs.path.size() != viaAStar.path.size()) ++mismatches;
            ++pairs;
            bfsTotal += viaBfs.expanded;
            astarTotal += viaAStar.expanded;
            bfsUs += tb;
            astarUs += ta;
            csv << obstacleCount << ',' << i << ',' << viaAStar.path.size() << ',' << viaBfs.expanded << ',' << viaAStar.expanded << ','
                << fmt(tb, 2) << ',' << fmt(ta, 2) << '\n';
        }
        std::cout << "  obstacles " << std::setw(2) << obstacleCount << ":  BFS expands " << fmt(double(bfsTotal) / pairs, 1)
                  << " cells (" << fmt(bfsUs / pairs, 1) << " us),  A* expands " << fmt(double(astarTotal) / pairs, 1) << " cells ("
                  << fmt(astarUs / pairs, 1) << " us),  A* does " << fmt(100.0 * astarTotal / bfsTotal, 1)
                  << "% of BFS's work.  Length mismatches: " << mismatches << '\n';
    }
}

// ===========================================================================
// E4 Route planning
// ===========================================================================

void experimentRouting()
{
    std::cout << "\nE4  ROUTE PLANNING: k shelves per trip (100 random trips per k)\n";
    const GridMap& map = warehouseMap();
    DistanceTable dist(map);
    RoutePlanner planner(map, dist);
    std::ofstream csv = openCsv("e4_routing.csv", "k,trial,as_given,greedy,backtracking,nodes_pruned,nodes_full,greedy_ms,backtracking_ms");
    std::cout << std::right << std::setw(3) << "k" << std::setw(11) << "as-given" << std::setw(9) << "greedy" << std::setw(9) << "exact"
              << std::setw(15) << "greedy worse" << std::setw(12) << "gap %" << std::setw(14) << "nodes pruned" << std::setw(14)
              << "nodes full" << std::setw(12) << "exact ms" << '\n';
    std::mt19937 rng(2026);
    for (int k = 1; k <= 10; ++k) {
        double sumGiven = 0, sumGreedy = 0, sumExact = 0, sumMs = 0;
        long sumPruned = 0, sumFull = 0;
        int greedyWorse = 0, fullTrials = 0;
        for (int trial = 0; trial < 100; ++trial) {
            std::vector<CellIndex> shelves = map.shelves();
            std::shuffle(shelves.begin(), shelves.end(), rng);
            shelves.resize(static_cast<std::size_t>(k));
            const CellIndex start = map.homes()[static_cast<std::size_t>(trial) % map.homes().size()];
            const CellIndex end = map.inductionCellFor(map.sorters()[static_cast<std::size_t>(trial) % map.sorters().size()]);

            const RoutePlan given = planner.asGiven(start, shelves, end);
            auto t0 = Clock::now();
            const RoutePlan greedy = planner.greedy(start, shelves, end);
            const double greedyMs = msSince(t0);
            t0 = Clock::now();
            const RoutePlan exact = planner.backtracking(start, shelves, end, true);
            const double exactMs = msSince(t0);
            long full = -1;
            if (k <= 7 && trial < 20) {   // without pruning the tree has k! * 2^k leaves: only small k
                full = planner.backtracking(start, shelves, end, false).nodes;
                sumFull += full;
                ++fullTrials;
            }
            sumGiven += given.length;
            sumGreedy += greedy.length;
            sumExact += exact.length;
            sumPruned += exact.nodes;
            sumMs += exactMs;
            if (greedy.length > exact.length) ++greedyWorse;
            csv << k << ',' << trial << ',' << given.length << ',' << greedy.length << ',' << exact.length << ',' << exact.nodes << ','
                << full << ',' << fmt(greedyMs, 4) << ',' << fmt(exactMs, 4) << '\n';
        }
        std::cout << std::setw(3) << k << std::setw(11) << fmt(sumGiven / 100, 1) << std::setw(9) << fmt(sumGreedy / 100, 1) << std::setw(9)
                  << fmt(sumExact / 100, 1) << std::setw(12) << greedyWorse << "/100" << std::setw(12)
                  << fmt(100.0 * (sumGreedy - sumExact) / sumExact, 1) << std::setw(14) << sumPruned / 100 << std::setw(14)
                  << (fullTrials ? std::to_string(sumFull / fullTrials) : "-") << std::setw(12) << fmt(sumMs / 100, 3) << '\n';
    }
}

// ===========================================================================
// Whole-warehouse runs (E5, E6)
// ===========================================================================

struct RunStats {
    Tick makespan = 0;
    double leadTime = 0;
    long moves = 0;
    long waits = 0;
    double congestion = 0;
    long collisions = 0;
    bool finished = false;
};

RunStats runWarehouse(SimConfig config, int orders)
{
    Shop shop(warehouseCatalogue(), "");
    Simulation sim(warehouseMap(), shop.catalogue(), config);
    std::mt19937 rng(config.seed);
    for (const Order& o : shop.generateOrders(orders, rng)) sim.submit(o);   // all orders arrive at tick 0
    while (!sim.isQuiescent() && sim.now() < 100000) sim.step();
    const Metrics& m = sim.metrics();
    return {m.lastCompletion, m.avgLeadTime(), m.moves, m.waits, m.congestion(), m.collisions, sim.allWorkDone()};
}

void experimentTraffic()
{
    std::cout << "\nE5  FLEET SIZE: 80 orders, 1..8 picking robots (average of 10 seeds)\n";
    std::ofstream csv = openCsv("e5_traffic.csv", "pickers,seed,makespan,lead_time,moves,waits,congestion,collisions");
    std::cout << std::right << std::setw(8) << "pickers" << std::setw(11) << "makespan" << std::setw(11) << "lead" << std::setw(10) << "moves"
              << std::setw(9) << "waits" << std::setw(13) << "congestion" << std::setw(12) << "speed-up" << std::setw(12) << "collisions" << '\n';
    double oneRobot = 0;
    for (int pickers = 1; pickers <= 8; ++pickers) {
        double makespan = 0, lead = 0, moves = 0, waits = 0, congestion = 0;
        long collisions = 0;
        for (unsigned seed = 1; seed <= 10; ++seed) {
            SimConfig config;
            config.seed = seed;
            config.maxPickers = pickers;
            const RunStats s = runWarehouse(config, 80);
            csv << pickers << ',' << seed << ',' << s.makespan << ',' << fmt(s.leadTime, 1) << ',' << s.moves << ',' << s.waits << ','
                << fmt(s.congestion, 4) << ',' << s.collisions << '\n';
            makespan += s.makespan;
            lead += s.leadTime;
            moves += s.moves;
            waits += s.waits;
            congestion += s.congestion;
            collisions += s.collisions;
        }
        if (pickers == 1) oneRobot = makespan;
        std::cout << std::setw(8) << pickers << std::setw(11) << fmt(makespan / 10, 0) << std::setw(11) << fmt(lead / 10, 0) << std::setw(10)
                  << fmt(moves / 10, 0) << std::setw(9) << fmt(waits / 10, 0) << std::setw(12) << fmt(10.0 * congestion, 2) << "%"
                  << std::setw(11) << fmt(oneRobot / makespan, 2) << "x" << std::setw(12) << collisions << '\n';
    }
}

void experimentPolicies()
{
    std::cout << "\nE6  POLICIES: 80 orders, 8 robots, routing x robot choice (average of 10 seeds)\n";
    std::ofstream csv = openCsv("e6_policies.csv", "routing,assignment,seed,makespan,lead_time,moves,waits");
    std::cout << std::left << std::setw(14) << "routing" << std::setw(17) << "robot choice" << std::right << std::setw(10) << "makespan"
              << std::setw(10) << "lead" << std::setw(10) << "moves" << std::setw(14) << "vs baseline" << '\n';
    double baselineMoves = 0;
    for (AssignmentMethod assignment : {AssignmentMethod::FirstAvailable, AssignmentMethod::Nearest}) {
        for (RoutingMethod routing : {RoutingMethod::AsGiven, RoutingMethod::Greedy, RoutingMethod::Backtracking}) {
            double makespan = 0, lead = 0, moves = 0;
            for (unsigned seed = 1; seed <= 10; ++seed) {
                SimConfig config;
                config.seed = seed;
                config.routing = routing;
                config.assignment = assignment;
                const RunStats s = runWarehouse(config, 80);
                csv << toString(routing) << ',' << toString(assignment) << ',' << seed << ',' << s.makespan << ',' << fmt(s.leadTime, 1)
                    << ',' << s.moves << ',' << s.waits << '\n';
                makespan += s.makespan;
                lead += s.leadTime;
                moves += s.moves;
            }
            if (baselineMoves == 0) baselineMoves = moves;
            std::cout << std::left << std::setw(14) << toString(routing) << std::setw(17) << toString(assignment) << std::right
                      << std::setw(10) << fmt(makespan / 10, 0) << std::setw(10) << fmt(lead / 10, 0) << std::setw(10) << fmt(moves / 10, 0)
                      << std::setw(12) << fmt(100.0 * (moves - baselineMoves) / baselineMoves, 1) << " %" << '\n';
        }
    }
}

// ===========================================================================
// E7 Trip filling: greedy first-fit vs first-fit decreasing vs exact (WP-E, fulfilment)
// ===========================================================================
//
// JobManager::takeTripUnits() fills a trip greedily: walk the pool in urgency
// order and take every item that still fits (<= tripCapacityUnits items and
// <= tripPayloadKg kg). Clearing a pool this way is a two-limit bin-packing
// problem (NP-hard), so first-fit may need more trips than necessary.
//
//   A  first-fit, urgency order        what the program does now   (Lecture 5b: greedy)
//   B  first-fit decreasing            merge-sort heaviest first, then first-fit (Lecture 4)
//   C  exact minimum                   backtracking with pruning   (Lecture 5b)
//
// The experiment does not need the simulation: it works on a pool of items
// whose weights come from the real catalogue.

struct TripItem {
    double kg;
    int rank;   // 0 = most urgent
};

struct FitResult {
    int trips = 0;
    std::vector<int> tripOfRank;   // trip number (1-based) in which each item travels
};

// Same rule as JobManager::takeTripUnits, repeated until the pool is empty.
FitResult firstFitTrips(std::vector<TripItem> pool, int capacity, double payload)
{
    FitResult r;
    r.tripOfRank.assign(pool.size(), 0);
    while (!pool.empty()) {
        ++r.trips;
        std::vector<TripItem> rest;
        int count = 0;
        double weight = 0.0;
        for (const TripItem& u : pool) {
            if (count < capacity && weight + u.kg <= payload + 1e-9) {
                ++count;
                weight += u.kg;
                r.tripOfRank[static_cast<std::size_t>(u.rank)] = r.trips;
            } else {
                rest.push_back(u);
            }
        }
        pool = rest;
    }
    return r;
}

// Can the weights be packed into `trips` trips? Depth-first backtracking.
struct TripPacker {
    const std::vector<double>& w;   // heaviest first: fails early
    int capacity;
    double payload;
    std::vector<double> load;
    std::vector<int> count;
    long nodes = 0;
    long limit = 0;
    bool capped = false;

    TripPacker(const std::vector<double>& weights, int trips, int cap, double pay, long nodeLimit)
        : w(weights), capacity(cap), payload(pay), load(static_cast<std::size_t>(trips), 0.0), count(static_cast<std::size_t>(trips), 0), limit(nodeLimit) {}

    bool place(std::size_t i)
    {
        if (i == w.size()) return true;
        if (++nodes > limit) {
            capped = true;
            return false;
        }
        for (std::size_t b = 0; b < load.size(); ++b) {
            if (count[b] >= capacity || load[b] + w[i] > payload + 1e-9) continue;   // does not fit
            bool twin = false;                                                         // pruning: an earlier trip with the
            for (std::size_t e = 0; e < b; ++e)                                        // same load and count is equivalent
                if (count[e] == count[b] && std::fabs(load[e] - load[b]) < 1e-9) twin = true;
            if (twin) continue;
            load[b] += w[i];
            ++count[b];
            const bool ok = place(i + 1);
            load[b] -= w[i];
            --count[b];
            if (ok) return true;
            if (capped) return false;
        }
        return false;
    }
};

struct ExactResult {
    int trips = 0;
    long nodes = 0;
    bool capped = false;
};

// Smallest number of trips, searching upwards from the lower bound. `upper` (a
// known feasible answer, from A or B) stops the search: nothing below it
// means `upper` is optimal.
ExactResult exactTrips(std::vector<double> weights, int lower, int upper, int capacity, double payload, long nodeLimit)
{
    std::sort(weights.begin(), weights.end(), [](double a, double b) { return a > b; });
    ExactResult r;
    r.trips = upper;
    for (int t = lower; t < upper; ++t) {
        TripPacker packer(weights, t, capacity, payload, nodeLimit);
        const bool ok = packer.place(0);
        r.nodes += packer.nodes;
        if (packer.capped) {
            r.capped = true;   // gave up: `upper` is only an upper bound
            return r;
        }
        if (ok) {
            r.trips = t;
            return r;
        }
    }
    return r;
}

void experimentFulfilment()
{
    const SimConfig defaults;
    std::cout << "\nE7  TRIP FILLING: first-fit (urgency) vs first-fit decreasing vs exact (1000 random pools per row)\n"
              << "    sweep: trip payload limit (kg) x pool size n; the item-count limit is " << defaults.tripCapacityUnits << "\n";
    const int capacity = defaults.tripCapacityUnits;
    const std::vector<Product>& products = warehouseCatalogue().all();
    constexpr int kPools = 1000;   // payload limits below the heaviest product (2.5 kg) are not swept
    constexpr long kNodeLimit = 2000000;

    std::ofstream csv = openCsv("e7_fulfilment.csv", "payload_kg,n,trial,total_kg,lower_bound,binding,trips_ff,trips_ffd,trips_opt,capped,nodes_exact,urgent_trip_ff,urgent_trip_ffd,ff_us,ffd_us,exact_us");
    std::cout << std::right << std::setw(8) << "payload" << std::setw(4) << "n" << std::setw(9) << "bound" << std::setw(8) << "FF" << std::setw(8) << "FFD" << std::setw(8) << "exact"
              << std::setw(11) << "FF opt %" << std::setw(11) << "FFD opt %" << std::setw(11) << "FF gap %" << std::setw(12) << "urgent FF" << std::setw(12) << "urgent FFD"
              << std::setw(13) << "nodes" << std::setw(8) << "capped" << '\n';

    std::mt19937 rng(2026);
    std::uniform_int_distribution<int> pick(0, static_cast<int>(products.size()) - 1);
    for (double payload : {12.0, 8.0, 6.0, 5.0, 4.0, 3.0})
    for (int n : {8, 12, 16, 20}) {
        double sumBound = 0, sumFf = 0, sumFfd = 0, sumOpt = 0, sumUrgFf = 0, sumUrgFfd = 0;
        long sumNodes = 0;
        int ffOptimal = 0, ffdOptimal = 0, cappedCount = 0;
        for (int trial = 0; trial < kPools; ++trial) {
            std::vector<TripItem> pool;   // already in urgency order: rank = position
            double total = 0.0;
            while (static_cast<int>(pool.size()) < n) {
                const double kg = products[static_cast<std::size_t>(pick(rng))].weightKg;
                if (kg > payload) continue;   // a single item must fit on a trip
                pool.push_back({kg, static_cast<int>(pool.size())});
                total += kg;
            }
            const int countBound = (n + capacity - 1) / capacity;
            const int weightBound = static_cast<int>(std::ceil(total / payload - 1e-9));
            const int lower = std::max(countBound, weightBound);
            const char* binding = weightBound > countBound ? "weight" : "count";

            auto t0 = Clock::now();
            const FitResult ff = firstFitTrips(pool, capacity, payload);
            const double ffUs = msSince(t0) * 1000.0;

            std::vector<TripItem> byWeight = pool;
            t0 = Clock::now();
            mergeSort(byWeight, [](const TripItem& a, const TripItem& b) { return a.kg > b.kg; });   // stable: ties keep urgency order
            const FitResult ffd = firstFitTrips(byWeight, capacity, payload);
            const double ffdUs = msSince(t0) * 1000.0;

            std::vector<double> weights;
            for (const TripItem& u : pool) weights.push_back(u.kg);
            t0 = Clock::now();
            const ExactResult opt = exactTrips(weights, lower, std::min(ff.trips, ffd.trips), capacity, payload, kNodeLimit);
            const double exactUs = msSince(t0) * 1000.0;

            // Delay of the most urgent quarter of the pool: mean trip number.
            const int urgent = std::max(1, n / 4);
            double urgFf = 0, urgFfd = 0;
            for (int r = 0; r < urgent; ++r) {
                urgFf += ff.tripOfRank[static_cast<std::size_t>(r)];
                urgFfd += ffd.tripOfRank[static_cast<std::size_t>(r)];
            }
            urgFf /= urgent;
            urgFfd /= urgent;

            sumBound += lower;
            sumFf += ff.trips;
            sumFfd += ffd.trips;
            sumOpt += opt.trips;
            sumNodes += opt.nodes;
            sumUrgFf += urgFf;
            sumUrgFfd += urgFfd;
            if (ff.trips == opt.trips) ++ffOptimal;
            if (ffd.trips == opt.trips) ++ffdOptimal;
            if (opt.capped) ++cappedCount;
            csv << fmt(payload, 1) << ',' << n << ',' << trial << ',' << fmt(total, 2) << ',' << lower << ',' << binding << ',' << ff.trips << ',' << ffd.trips << ',' << opt.trips << ','
                << (opt.capped ? 1 : 0) << ',' << opt.nodes << ',' << fmt(urgFf, 2) << ',' << fmt(urgFfd, 2) << ',' << fmt(ffUs, 2) << ',' << fmt(ffdUs, 2) << ','
                << fmt(exactUs, 2) << '\n';
        }
        std::cout << std::setw(8) << fmt(payload, 1) << std::setw(4) << n << std::setw(9) << fmt(sumBound / kPools, 3) << std::setw(8) << fmt(sumFf / kPools, 3) << std::setw(8) << fmt(sumFfd / kPools, 3)
                  << std::setw(8) << fmt(sumOpt / kPools, 3) << std::setw(11) << fmt(100.0 * ffOptimal / kPools, 1) << std::setw(11) << fmt(100.0 * ffdOptimal / kPools, 1)
                  << std::setw(11) << fmt(100.0 * (sumFf - sumOpt) / sumOpt, 2) << std::setw(12) << fmt(sumUrgFf / kPools, 2) << std::setw(12) << fmt(sumUrgFfd / kPools, 2)
                  << std::setw(13) << sumNodes / kPools << std::setw(8) << cappedCount << '\n';
    }
}

// ===========================================================================
// E8 Lookup structures: tree shape vs insertion order (Lectures 7 and 8)   (WP-B extension)
// ===========================================================================
//
// Question: the catalogue is looked up by key (product name or SKU). Lecture 8 says the shape of a
// plain binary search tree "depends on order of insertion", so search cost per lookup is 1 + depth.
// Does insertion order matter, and does a self-balancing tree (left-leaning red-black tree, LLRBT)
// or a hash table with separate chaining (Lecture 7) remove the problem?
//
// Cost measure: key comparisons per successful lookup (one three-way comparison per node or chain
// entry visited, which is the "1 + depth" of the lecture). Counts do not depend on the computer.
// All structures keep their nodes in a std::vector and link them by index, so even a degenerate
// tree of 10,000 nodes needs no deep recursion and no recursive destructor.

/// Plain (unbalanced) binary search tree, Lecture 8 slides 6-13.
template <typename Key>
class PlainBst {
public:
    /// Inserts `key` (ignored if present). Returns the number of nodes compared on the way down.
    long insert(const Key& key)
    {
        if (nodes_.empty()) {
            nodes_.push_back({key, -1, -1});
            return 0;
        }
        long cmp = 0;
        int cur = 0, depth = 0;
        for (;;) {
            ++cmp;
            ++depth;
            Node& n = nodes_[static_cast<std::size_t>(cur)];
            int* link = nullptr;
            if (key < n.key) link = &n.left;
            else if (n.key < key) link = &n.right;
            else return cmp;                      // already present
            if (*link == -1) {
                const int fresh = static_cast<int>(nodes_.size());
                *link = fresh;                    // set the link first: push_back may move the vector
                nodes_.push_back({key, -1, -1});
                height_ = std::max(height_, depth);
                return cmp;
            }
            cur = *link;
        }
    }
    /// Comparisons for a lookup of `key` (= 1 + depth of the node when it is present).
    long search(const Key& key) const
    {
        long cmp = 0;
        int cur = nodes_.empty() ? -1 : 0;
        while (cur != -1) {
            ++cmp;
            const Node& n = nodes_[static_cast<std::size_t>(cur)];
            if (key < n.key) cur = n.left;
            else if (n.key < key) cur = n.right;
            else return cmp;
        }
        return -cmp;                              // not found (never happens in this experiment)
    }
    int height() const { return nodes_.empty() ? -1 : height_; }   // edges on the longest path; leaf = 0
    std::size_t size() const { return nodes_.size(); }

private:
    struct Node { Key key; int left, right; };
    std::vector<Node> nodes_;
    int height_ = 0;
};

/// Left-leaning red-black tree, Lecture 8 slides 32-47: rotate left, rotate right, flip colours.
template <typename Key>
class LeftLeaningRbt {
public:
    long insert(const Key& key)
    {
        long cmp = 0;
        root_ = put(root_, key, cmp);
        nodes_[static_cast<std::size_t>(root_)].red = false;   // the root is always black
        return cmp;
    }
    long search(const Key& key) const   // identical to the plain BST: colour is ignored (slide 35)
    {
        long cmp = 0;
        int cur = root_;
        while (cur != -1) {
            ++cmp;
            const Node& n = nodes_[static_cast<std::size_t>(cur)];
            if (key < n.key) cur = n.left;
            else if (n.key < key) cur = n.right;
            else return cmp;
        }
        return -cmp;
    }
    int height() const { return height(root_); }
    std::size_t size() const { return nodes_.size(); }
    /// Slide 33: no right-leaning red link, no two reds in a row, same number of black links on every path.
    bool valid() const { return !isRed(root_) && blackHeight(root_) >= 0; }

private:
    struct Node { Key key; int left, right; bool red; };
    std::vector<Node> nodes_;
    int root_ = -1;

    Node& at(int i) { return nodes_[static_cast<std::size_t>(i)]; }
    const Node& at(int i) const { return nodes_[static_cast<std::size_t>(i)]; }
    bool isRed(int i) const { return i != -1 && at(i).red; }

    int rotateLeft(int h)    // orient a right-leaning red link to lean left (slide 38)
    {
        const int x = at(h).right;
        at(h).right = at(x).left;
        at(x).left = h;
        at(x).red = at(h).red;
        at(h).red = true;
        return x;
    }
    int rotateRight(int h)   // slide 39
    {
        const int x = at(h).left;
        at(h).left = at(x).right;
        at(x).right = h;
        at(x).red = at(h).red;
        at(h).red = true;
        return x;
    }
    void flipColors(int h)   // split a temporary 4-node (slide 40)
    {
        at(h).red = !at(h).red;
        at(at(h).left).red = !at(at(h).left).red;
        at(at(h).right).red = !at(at(h).right).red;
    }
    int put(int h, const Key& key, long& cmp)
    {
        if (h == -1) {
            nodes_.push_back({key, -1, -1, true});   // new links are red
            return static_cast<int>(nodes_.size()) - 1;
        }
        ++cmp;
        if (key < at(h).key) {
            const int child = put(at(h).left, key, cmp);
            at(h).left = child;
        } else if (at(h).key < key) {
            const int child = put(at(h).right, key, cmp);
            at(h).right = child;
        }
        // The same three lines fix every case (slide 45).
        if (isRed(at(h).right) && !isRed(at(h).left)) h = rotateLeft(h);
        if (isRed(at(h).left) && isRed(at(at(h).left).left)) h = rotateRight(h);
        if (isRed(at(h).left) && isRed(at(h).right)) flipColors(h);
        return h;
    }
    int height(int h) const { return h == -1 ? -1 : 1 + std::max(height(at(h).left), height(at(h).right)); }
    int blackHeight(int h) const   // -1 when some invariant is broken
    {
        if (h == -1) return 0;
        if (isRed(at(h).right)) return -1;
        if (isRed(h) && isRed(at(h).left)) return -1;
        const int l = blackHeight(at(h).left), r = blackHeight(at(h).right);
        if (l < 0 || r < 0 || l != r) return -1;
        return l + (isRed(h) ? 0 : 1);
    }
};

/// Hash table with separate chaining, division method h(k) = k mod m with m prime (Lecture 7 slides 13-14).
template <typename Key, typename Hash>
class ChainedHashTable {
public:
    explicit ChainedHashTable(std::size_t buckets) : chains_(buckets) {}
    /// Inserts at the front of the chain (slide 14). Returns the chain entries compared to rule out a duplicate.
    long insert(const Key& key)
    {
        std::vector<Key>& chain = chains_[Hash()(key) % chains_.size()];
        long cmp = 0;
        for (const Key& k : chain) {
            ++cmp;
            if (k == key) return cmp;
        }
        chain.insert(chain.begin(), key);
        return cmp;
    }
    long search(const Key& key) const
    {
        const std::vector<Key>& chain = chains_[Hash()(key) % chains_.size()];
        long cmp = 0;
        for (const Key& k : chain) {
            ++cmp;
            if (k == key) return cmp;
        }
        return -cmp;
    }

private:
    std::vector<std::vector<Key>> chains_;
};

struct IntHash { std::size_t operator()(int k) const { return static_cast<std::size_t>(k); } };
struct StringHash {   // polynomial hash, written out so that every compiler gives the same buckets
    std::size_t operator()(const std::string& s) const
    {
        std::size_t h = 0;
        for (unsigned char c : s) h = h * 31 + c;
        return h;
    }
};

std::size_t primeAtLeast(std::size_t x)
{
    auto isPrime = [](std::size_t v) {
        if (v < 2) return false;
        for (std::size_t d = 2; d * d <= v; ++d)
            if (v % d == 0) return false;
        return true;
    };
    while (!isPrime(x)) ++x;
    return x;
}

/// Fisher-Yates shuffle driven by raw mt19937 output (std::shuffle is not portable across standard libraries).
template <typename T>
void portableShuffle(std::vector<T>& v, unsigned seed)
{
    std::mt19937 rng(seed);
    for (std::size_t i = v.size(); i > 1; --i) std::swap(v[i - 1], v[rng() % i]);
}

/// Insertion order that builds a perfectly balanced BST: the median first, then the medians of each half (level by level).
template <typename T>
std::vector<T> balancedOrder(const std::vector<T>& sorted)
{
    std::vector<T> order;
    std::vector<std::pair<int, int>> queue{{0, static_cast<int>(sorted.size()) - 1}};
    for (std::size_t head = 0; head < queue.size(); ++head) {
        const auto [lo, hi] = queue[head];
        if (lo > hi) continue;
        const int mid = lo + (hi - lo) / 2;
        order.push_back(sorted[static_cast<std::size_t>(mid)]);
        queue.push_back({lo, mid - 1});
        queue.push_back({mid + 1, hi});
    }
    return order;
}

/// Builds the four structures from `insertionOrder`, looks every key up once, writes four CSV rows.
template <typename Key, typename Hash>
void measureLookups(std::ofstream& csv, const std::string& dataset, const std::string& order, unsigned seed,
                    const std::vector<Key>& insertionOrder, const std::vector<Key>& sortedKeys, bool print)
{
    const int n = static_cast<int>(insertionOrder.size());
    struct Row { std::string structure; std::string height; double avg; long max; long build; };
    std::vector<Row> rows;
    auto lookupAll = [&](auto&& search, long& maxCmp) {
        long total = 0;
        maxCmp = 0;
        for (const Key& k : sortedKeys) {
            const long c = search(k);
            if (c <= 0) std::cout << "  !! key not found in " << dataset << '/' << order << '\n';
            total += c;
            maxCmp = std::max(maxCmp, c);
        }
        return double(total) / n;
    };

    {   // plain BST
        PlainBst<Key> t;
        long build = 0, maxCmp = 0;
        for (const Key& k : insertionOrder) build += t.insert(k);
        const double avg = lookupAll([&](const Key& k) { return t.search(k); }, maxCmp);
        if (maxCmp != t.height() + 1) std::cout << "  !! BST: max comparisons is not height + 1\n";
        rows.push_back({"BST", std::to_string(t.height()), avg, maxCmp, build});
    }
    {   // left-leaning red-black tree
        LeftLeaningRbt<Key> t;
        long build = 0, maxCmp = 0;
        for (const Key& k : insertionOrder) build += t.insert(k);
        const double avg = lookupAll([&](const Key& k) { return t.search(k); }, maxCmp);
        if (!t.valid() || t.size() != insertionOrder.size()) std::cout << "  !! LLRBT invariant broken (" << dataset << '/' << order << ")\n";
        if (t.height() > 2 * static_cast<int>(std::floor(std::log2(n))) + 1) std::cout << "  !! LLRBT taller than 2 lg n\n";
        rows.push_back({"LLRBT", std::to_string(t.height()), avg, maxCmp, build});
    }
    {   // hash table, load factor about 4 (Lecture 7 slide 28 keeps k/m between 2 and 8)
        ChainedHashTable<Key, Hash> t(primeAtLeast(static_cast<std::size_t>(std::max(1, n / 4))));
        long build = 0, maxCmp = 0;
        for (const Key& k : insertionOrder) build += t.insert(k);
        const double avg = lookupAll([&](const Key& k) { return t.search(k); }, maxCmp);
        rows.push_back({"hash-chaining", "", avg, maxCmp, build});
    }
    {   // sorted array + binary search; "build" is the merge-sort cost of sorting the incoming keys
        std::vector<Key> a = insertionOrder;
        long build = 0, maxCmp = 0;
        mergeSort(a, [&build](const Key& x, const Key& y) { ++build; return x < y; });
        const double avg = lookupAll(
            [&](const Key& k) {
                long c = 0;
                const int idx = binarySearch(a, k, [&c](const Key& element, const Key& key) { ++c; return element < key ? -1 : (key < element ? 1 : 0); });
                return idx >= 0 ? c : -c;
            },
            maxCmp);
        rows.push_back({"binary-search-array", "", avg, maxCmp, build});
    }

    for (const Row& r : rows) {
        csv << dataset << ',' << r.structure << ',' << order << ',' << n << ',' << seed << ',' << r.height << ',' << fmt(r.avg, 3) << ',' << r.max << ',' << r.build << '\n';
        if (print)
            std::cout << std::left << std::setw(11) << dataset << std::setw(16) << order << std::setw(21) << r.structure << std::right << std::setw(8) << n
                      << std::setw(9) << (r.height.empty() ? "-" : r.height) << std::setw(11) << fmt(r.avg, 2) << std::setw(9) << r.max << std::setw(14) << r.build << '\n';
    }
}

void experimentTrees()
{
    std::cout << "\nE8  LOOKUP STRUCTURES: BST vs LLRBT vs hash table vs sorted array (comparisons per successful lookup)\n";
    std::ofstream csv = openCsv("e8_trees.csv", "dataset,structure,order,n,seed,height,avg_cmp,max_cmp,build_cmp");
    std::cout << std::left << std::setw(11) << "dataset" << std::setw(16) << "insertion order" << std::setw(21) << "structure" << std::right << std::setw(8) << "n"
              << std::setw(9) << "height" << std::setw(11) << "avg cmp" << std::setw(9) << "max cmp" << std::setw(14) << "build cmp" << '\n';

    // Part A: synthetic SKU numbers 10000, 10007, 10014, ... in five insertion orders.
    const std::vector<int> sizes{100, 200, 500, 1000, 2000, 5000, 10000};
    for (int n : sizes) {
        std::vector<int> sorted(static_cast<std::size_t>(n));
        for (int i = 0; i < n; ++i) sorted[static_cast<std::size_t>(i)] = 10000 + 7 * i;
        const bool print = (n == 10000);

        for (unsigned seed = 1; seed <= 10; ++seed) {   // random order: ten different seeds
            std::vector<int> v = sorted;
            portableShuffle(v, seed);
            measureLookups<int, IntHash>(csv, "sku", "random", seed, v, sorted, print && seed == 1);
        }
        measureLookups<int, IntHash>(csv, "sku", "ascending", 0, sorted, sorted, print);
        std::vector<int> desc(sorted.rbegin(), sorted.rend());
        measureLookups<int, IntHash>(csv, "sku", "descending", 0, desc, sorted, print);
        std::vector<int> nearly = sorted;   // sorted batches with 1% of the entries swapped (late corrections)
        std::mt19937 rng(7);
        for (int s = 0; s < std::max(1, n / 100); ++s) std::swap(nearly[rng() % nearly.size()], nearly[rng() % nearly.size()]);
        measureLookups<int, IntHash>(csv, "sku", "nearly-sorted", 7, nearly, sorted, print);
        measureLookups<int, IntHash>(csv, "sku", "median-first", 0, balancedOrder(sorted), sorted, print);
    }

    // Part B: the real 91 product names from data/Products.txt.
    std::vector<std::string> names;
    for (const Product& p : warehouseCatalogue().all()) names.push_back(p.name);
    std::vector<std::string> alpha = names;
    std::sort(alpha.begin(), alpha.end());
    if (std::adjacent_find(alpha.begin(), alpha.end()) != alpha.end()) std::cout << "  !! duplicate product names\n";
    measureLookups<std::string, StringHash>(csv, "catalogue", "file-order", 0, names, alpha, true);
    measureLookups<std::string, StringHash>(csv, "catalogue", "alphabetical", 0, alpha, alpha, true);
    std::vector<std::string> revAlpha(alpha.rbegin(), alpha.rend());
    measureLookups<std::string, StringHash>(csv, "catalogue", "reverse-alpha", 0, revAlpha, alpha, true);
    for (unsigned seed = 1; seed <= 10; ++seed) {
        std::vector<std::string> v = names;
        portableShuffle(v, seed);
        measureLookups<std::string, StringHash>(csv, "catalogue", "random", seed, v, alpha, seed == 1);
    }
}


// ===========================================================================
// E9 Substring search for the shop's search box: linear scan vs suffix array   (WP-B)
// ===========================================================================
//
// Question: the search box finds products whose name CONTAINS the typed text ("ric" -> "Bag of
// rice"). Binary search cannot do this directly, because the names are sorted by their FIRST
// letters only. Catalogue::search() therefore scans every name: O(n * m). Is that good enough, or
// is a better structure worth its cost?
//
// The structure tested is a suffix array, which COMBINES two course topics (sorting + binary search):
//   1. Build once: list every suffix of every lower-case name ("rice", "ice", "ce", "e" ...) and
//      sort that list with mergeSort [L4]. Cost: O(S log S) comparisons, S = number of suffixes.
//   2. Search: every name that contains "ric" has a suffix that STARTS with "ric", and in the sorted
//      list those suffixes sit next to each other. A binary search for the first suffix >= "ric"
//      (the "rank" of the key, Lecture 7 slide 5) finds the block in O(log S) comparisons; the block
//      is then read off. The problem is transformed so that binary search can be used.
//
// Three versions of the search box are compared, all returning exactly the same products:
//   A  linear scan, lower-casing every name on every search    (what Catalogue::search does today)
//   B  linear scan over names lower-cased once in advance      (the cheap fix the shop README suggests)
//   C  suffix array, built once                                (more memory, much less work per search)
// Measured: names examined / suffix comparisons per search (machine independent), microseconds per
// search (median of 3 runs, machine dependent), build cost and extra memory of C, and the number of
// searches after which C's build cost has been repaid ("break-even").
// Only product NAMES are searched here; the real search() also checks the category.

struct Suffix {
    int name;   ///< index of the name in the list
    int start;  ///< position in that name where the suffix begins
};

/// Product-like names for large n: "<adjective> <food> <code>", e.g. "fresh mango bkq". The 3-letter
/// code (the index written in base 26) keeps every name unique. rng() % k gives the same values with
/// every compiler, unlike std::uniform_int_distribution.
std::vector<std::string> syntheticNames(int n, unsigned seed)
{
    static const char* adjectives[] = {"fresh", "organic", "frozen", "smoked", "sliced", "whole", "low fat", "sweet",
                                       "spicy", "dried", "salted", "baby", "large", "mini", "wild", "premium"};
    static const char* foods[] = {"rice", "milk", "bread", "apple", "mango", "cheese", "yoghurt", "butter", "chicken",
                                  "salmon", "tuna", "pasta", "noodles", "beans", "carrots", "onions", "garlic", "tea",
                                  "coffee", "honey", "jam", "cereal", "oats", "eggs", "tofu", "lettuce", "spinach",
                                  "grapes", "banana", "orange juice", "soy sauce", "peanut butter"};
    std::mt19937 rng(seed);
    std::vector<std::string> names;
    names.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        std::string code;
        for (int v = i, k = 0; k < 4; ++k, v /= 26) code.insert(code.begin(), static_cast<char>('a' + v % 26));
        names.push_back(std::string(adjectives[rng() % 16]) + " " + foods[rng() % 32] + " " + code);
    }
    return names;
}

/// Version A: what Catalogue::search does (toLower on every name, every time).
std::vector<int> scanLowerEveryTime(const std::vector<std::string>& names, const std::string& text)
{
    const std::string needle = toLower(text);
    std::vector<int> found;
    for (int i = 0; i < static_cast<int>(names.size()); ++i)
        if (toLower(names[static_cast<std::size_t>(i)]).find(needle) != std::string::npos) found.push_back(i);
    return found;
}

/// Version B: the same scan over names that were lower-cased once in advance.
std::vector<int> scanPreLowered(const std::vector<std::string>& lowerNames, const std::string& text)
{
    const std::string needle = toLower(text);
    std::vector<int> found;
    for (int i = 0; i < static_cast<int>(lowerNames.size()); ++i)
        if (lowerNames[static_cast<std::size_t>(i)].find(needle) != std::string::npos) found.push_back(i);
    return found;
}

/// Version C: a sorted list of all suffixes of all lower-case names.
class SuffixArray {
public:
    explicit SuffixArray(const std::vector<std::string>& names) : names_(names)
    {
        for (int i = 0; i < static_cast<int>(names_.size()); ++i)
            for (int s = 0; s < static_cast<int>(names_[static_cast<std::size_t>(i)].size()); ++s) suffixes_.push_back({i, s});
        long& cmp = buildComparisons_;
        mergeSort(suffixes_, [this, &cmp](const Suffix& a, const Suffix& b) {   // O(S log S) [L4]
            ++cmp;
            return compareSuffixes(a, b) < 0;
        });
    }

    /// Products whose name contains `needle` (lower case), in id order.
    /// searchCmp: suffix comparisons made by the binary search (about log2 S).
    /// readCmp:   suffixes read from the matching block (one per match, plus the one that stops it).
    /// Total work O(log S + k) for k matches, instead of O(n * m) for a scan.
    std::vector<int> find(const std::string& needle, long& searchCmp, long& readCmp) const
    {
        // Binary search for the first suffix that is >= needle (its "rank"), Lecture 7 slide 5.
        int lo = 0, hi = static_cast<int>(suffixes_.size());
        while (lo < hi) {
            const int mid = lo + (hi - lo) / 2;
            ++searchCmp;
            if (compareWithText(suffixes_[static_cast<std::size_t>(mid)], needle) < 0) lo = mid + 1;
            else hi = mid;
        }
        // All suffixes that START with needle follow one another from position lo.
        std::vector<int> found;
        for (int i = lo; i < static_cast<int>(suffixes_.size()); ++i) {
            ++readCmp;
            const Suffix& sfx = suffixes_[static_cast<std::size_t>(i)];
            if (nameOf(sfx).compare(static_cast<std::size_t>(sfx.start), needle.size(), needle) != 0) break;   // no longer starts with needle
            found.push_back(sfx.name);
        }
        // A name can contain the text twice ("banana" contains "an" twice): sort and drop repeats.
        std::sort(found.begin(), found.end());
        found.erase(std::unique(found.begin(), found.end()), found.end());
        return found;
    }

    long size() const { return static_cast<long>(suffixes_.size()); }
    long buildComparisons() const { return buildComparisons_; }
    long extraBytes() const { return size() * static_cast<long>(sizeof(Suffix)); }

private:
    const std::string& nameOf(const Suffix& a) const { return names_[static_cast<std::size_t>(a.name)]; }

    // Compares two suffixes in alphabetical order without copying them: std::string::compare can
    // compare "the part of this string from position p onwards" directly. <0, 0 or >0 like strcmp.
    int compareSuffixes(const Suffix& a, const Suffix& b) const
    {
        return nameOf(a).compare(static_cast<std::size_t>(a.start), std::string::npos, nameOf(b), static_cast<std::size_t>(b.start),
                                 std::string::npos);
    }
    int compareWithText(const Suffix& a, const std::string& text) const
    {
        return nameOf(a).compare(static_cast<std::size_t>(a.start), std::string::npos, text);
    }

    std::vector<std::string> names_;
    std::vector<Suffix> suffixes_;
    long buildComparisons_ = 0;
};

double medianOf3(double a, double b, double c) { return std::max(std::min(a, b), std::min(std::max(a, b), c)); }

void experimentSubstring()
{
    std::cout << "\nE9  SEARCH BOX: substring search, linear scan vs suffix array\n";
    std::ofstream csv = openCsv("e9_substring.csv",
                                "dataset,n,queries,avg_matches,suffixes,scan_names_per_query,sa_search_avg_cmp,sa_search_max_cmp,"
                                "log2_suffixes,sa_read_avg,sa_build_cmp,sa_build_ms,sa_extra_kb,us_scan_lower_each_time,"
                                "us_scan_prelowered,us_suffix_array,breakeven_vs_A,breakeven_vs_B,all_results_equal");
    std::cout << std::right << std::setw(8) << "n" << std::setw(9) << "matches" << std::setw(10) << "suffixes" << std::setw(9) << "scan/q"
              << std::setw(10) << "SA bs/q" << std::setw(8) << "log2 S" << std::setw(9) << "SA rd/q" << std::setw(11) << "build cmp"
              << std::setw(9) << "extra KB" << std::setw(9) << "us A" << std::setw(9) << "us B" << std::setw(9) << "us C" << std::setw(8)
              << "BE(A)" << std::setw(8) << "BE(B)" << std::setw(6) << "same" << '\n';

    struct Dataset {
        std::string label;
        std::vector<std::string> names;
    };
    std::vector<Dataset> datasets;
    {
        std::vector<std::string> real;
        for (const Product& p : warehouseCatalogue().all()) real.push_back(p.name);
        datasets.push_back({"catalogue", real});
    }
    for (int n : {1000, 10000, 100000}) datasets.push_back({"synthetic", syntheticNames(n, 9)});

    const int queriesPerSize = 300;
    for (const Dataset& d : datasets) {
        const std::vector<std::string>& names = d.names;
        const int n = static_cast<int>(names.size());
        std::vector<std::string> lower;
        for (const std::string& s : names) lower.push_back(toLower(s));

        // Queries: a random 3-letter piece of a random name, as a customer would type it ("ric").
        std::mt19937 rng(11);
        std::vector<std::string> queries;
        while (static_cast<int>(queries.size()) < queriesPerSize) {
            const std::string& s = lower[rng() % lower.size()];
            if (s.size() < 3) continue;
            queries.push_back(s.substr(rng() % (s.size() - 2), 3));
        }

        // Build the suffix array (timed once; counted exactly).
        const auto t0 = Clock::now();
        const SuffixArray sa(lower);
        const double buildMs = msSince(t0);

        // Correctness first: all three versions must return exactly the same products.
        bool same = true;
        long searchTotal = 0, searchMax = 0, readTotal = 0, matches = 0;
        for (const std::string& q : queries) {
            long searchCmp = 0, readCmp = 0;
            const std::vector<int> a = scanLowerEveryTime(names, q);
            const std::vector<int> b = scanPreLowered(lower, q);
            const std::vector<int> r = sa.find(q, searchCmp, readCmp);
            if (a != b || a != r) same = false;
            searchTotal += searchCmp;
            searchMax = std::max(searchMax, searchCmp);
            readTotal += readCmp;
            matches += static_cast<long>(a.size());
        }
        if (!same) std::cout << "  !! the three searches returned different products\n";

        // Time per search: median of three runs over all queries (Lecture 3: times vary between runs).
        auto timePerQuery = [&](const std::function<std::size_t(const std::string&)>& search) {
            double runs[3];
            for (double& run : runs) {
                std::size_t sink = 0;
                const auto t = Clock::now();
                for (const std::string& q : queries) sink += search(q);
                run = msSince(t) * 1000.0 / queriesPerSize;
                if (sink == static_cast<std::size_t>(-1)) std::cout << "";   // keeps the work from being optimised away
            }
            return medianOf3(runs[0], runs[1], runs[2]);
        };
        const double usA = timePerQuery([&](const std::string& q) { return scanLowerEveryTime(names, q).size(); });
        const double usB = timePerQuery([&](const std::string& q) { return scanPreLowered(lower, q).size(); });
        const double usC = timePerQuery([&](const std::string& q) { long sc = 0, rc = 0; return sa.find(q, sc, rc).size(); });

        // Break-even: searches needed before the time saved per search repays the one-off build time.
        auto breakEven = [&](double usScan) { return usScan > usC ? buildMs * 1000.0 / (usScan - usC) : -1.0; };
        const double beA = breakEven(usA), beB = breakEven(usB);

        const double q = queriesPerSize;
        const double searchAvg = searchTotal / q, readAvg = readTotal / q, matchAvg = matches / q;
        const double log2S = std::log2(double(sa.size())), extraKb = sa.extraBytes() / 1024.0;
        csv << d.label << ',' << n << ',' << queriesPerSize << ',' << fmt(matchAvg, 2) << ',' << sa.size() << ',' << n << ','
            << fmt(searchAvg, 2) << ',' << searchMax << ',' << fmt(log2S, 2) << ',' << fmt(readAvg, 2) << ',' << sa.buildComparisons() << ','
            << fmt(buildMs, 2) << ',' << fmt(extraKb, 1) << ',' << fmt(usA, 2) << ',' << fmt(usB, 2) << ',' << fmt(usC, 2) << ','
            << fmt(beA, 0) << ',' << fmt(beB, 0) << ',' << (same ? "yes" : "no") << '\n';
        std::cout << std::setw(8) << n << std::setw(9) << fmt(matchAvg, 1) << std::setw(10) << sa.size() << std::setw(9) << n << std::setw(10)
                  << fmt(searchAvg, 1) << std::setw(8) << fmt(log2S, 1) << std::setw(9) << fmt(readAvg, 1) << std::setw(11)
                  << sa.buildComparisons() << std::setw(9) << fmt(extraKb, 0) << std::setw(9) << fmt(usA, 1) << std::setw(9) << fmt(usB, 1)
                  << std::setw(9) << fmt(usC, 1) << std::setw(8) << fmt(beA, 0) << std::setw(8) << fmt(beB, 0) << std::setw(6)
                  << (same ? "yes" : "NO") << '\n';
    }
}

} // namespace

// ===========================================================================
// E10 Route improvement: greedy vs greedy + local search (2-opt) vs exact (WP-C, navigation)
//
// Question: how much of greedy's gap to the optimal route can a cheap local search close,
// and at what cost compared with the exact backtracking search?
// This experiment is standalone: the local search below lives HERE, not in the navigation
// module. It only uses the module's GridMap, DistanceTable and RoutePlanner as they are.
// Same random trips as E4 (same seed and sampling), so the two tables can be compared.
// ===========================================================================

namespace {

/// One stop of a route: which shelf (index into the trip's shelf list) and the cell the robot stands on.
struct Stop {
    int shelf;
    CellIndex cell;
};

/// Moves needed for start -> every stop in order -> end.
int routeLength(DistanceTable& dist, CellIndex start, const std::vector<Stop>& route, CellIndex end)
{
    int length = 0;
    CellIndex position = start;
    for (const Stop& s : route) {
        length += dist.distance(position, s.cell);
        position = s.cell;
    }
    return length + dist.distance(position, end);
}

/// Local search on a finished route. Two kinds of move, repeated until none of them shortens the route:
///   2-opt  : reverse the part of the route between stop i and stop j;
///   side   : visit one shelf from a different side.
/// `evaluations` counts how many candidate routes were measured (the work done).
std::vector<Stop> improveRoute(const GridMap& map, DistanceTable& dist, CellIndex start, std::vector<Stop> route, CellIndex end,
                               const std::vector<CellIndex>& shelves, long& evaluations)
{
    int best = routeLength(dist, start, route, end);
    bool improved = true;
    while (improved) {
        improved = false;
        for (std::size_t i = 0; i < route.size(); ++i) {
            for (std::size_t j = i + 1; j < route.size(); ++j) {   // 2-opt: reverse route[i..j]
                std::reverse(route.begin() + static_cast<std::ptrdiff_t>(i), route.begin() + static_cast<std::ptrdiff_t>(j) + 1);
                ++evaluations;
                const int length = routeLength(dist, start, route, end);
                if (length < best) {
                    best = length;
                    improved = true;
                } else {
                    std::reverse(route.begin() + static_cast<std::ptrdiff_t>(i), route.begin() + static_cast<std::ptrdiff_t>(j) + 1);   // undo
                }
            }
        }
        for (Stop& stop : route) {                                    // side change
            const CellIndex original = stop.cell;
            for (CellIndex other : map.accessCells(shelves[static_cast<std::size_t>(stop.shelf)])) {
                if (other == original) continue;
                stop.cell = other;
                ++evaluations;
                const int length = routeLength(dist, start, route, end);
                if (length < best) {
                    best = length;
                    improved = true;
                } else {
                    stop.cell = original;                              // undo
                    continue;
                }
                break;                                                 // keep the improvement, go to the next stop
            }
        }
    }
    return route;
}

} // namespace

void experimentRouteImprovement()
{
    std::cout << "\nE10  ROUTE IMPROVEMENT: greedy vs greedy + 2-opt vs exact (same 100 trips per k as E4)\n";
    const GridMap& map = warehouseMap();
    DistanceTable dist(map);
    RoutePlanner planner(map, dist);
    std::ofstream csv = openCsv("e10_route_improvement.csv", "k,trial,greedy,two_opt,exact,two_opt_evaluations,greedy_us,two_opt_us,exact_us");
    std::cout << std::right << std::setw(3) << "k" << std::setw(9) << "greedy" << std::setw(9) << "2-opt" << std::setw(9) << "exact"
              << std::setw(11) << "gap greedy" << std::setw(11) << "gap 2-opt" << std::setw(13) << "2-opt=exact" << std::setw(12) << "evals 2-opt"
              << std::setw(11) << "greedy us" << std::setw(10) << "2opt us" << std::setw(11) << "exact us" << '\n';
    std::mt19937 rng(2026);   // same seed and sampling as experimentRouting(), so the trips are identical
    for (int k = 1; k <= 10; ++k) {
        double sumGreedy = 0, sumTwoOpt = 0, sumExact = 0, usGreedy = 0, usTwoOpt = 0, usExact = 0;
        long sumEvals = 0;
        int twoOptIsExact = 0;
        for (int trial = 0; trial < 100; ++trial) {
            std::vector<CellIndex> shelves = map.shelves();
            std::shuffle(shelves.begin(), shelves.end(), rng);
            shelves.resize(static_cast<std::size_t>(k));
            const CellIndex start = map.homes()[static_cast<std::size_t>(trial) % map.homes().size()];
            const CellIndex end = map.inductionCellFor(map.sorters()[static_cast<std::size_t>(trial) % map.sorters().size()]);

            auto t0 = Clock::now();
            const RoutePlan greedy = planner.greedy(start, shelves, end);
            const double greedyUs = msSince(t0) * 1000.0;

            t0 = Clock::now();
            std::vector<Stop> route;
            for (std::size_t i = 0; i < greedy.order.size(); ++i) route.push_back({greedy.order[i], greedy.access[i]});
            long evals = 0;
            route = improveRoute(map, dist, start, route, end, shelves, evals);
            const int twoOptLength = routeLength(dist, start, route, end);
            const double twoOptUs = greedyUs + msSince(t0) * 1000.0;   // includes building the greedy route it starts from

            t0 = Clock::now();
            const RoutePlan exact = planner.backtracking(start, shelves, end, true);
            const double exactUs = msSince(t0) * 1000.0;

            sumGreedy += greedy.length;
            sumTwoOpt += twoOptLength;
            sumExact += exact.length;
            sumEvals += evals;
            usGreedy += greedyUs;
            usTwoOpt += twoOptUs;
            usExact += exactUs;
            if (twoOptLength == exact.length) ++twoOptIsExact;
            csv << k << ',' << trial << ',' << greedy.length << ',' << twoOptLength << ',' << exact.length << ',' << evals << ','
                << fmt(greedyUs, 2) << ',' << fmt(twoOptUs, 2) << ',' << fmt(exactUs, 2) << '\n';
        }
        std::cout << std::setw(3) << k << std::setw(9) << fmt(sumGreedy / 100, 1) << std::setw(9) << fmt(sumTwoOpt / 100, 1) << std::setw(9)
                  << fmt(sumExact / 100, 1) << std::setw(10) << fmt(100.0 * (sumGreedy - sumExact) / sumExact, 1) << '%' << std::setw(10)
                  << fmt(100.0 * (sumTwoOpt - sumExact) / sumExact, 1) << '%' << std::setw(10) << twoOptIsExact << "/100" << std::setw(12)
                  << sumEvals / 100 << std::setw(11) << fmt(usGreedy / 100, 1) << std::setw(10) << fmt(usTwoOpt / 100, 1) << std::setw(11)
                  << fmt(usExact / 100, 1) << '\n';
    }
}

// ===========================================================================
// E14 Exact routes: Held-Karp dynamic programming vs backtracking (WP-C, navigation)
//
// Question: backtracking with pruning is exact but its worst case is k! * 2^k leaves
// (k shelves, 2 sides each). Is there an exact method that scales better, and does it
// justify a larger exactLimit than 8?
//
// Held-Karp: best[S][j][s] = fewest moves to start, visit exactly the shelves in set S
// and finish at shelf j standing on side s. Each subset is solved once and reused, so the
// work is O(2^k * k^2 * sides^2) instead of k! * 2^k. The cost is memory: 2^k * k * sides numbers.
//
// Standalone, like E10: it only uses the module's GridMap and DistanceTable. Same random
// trips as E4 (same seed and sampling). Before timing, one untimed Held-Karp run fills the
// DistanceTable cache, so both methods are timed on pure table lookups.
// ===========================================================================

namespace {

struct HeldKarpResult {
    int length = kUnreachable;
    long states = 0;   // (subset, last shelf, side) states that were reached = the work done
};

HeldKarpResult heldKarp(const GridMap& map, DistanceTable& dist, CellIndex start, const std::vector<CellIndex>& shelves, CellIndex end)
{
    const std::size_t k = shelves.size();
    HeldKarpResult result;
    if (k == 0) {
        result.length = dist.distance(start, end);
        return result;
    }
    std::vector<std::vector<CellIndex>> cells(k);
    std::size_t sides = 1;
    for (std::size_t i = 0; i < k; ++i) {
        cells[i] = map.accessCells(shelves[i]);
        sides = std::max(sides, cells[i].size());
    }
    const int inf = kUnreachable;
    const std::size_t full = (std::size_t{1} << k) - 1;
    auto at = [&](std::size_t mask, std::size_t i, std::size_t s) { return (mask * k + i) * sides + s; };
    std::vector<int> best((full + 1) * k * sides, inf);

    for (std::size_t i = 0; i < k; ++i)                                // first shelf: walk from the start
        for (std::size_t s = 0; s < cells[i].size(); ++s) best[at(std::size_t{1} << i, i, s)] = dist.distance(start, cells[i][s]);

    for (std::size_t mask = 1; mask <= full; ++mask) {
        for (std::size_t j = 0; j < k; ++j) {
            if (!(mask & (std::size_t{1} << j))) continue;
            for (std::size_t s = 0; s < cells[j].size(); ++s) {
                const int here = best[at(mask, j, s)];
                if (here >= inf) continue;
                ++result.states;
                if (mask == full) {
                    result.length = std::min(result.length, here + dist.distance(cells[j][s], end));
                    continue;
                }
                for (std::size_t i = 0; i < k; ++i) {                  // next shelf
                    if (mask & (std::size_t{1} << i)) continue;
                    for (std::size_t t = 0; t < cells[i].size(); ++t) {
                        const int step = dist.distance(cells[j][s], cells[i][t]);
                        if (step >= inf) continue;
                        int& target = best[at(mask | (std::size_t{1} << i), i, t)];
                        target = std::min(target, here + step);
                    }
                }
            }
        }
    }
    return result;
}

} // namespace

void experimentHeldKarp()
{
    std::cout << "\nE14  EXACT ROUTES: Held-Karp DP vs backtracking (same trips as E4 for k <= 10)\n";
    const GridMap& map = warehouseMap();
    DistanceTable dist(map);
    RoutePlanner planner(map, dist);
    std::ofstream csv = openCsv("e14_heldkarp.csv", "k,trial,backtracking,heldkarp,backtracking_nodes,heldkarp_states,backtracking_us,heldkarp_us");
    std::cout << std::right << std::setw(3) << "k" << std::setw(8) << "trips" << std::setw(11) << "exact len" << std::setw(11) << "mismatch" << std::setw(14) << "bt nodes"
              << std::setw(14) << "hk states" << std::setw(13) << "bt us" << std::setw(13) << "hk us" << std::setw(11) << "bt/hk" << '\n';
    std::mt19937 rng(2026);   // same seed and sampling as experimentRouting()
    for (int k = 1; k <= 14; ++k) {
        const int trips = k <= 10 ? 100 : 20;     // backtracking becomes too slow beyond k = 10: fewer trips
        const bool withBacktracking = k <= 12;    // beyond 12 only Held-Karp is run
        double sumLen = 0, sumBtUs = 0, sumHkUs = 0;
        long sumBtNodes = 0, sumHkStates = 0;
        int mismatches = 0;
        for (int trial = 0; trial < trips; ++trial) {
            std::vector<CellIndex> shelves = map.shelves();
            std::shuffle(shelves.begin(), shelves.end(), rng);
            shelves.resize(static_cast<std::size_t>(k));
            const CellIndex start = map.homes()[static_cast<std::size_t>(trial) % map.homes().size()];
            const CellIndex end = map.inductionCellFor(map.sorters()[static_cast<std::size_t>(trial) % map.sorters().size()]);

            heldKarp(map, dist, start, shelves, end);   // untimed: fills the DistanceTable cache
            auto t0 = Clock::now();
            const HeldKarpResult hk = heldKarp(map, dist, start, shelves, end);
            const double hkUs = msSince(t0) * 1000.0;

            int btLength = -1;
            long btNodes = -1;
            double btUs = -1;
            if (withBacktracking) {
                t0 = Clock::now();
                const RoutePlan bt = planner.backtracking(start, shelves, end, true);
                btUs = msSince(t0) * 1000.0;
                btLength = bt.length;
                btNodes = bt.nodes;
                if (bt.length != hk.length) ++mismatches;
                sumBtUs += btUs;
                sumBtNodes += btNodes;
            }
            sumLen += hk.length;
            sumHkUs += hkUs;
            sumHkStates += hk.states;
            csv << k << ',' << trial << ',' << btLength << ',' << hk.length << ',' << btNodes << ',' << hk.states << ',' << fmt(btUs, 2) << ',' << fmt(hkUs, 2) << '\n';
        }
        std::cout << std::setw(3) << k << std::setw(8) << trips << std::setw(11) << fmt(sumLen / trips, 1) << std::setw(11) << (withBacktracking ? std::to_string(mismatches) : "-")
                  << std::setw(14) << (withBacktracking ? std::to_string(sumBtNodes / trips) : "-") << std::setw(14) << sumHkStates / trips << std::setw(13)
                  << (withBacktracking ? fmt(sumBtUs / trips, 1) : "-") << std::setw(13) << fmt(sumHkUs / trips, 1) << std::setw(11)
                  << (withBacktracking ? fmt(sumBtUs / sumHkUs, 2) : "-") << '\n';
    }
}

int main(int argc, char** argv)
{
    std::vector<std::string> args(argv + 1, argv + argc);
    std::string which = "all";
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "--out" && i + 1 < args.size()) g_outDir = args[++i];
        else which = args[i];
    }
    const bool all = which == "all";
    bool ran = false;
    if (all || which == "sorting") { experimentSorting(); ran = true; }
    if (all || which == "search") { experimentSearch(); ran = true; }
    if (all || which == "pathfinding") { experimentPathfinding(); ran = true; }
    if (all || which == "routing") { experimentRouting(); ran = true; }
    if (all || which == "traffic") { experimentTraffic(); ran = true; }
    if (all || which == "policies") { experimentPolicies(); ran = true; }
    if (all || which == "fulfilment") { experimentFulfilment(); ran = true; }
    if (all || which == "trees") { experimentTrees(); ran = true; }
    if (all || which == "substring") { experimentSubstring(); ran = true; }
    if (all || which == "improve") { experimentRouteImprovement(); ran = true; }
    if (all || which == "heldkarp") { experimentHeldKarp(); ran = true; }
    if (!ran) {
        std::cerr << "usage: experiments [sorting|search|pathfinding|routing|traffic|policies|fulfilment|trees|substring|improve|all|heldkarp] [--out DIR]\n";
        return 2;
    }
    return 0;
}
