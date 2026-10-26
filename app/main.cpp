/**
 * @file main.cpp
 * @brief Program entry point: reads the command line, loads the data files and starts a mode.
 *
 *   Warehouse                    interactive shop + live warehouse (FTXUI)
 *   Warehouse --headless 80      simulate 80 random orders without a UI and print the results
 *   Warehouse --snapshot 150     print one dashboard picture after 150 ticks (for the report)
 *
 * Options: --data DIR  --seed N  --pickers N  --routing asgiven|greedy|backtracking
 *          --assignment first|nearest  --no-obstacles  --size WxH  --help
 *
 * For algorithm comparisons use the separate `experiments` program.
 */
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "core/Errors.hpp"
#include "shop/Shop.hpp"
#include "sim/Simulation.hpp"
#include "ui/App.hpp"
#include "ui/Views.hpp"

#ifndef WH_DEFAULT_DATA_DIR
#define WH_DEFAULT_DATA_DIR "data"
#endif

using namespace wh;

namespace {

struct Options {
    std::string dataDir;   ///< "" = search the usual places
    SimConfig config;
    enum class Mode { Interactive, Headless, Snapshot } mode = Mode::Interactive;
    int orders = 60;
    int ticks = 150;
    int width = 170;
    int height = 50;
};

void printUsage()
{
    std::cout << "Automated Grocery Warehouse (RSE2203 team project)\n\n"
                 "Usage: Warehouse [mode] [options]\n\n"
                 "Modes:\n"
                 "  (none)                 interactive shop and live warehouse\n"
                 "  --headless N           simulate N random orders without a UI and print the results\n"
                 "  --snapshot T           print one dashboard picture after T ticks of a demo load\n\n"
                 "Options:\n"
                 "  --data DIR             folder with Map.txt and Products.txt\n"
                 "  --seed N               random seed (default 2026); same seed = same run\n"
                 "  --pickers N            use at most N picking robots\n"
                 "  --routing M            asgiven | greedy | backtracking (default)\n"
                 "  --assignment M         first | nearest (default)\n"
                 "  --no-obstacles         no random obstacles\n"
                 "  --size WxH             picture size for --snapshot (default 170x50)\n"
                 "  --help                 show this help\n";
}

int readNumber(const std::vector<std::string>& args, std::size_t& i, const std::string& flag)
{
    if (i + 1 >= args.size()) throw std::invalid_argument(flag + " needs a number");
    const std::string& text = args[++i];
    std::size_t used = 0;
    int value = -1;
    try {
        value = std::stoi(text, &used);
    } catch (const std::exception&) {
        used = 0;
    }
    if (used != text.size() || value < 0) throw std::invalid_argument(flag + " needs a whole number >= 0, got '" + text + "'");
    return value;
}

std::string readWord(const std::vector<std::string>& args, std::size_t& i, const std::string& flag)
{
    if (i + 1 >= args.size()) throw std::invalid_argument(flag + " needs a value");
    return args[++i];
}

Options parseArgs(int argc, char** argv)
{
    Options o;
    const std::vector<std::string> args(argv + 1, argv + argc);
    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& a = args[i];
        if (a == "--help" || a == "-h") {
            printUsage();
            std::exit(0);
        } else if (a == "--data") {
            o.dataDir = readWord(args, i, a);
        } else if (a == "--seed") {
            o.config.seed = static_cast<unsigned>(readNumber(args, i, a));
        } else if (a == "--pickers") {
            o.config.maxPickers = readNumber(args, i, a);
        } else if (a == "--no-obstacles") {
            o.config.randomObstacles = false;
        } else if (a == "--routing") {
            const std::string v = readWord(args, i, a);
            if (v == "asgiven") o.config.routing = RoutingMethod::AsGiven;
            else if (v == "greedy") o.config.routing = RoutingMethod::Greedy;
            else if (v == "backtracking") o.config.routing = RoutingMethod::Backtracking;
            else throw std::invalid_argument("--routing expects asgiven, greedy or backtracking");
        } else if (a == "--assignment") {
            const std::string v = readWord(args, i, a);
            if (v == "first") o.config.assignment = AssignmentMethod::FirstAvailable;
            else if (v == "nearest") o.config.assignment = AssignmentMethod::Nearest;
            else throw std::invalid_argument("--assignment expects first or nearest");
        } else if (a == "--headless") {
            o.mode = Options::Mode::Headless;
            o.orders = readNumber(args, i, a);
        } else if (a == "--snapshot") {
            o.mode = Options::Mode::Snapshot;
            o.ticks = readNumber(args, i, a);
        } else if (a == "--size") {
            const std::string v = readWord(args, i, a);
            if (std::sscanf(v.c_str(), "%dx%d", &o.width, &o.height) != 2) throw std::invalid_argument("--size expects WxH, e.g. 170x50");
        } else {
            throw std::invalid_argument("unknown option '" + a + "' (see --help)");
        }
    }
    return o;
}

