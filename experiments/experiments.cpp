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

} // namespace

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
    if (!ran) {
        std::cerr << "usage: experiments [sorting|search|pathfinding|routing|traffic|policies|all] [--out DIR]\n";
        return 2;
    }
    return 0;
}