/// Finds the data folder whether the program is started from the project folder, build/ or build/Debug/.
std::string findDataDir(const std::string& explicitDir)
{
    if (!explicitDir.empty()) return explicitDir;
    for (const std::string candidate : {"data", "../data", "../../data", WH_DEFAULT_DATA_DIR}) {
        if (std::filesystem::exists(candidate + "/Map.txt") && std::filesystem::exists(candidate + "/Products.txt")) return candidate;
    }
    throw DataError("data", 0, "cannot find Map.txt and Products.txt; run from the project folder or pass --data DIR");
}

void printResults(const Simulation& sim)
{
    const Metrics& m = sim.metrics();
    std::cout << "Simulated " << sim.now() << " ticks with " << sim.pickers().size() << " pickers and " << sim.sorters().size()
              << " sorters\n"
              << "  routing / assignment   " << toString(sim.config().routing) << " / " << toString(sim.config().assignment) << "\n"
              << "  orders completed       " << m.ordersCompleted << " / " << m.ordersSubmitted << "\n"
              << "  items picked / sorted  " << m.unitsPicked << " / " << m.unitsSorted << "\n"
              << "  trips                  " << m.tripsDispatched << "\n"
              << "  mean lead time         " << ui::fixed(m.avgLeadTime(), 1) << " ticks (order placed -> packed)\n"
              << "  throughput             " << ui::fixed(m.throughput(sim.now()), 2) << " orders per 100 ticks\n"
              << "  distance driven        " << m.moves << " cells\n"
              << "  congestion             " << ui::fixed(100.0 * m.congestion(), 2) << " % of move attempts had to wait\n"
              << "  detours / sidesteps    " << m.detours << " / " << m.sidesteps << "  (" << m.obstaclesSpawned << " obstacles)\n"
              << "  charging sessions      " << m.chargeSessions << "\n"
              << "  longest wait           " << m.longestWait << " ticks\n"
              << "  A* cells per plan      " << ui::fixed(m.astarCalls ? double(m.astarExpanded) / double(m.astarCalls) : 0.0, 1) << "\n"
              << "  collisions             " << m.collisions << "\n";
}

} // namespace

int main(int argc, char** argv)
{
    try {
        const Options opt = parseArgs(argc, argv);
        const std::string dataDir = findDataDir(opt.dataDir);
        const GridMap map = GridMap::loadFromFile(dataDir + "/Map.txt");
        const Catalogue catalogue = Catalogue::loadFromFile(dataDir + "/Products.txt");

        if (opt.mode == Options::Mode::Interactive) {
            ui::App app(map, catalogue, ui::AppOptions{opt.config, "receipts"});
            return app.run();
        }

        Shop shop(catalogue, "");
        Simulation sim(map, shop.catalogue(), opt.config);
        std::mt19937 rng(opt.config.seed);
        for (const Order& o : shop.generateOrders(opt.orders, rng)) sim.submit(o);

        if (opt.mode == Options::Mode::Headless) {
            while (!sim.isQuiescent() && sim.now() < 100000) sim.step();
            printResults(sim);
            return sim.metrics().collisions == 0 && sim.allWorkDone() ? 0 : 1;
        }

        // Snapshot: draw one frame of the live dashboard to the terminal.
        for (int t = 0; t < opt.ticks; ++t) sim.step();
        using namespace ftxui;
        ui::LiveViewState state;
        Element frame = vbox({
                            ui::titleBar("WHSE <GO>  │  AUTOMATED GROCERY FULFILMENT CENTRE  │  LIVE OPERATIONS",
                                         "TICK " + ui::padLeft(std::to_string(sim.now()), 6, '0') + "  │  " + ui::simStateLabel(sim, false)),
                            ui::tickerBar(ui::simTickerItems(sim)),
                            ui::liveDashboard(sim, state, opt.width) | flex,
                        }) |
                        color(ui::theme::text()) | bgcolor(ui::theme::black());
        auto screen = Screen::Create(Dimension::Fixed(opt.width), Dimension::Fixed(opt.height));
        Render(screen, frame);
        std::cout << screen.ToString() << "\x1b[0m\n";
        return 0;
    } catch (const DataError& e) {
        std::cerr << "Data error: " << e.what() << '\n';
        return 2;
    } catch (const std::invalid_argument& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 2;
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << '\n';
        return 1;
    }
}
