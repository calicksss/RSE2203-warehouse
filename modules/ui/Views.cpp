#include "ui/Views.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <map>

#include "ds/Sorting.hpp"

namespace wh::ui {

using namespace ftxui;

// ---------------------------------------------------------------------------
// Theme
// ---------------------------------------------------------------------------
namespace theme {
Color black() { return Color::RGB(0, 0, 0); }
Color amber() { return Color::RGB(255, 175, 0); }
Color orange() { return Color::RGB(255, 135, 0); }
Color text() { return Color::RGB(215, 215, 215); }
Color white() { return Color::RGB(255, 255, 255); }
Color dim() { return Color::RGB(135, 135, 135); }
Color faint() { return Color::RGB(70, 70, 70); }
Color border() { return Color::RGB(175, 95, 0); }
Color green() { return Color::RGB(0, 215, 135); }
Color red() { return Color::RGB(255, 95, 95); }
Color cyan() { return Color::RGB(95, 215, 215); }
Color blue() { return Color::RGB(95, 175, 255); }
Color yellow() { return Color::RGB(255, 255, 95); }
Color magenta() { return Color::RGB(215, 135, 255); }
Color headerBg() { return Color::RGB(48, 48, 48); }
Color tickerBg() { return Color::RGB(28, 28, 28); }
Color rowAlt() { return Color::RGB(16, 16, 16); }
Color gaugeBg() { return Color::RGB(38, 38, 38); }

Color robot(int number)
{
    static const std::array<std::array<std::uint8_t, 3>, 10> kColours{{{0, 255, 0},
                                                                     {0, 255, 255},
                                                                     {255, 135, 255},
                                                                     {255, 255, 0},
                                                                     {0, 175, 255},
                                                                     {255, 135, 0},
                                                                     {175, 135, 255},
                                                                     {135, 255, 0},
                                                                     {255, 95, 95},
                                                                     {95, 255, 255}}};
    const auto& c = kColours[static_cast<std::size_t>((number - 1) % 10)];
    return Color::RGB(c[0], c[1], c[2]);
}

Color battery(double percent)
{
    if (percent < 30.0) return red();
    if (percent < 60.0) return amber();
    return green();
}

Color status(OrderStatus s)
{
    switch (s) {
        case OrderStatus::Queued:    return dim();
        case OrderStatus::Picking:   return amber();
        case OrderStatus::Sorting:   return cyan();
        case OrderStatus::Packing:   return magenta();
        case OrderStatus::Completed: return green();
    }
    return text();
}

Color severity(Severity s)
{
    switch (s) {
        case Severity::Info:    return text();
        case Severity::Success: return green();
        case Severity::Warning: return yellow();
        case Severity::Alert:   return red();
    }
    return text();
}
} // namespace theme

// ---------------------------------------------------------------------------
// Formatting
// ---------------------------------------------------------------------------
std::string fixed(double value, int decimals)
{
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.*f", decimals, value);
    return buf;
}

std::string withCommas(long value)
{
    std::string digits = std::to_string(value < 0 ? -value : value);
    for (int i = static_cast<int>(digits.size()) - 3; i > 0; i -= 3) digits.insert(static_cast<std::size_t>(i), ",");
    return (value < 0 ? "-" : "") + digits;
}

std::string padLeft(std::string s, std::size_t width, char fill)
{
    if (s.size() < width) s.insert(s.begin(), width - s.size(), fill);
    return s;
}

std::string wallClock()
{
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    char buf[16];
    std::strftime(buf, sizeof buf, "%H:%M:%S", &local);
    return buf;
}

int pickerNumber(const PickingRobot& p) { return std::stoi(p.name().substr(4)); }

// ---------------------------------------------------------------------------
// Building blocks
// ---------------------------------------------------------------------------
Element cell(const std::string& s, int width, bool alignRight)
{
    Element e = text(s);
    if (alignRight) e = e | align_right;
    return e | size(WIDTH, EQUAL, width);
}

Element panel(const std::string& title, Element content, const std::string& info)
{
    Element heading = hbox({
        text(" " + title + " ") | bold | color(theme::black()) | bgcolor(theme::amber()),
        filler(),
        info.empty() ? text("") : text(" " + info + " ") | color(theme::amber()),
    });
    return window(heading, std::move(content) | color(theme::text()), LIGHT) | color(theme::border());
}

Element titleBar(const std::string& left, const std::string& right)
{
    return hbox({text(" " + left), filler(), text(right + " ")}) | bold | color(theme::black()) | bgcolor(theme::orange());
}

Element tickerBar(const std::vector<TickerItem>& items)
{
    Elements parts{text(" ")};
    for (const auto& it : items) {
        parts.push_back(text(it.label + " ") | color(theme::dim()));
        parts.push_back(text(it.value) | bold | color(it.colour));
        parts.push_back(text("   "));
    }
    return hbox(std::move(parts)) | bgcolor(theme::tickerBg());
}

Element keyHints(const std::vector<std::pair<std::string, std::string>>& keys)
{
    Elements parts{text(" ")};
    for (const auto& [key, label] : keys) {
        parts.push_back(text(" " + key + " ") | bold | color(theme::black()) | bgcolor(theme::amber()));
        parts.push_back(text(" " + label + "  ") | color(theme::text()));
    }
    parts.push_back(filler());
    return hbox(std::move(parts)) | bgcolor(theme::tickerBg());
}

Element meter(double fraction, Color colour, int width)
{
    return gauge(static_cast<float>(std::clamp(fraction, 0.0, 1.0))) | color(colour) | bgcolor(theme::gaugeBg()) |
           size(WIDTH, EQUAL, width);
}

Element headerRow(Elements cells)
{
    return hbox(std::move(cells)) | bold | color(theme::amber()) | bgcolor(theme::headerBg());
}

// ---------------------------------------------------------------------------
// Live dashboard
// ---------------------------------------------------------------------------
std::string simStateLabel(const Simulation& sim, bool paused)
{
    if (sim.isQuiescent()) return sim.metrics().ordersSubmitted == 0 ? "■ IDLE · NO ORDERS" : "■ ALL ORDERS COMPLETE";
    return paused ? "❚❚ PAUSED" : "▶ LIVE";
}

std::vector<TickerItem> simTickerItems(const Simulation& sim)
{
    const Metrics& m = sim.metrics();
    int busy = 0;
    for (const PickingRobot* p : sim.pickers()) busy += p->isBusy() ? 1 : 0;
    const double expanded = m.astarCalls ? double(m.astarExpanded) / double(m.astarCalls) : 0.0;
    return {
        {"ORDERS", std::to_string(m.ordersCompleted) + "/" + std::to_string(m.ordersSubmitted),
         m.ordersCompleted == m.ordersSubmitted ? theme::green() : theme::white()},
        {"BACKLOG", std::to_string(sim.jobs().backlogSize()), sim.jobs().backlogSize() ? theme::yellow() : theme::white()},
        {"THRU", fixed(m.throughput(sim.now()), 2) + "/100t", theme::white()},
        {"LEAD", fixed(m.avgLeadTime(), 1) + "t", theme::white()},
        {"BUSY", std::to_string(busy) + "/" + std::to_string(sim.pickers().size()), theme::white()},
        {"CONGEST", fixed(m.congestion() * 100.0, 1) + "%", m.congestion() > 0.1 ? theme::red() : theme::green()},
        {"DIST", withCommas(m.moves), theme::white()},
        {"A* CELLS/PLAN", fixed(expanded, 1), theme::cyan()},
        {"MAX WAIT", std::to_string(m.longestWait) + "t", m.longestWait > 30 ? theme::red() : theme::white()},
        {"COLLISIONS", std::to_string(m.collisions), m.collisions ? theme::red() : theme::green()},
    };
}

Element floorMap(const Simulation& sim, bool showPaths, int cw)
{
    const GridMap& map = sim.map();

    // Overlays: planned routes, goals, shelves on active trips.
    std::map<CellIndex, Color> pathColour, goalColour, activeShelf;
    for (std::size_t i = 0; i < sim.pickers().size(); ++i) {
        const PickingRobot* p = sim.pickers()[i];
        const Color c = theme::robot(pickerNumber(*p));
        if (showPaths && p->wantsToMove()) {
            for (CellIndex cell : sim.traffic().plannedPath(static_cast<int>(i))) pathColour.emplace(cell, c);
            goalColour[p->goal()] = c;
        }
        for (CellIndex shelf : p->shelvesLeft()) activeShelf[shelf] = c;
    }
    std::map<CellIndex, const PickingRobot*> robotAt;
    for (const PickingRobot* p : sim.pickers()) robotAt[p->cell()] = p;
    std::map<CellIndex, int> sorterAt, dockAt;
    for (const SortingRobot* s : sim.sorters()) sorterAt[s->cell()] = s->index() + 1;
    for (const DockingStation& d : sim.docks()) dockAt[d.cell()] = d.id() + 1;

    // Two-column cells: pad one-column glyphs. Measure display width, not bytes ("·" is 2 bytes in UTF-8).
    auto pad = [cw](const std::string& g) { return cw == 2 && string_width(g) == 1 ? g + " " : g; };

    Elements rows;
    for (int r = 0; r < map.rows(); ++r) {
        Elements cells;
        for (int c = 0; c < map.cols(); ++c) {
            const CellIndex idx = map.index({r, c});
            std::string g;
            Color fg = theme::faint(), bg = theme::black();
            bool strong = false;
            if (const auto it = robotAt.find(idx); it != robotAt.end()) {
                const int n = pickerNumber(*it->second);
                g = cw == 2 ? "P" + std::to_string(n % 10) : std::to_string(n % 10);
                fg = theme::black();
                bg = theme::robot(n);
                strong = true;
            } else if (sim.obstacles().isBlocked(idx)) {
                g = cw == 2 ? "!!" : "!";
                fg = theme::white();
                bg = Color::RGB(200, 0, 0);
                strong = true;
            } else {
                switch (map.tile(idx)) {
                    case Tile::Wall:   g = cw == 2 ? "██" : "█"; fg = Color::RGB(58, 58, 58); break;
                    case Tile::Shelf: {
                        const auto a = activeShelf.find(idx);
                        g = cw == 2 ? "▓▓" : "▓";
                        fg = a != activeShelf.end() ? a->second : Color::RGB(135, 95, 0);
                        break;
                    }
                    case Tile::Sorter:
                        g = cw == 2 ? "R" + std::to_string(sorterAt[idx]) : "R";
                        fg = theme::black(); bg = theme::magenta(); strong = true;
                        break;
                    case Tile::Dock:
                        g = cw == 2 ? "C" + std::to_string(dockAt[idx]) : "C";
                        fg = theme::blue(); bg = Color::RGB(0, 0, 95); strong = true;
                        break;
                    case Tile::Floor:
                        if (const auto gl = goalColour.find(idx); gl != goalColour.end()) {
                            g = pad("◇"); fg = gl->second; strong = true;
                        } else if (const auto pc = pathColour.find(idx); pc != pathColour.end()) {
                            g = pad("•"); fg = pc->second;
                        } else {
                            g = pad(map.isHome(idx) ? "▫" : "·");
                            if (map.isHome(idx)) fg = Color::RGB(108, 108, 108);
                        }
                        break;
                }
            }
            Element e = text(g) | color(fg) | bgcolor(bg);
            if (strong) e = e | bold;
            cells.push_back(e);
        }
        rows.push_back(hbox(std::move(cells)));
    }
    auto legend = [](const std::string& sym, Color fg, Color bg, const std::string& label) {
        return hbox({text(sym) | bold | color(fg) | bgcolor(bg), text(" " + label + "  ") | color(theme::dim())});
    };
    rows.push_back(hbox({
        legend("P#", theme::black(), theme::robot(1), "picker"),
        legend("R#", theme::black(), theme::magenta(), "sorter"),
        legend("C#", theme::blue(), Color::RGB(0, 0, 95), "dock"),
        legend("▓", Color::RGB(135, 95, 0), theme::black(), "shelf"),
        legend("!!", theme::white(), Color::RGB(200, 0, 0), "obstacle"),
        legend("•◇", theme::cyan(), theme::black(), "route/goal"),
    }));
    return panel("1) FLOOR MAP", vbox(std::move(rows)),
                 std::to_string(map.rows()) + "x" + std::to_string(map.cols()) + " · " + std::to_string(map.shelves().size()) + " SKUs");
}

namespace {

Color pickerStateColour(const PickingRobot& p)
{
    using S = PickingRobot::State;
    if (p.battery() <= 0.0) return theme::red();
    if (p.wantsToMove() && p.waitStreak() >= 3) return theme::yellow();
    switch (p.state()) {
        case S::Idle:      return theme::dim();
        case S::Picking:   return theme::amber();
        case S::Dropping:  return theme::cyan();
        case S::ToDock:
        case S::Charging:  return theme::blue();
        case S::ToShelf:
        case S::ToSorter:
        case S::Returning: return theme::green();
    }
    return theme::text();
}

} // namespace

Element fleetPanel(const Simulation& sim)
{
    int busy = 0;
    for (const PickingRobot* p : sim.pickers()) busy += p->hasWork() ? 1 : 0;
    Elements rows{headerRow({cell("UNIT", 8), cell("STATE", 11), cell("POS", 7), cell("BATTERY", 16), cell("LOAD", 5, true),
                             cell("TRIP", 6, true), text(" ACTIVITY") | flex})};
    for (const PickingRobot* p : sim.pickers()) {
        const int n = pickerNumber(*p);
        std::string label = p->stateLabel();
        if (p->wantsToMove() && p->waitStreak() >= 3 && p->battery() > 0.0) label = "YIELDING";
        const GridPos pos = sim.map().pos(p->cell());
        const int pct = static_cast<int>(std::lround(p->battery()));
        rows.push_back(hbox({
            text("■ ") | color(theme::robot(n)),
            cell(p->name(), 6) | bold | color(theme::white()),
            cell(label, 11) | bold | color(pickerStateColour(*p)),
            cell(std::to_string(pos.row) + "," + std::to_string(pos.col), 7),
            meter(p->battery() / 100.0, theme::battery(p->battery()), 10),
            cell(std::to_string(pct) + "%", 6, true) | color(theme::battery(p->battery())),
            cell(p->hasWork() ? std::to_string(p->cargo().size()) + "/" + std::to_string(p->unitsOnTrip()) : "-", 5, true) |
                color(theme::white()),
            cell(p->hasWork() ? "#" + std::to_string(p->tripId()) : "-", 6, true) | color(theme::dim()),
            text(" " + p->activity()) | flex,
        }));
    }
    return panel("2) FLEET MONITOR · PICKERS", vbox(std::move(rows)),
                 std::to_string(busy) + "/" + std::to_string(sim.pickers().size()) + " ON TRIP");
}

Element sorterPanel(const Simulation& sim)
{
    long sorted = 0;
    for (const SortingRobot* s : sim.sorters()) sorted += s->unitsSorted();
    Elements rows{headerRow({cell("UNIT", 8), cell("STATE", 9), cell("QUEUE", 6, true), text("  BINS (order sorted/total)") | flex,
                             cell("DONE", 6, true)})};
    for (const SortingRobot* s : sim.sorters()) {
        const std::string state = s->stateLabel();
        const Color sc = state == "IDLE" ? theme::dim() : state == "SORTING" ? theme::cyan() : state == "PACKING" ? theme::green() : theme::amber();
        Elements bins{text("  ")};
        for (const auto& bin : s->bins()) {
            if (bin.order == kNoOrder) {
                bins.push_back(text("[  ----  ]") | color(theme::faint()));
            } else if (bin.packTicksLeft > 0) {
                bins.push_back(text("[#" + padLeft(std::to_string(bin.order), 3) + " PACK]") | bold | color(theme::black()) |
                               bgcolor(theme::green()));
            } else {
                const int inBin = static_cast<int>(bin.contents.size());
                const std::string body = "#" + std::to_string(bin.order) + " " + std::to_string(inBin) + "/" + std::to_string(bin.expected);
                bins.push_back(text("[" + padLeft(body, 8).substr(0, 8) + "]") | color(inBin ? theme::cyan() : theme::text()));
            }
            bins.push_back(text(" "));
        }
        rows.push_back(hbox({
            text("■ ") | color(theme::magenta()),
            cell(s->name(), 6) | bold | color(theme::white()),
            cell(state, 9) | bold | color(sc),
            cell(std::to_string(s->queuedUnits()), 6, true) | color(s->queuedUnits() ? theme::yellow() : theme::dim()),
            hbox(std::move(bins)) | flex,
            cell(std::to_string(s->unitsSorted()), 6, true) | color(theme::white()),
        }));
    }
    return panel("3) SORT STATIONS · PUT-WALL", vbox(std::move(rows)), withCommas(sorted) + " UNITS SORTED");
}

Element dockPanel(const Simulation& sim, const std::string& flash)
{
    Elements docks;
    for (const DockingStation& d : sim.docks()) {
        const PickingRobot* holder = nullptr;
        for (const PickingRobot* p : sim.pickers())
            if (p->id() == d.holder()) holder = p;
        Element status;
        if (!holder) {
            status = text("free") | color(theme::dim());
        } else if (holder->state() == PickingRobot::State::Charging) {
            status = hbox({text(holder->name() + " ") | bold | color(theme::robot(pickerNumber(*holder))),
                           meter(holder->battery() / 100.0, theme::battery(holder->battery()), 8),
                           text(" " + std::to_string(static_cast<int>(std::lround(holder->battery()))) + "%")});
        } else {
            status = hbox({text(holder->name() + " ") | bold | color(theme::robot(pickerNumber(*holder))), text("en route") | color(theme::blue())});
        }
        docks.push_back(hbox({text(" C" + std::to_string(d.id() + 1) + " ") | bold | color(theme::blue()) | bgcolor(Color::RGB(0, 0, 95)),
                              text(" "), status}) |
                        size(WIDTH, EQUAL, 26));
    }
    std::string obstacles = "Obstacles: " + std::to_string(sim.obstacles().active().size()) + " active";
    for (const Obstacle& o : sim.obstacles().active())
        obstacles += "  " + toString(sim.map().pos(o.cell)) + " clears t" + std::to_string(o.expires);
    return panel("4) CHARGING & FLOOR",
                 vbox({
                     hflow(std::move(docks)),
                     text(obstacles) | color(sim.obstacles().active().empty() ? theme::dim() : theme::yellow()),
                     flash.empty() ? text("") : text("› " + flash) | color(theme::amber()),
                 }),
                 std::string("RANDOM OBSTACLES ") + (sim.config().randomObstacles ? "ON" : "OFF"));
}

Element orderBookPanel(const Simulation& sim)
{
    const JobManager& jobs = sim.jobs();
    std::vector<const OrderRecord*> active, done;
    for (OrderId id : jobs.orderIds()) {
        const OrderRecord* p = jobs.record(id);
        (p->status == OrderStatus::Completed ? done : active).push_back(p);
    }
    // Orders in progress before queued ones; finished orders newest first. Our own (stable) merge sort.
    mergeSort(active, [](const OrderRecord* a, const OrderRecord* b) {
        return (a->status == OrderStatus::Queued) < (b->status == OrderStatus::Queued);
    });
    mergeSort(done, [](const OrderRecord* a, const OrderRecord* b) { return a->completed > b->completed; });

    Elements rows{headerRow({cell("ORDER", 7), cell("PRI", 5), cell("STATUS", 10), cell("SRT", 4), cell("PICKED", 7, true),
                             text("  PROGRESS") | flex, cell("AGE", 7, true)})};
    int k = 0;
    auto add = [&](const OrderRecord* p) {
        const bool express = p->order.priority == Priority::Express;
        const double frac = p->status == OrderStatus::Completed
                                ? 1.0
                                : (p->unitsTotal ? (p->unitsPicked + p->unitsSorted) / (2.0 * p->unitsTotal) : 0.0);
        const Tick age = (p->completed >= 0 ? p->completed : sim.now()) - p->submitted;
        rows.push_back(hbox({
                           cell("#" + std::to_string(p->order.id), 7) | bold | color(theme::white()),
                           cell(express ? "EXP" : "STD", 5) | color(express ? theme::orange() : theme::dim()),
                           cell(toString(p->status), 10) | bold | color(theme::status(p->status)),
                           cell(p->sorterIndex >= 0 ? "R" + std::to_string(p->sorterIndex + 1) : "-", 4) | color(theme::magenta()),
                           cell(std::to_string(p->unitsPicked) + "/" + std::to_string(p->unitsTotal), 7, true),
                           text("  "),
                           meter(frac, theme::status(p->status), 12),
                           filler(),
                           cell(std::to_string(age) + "t", 7, true) | color(p->status == OrderStatus::Completed ? theme::green() : theme::text()),
                       }) |
                       bgcolor(k++ % 2 ? theme::rowAlt() : theme::black()));
    };
    for (const OrderRecord* p : active) add(p);
    for (const OrderRecord* p : done) add(p);
    if (active.empty() && done.empty())
        rows.push_back(text("No orders yet. Check out from the shop (F2) or generate test orders (F5).") | color(theme::dim()));
    return panel("5) ORDER BOOK", vbox(std::move(rows)) | yframe,
                 std::to_string(active.size()) + " OPEN · " + std::to_string(done.size()) + " DONE");
}

Element eventPanel(const Simulation& sim)
{
    Elements rows;
    const auto& events = sim.events().events();
    for (auto it = events.rbegin(); it != events.rend() && rows.size() < 60; ++it) {
        rows.push_back(hbox({
            cell(padLeft(std::to_string(it->tick), 5) + " ", 6) | color(theme::dim()),
            cell(it->source, 7) | color(theme::amber()),
            text(it->text) | color(theme::severity(it->severity)) | (it->severity == Severity::Alert ? bold : nothing),
        }));
    }
    return panel("6) EVENT TAPE", vbox(std::move(rows)) | yframe, withCommas(static_cast<long>(sim.events().total())) + " EVENTS");
}

Element liveDashboard(const Simulation& sim, const LiveViewState& state, int terminalWidth)
{
    const int cellWidth = terminalWidth >= sim.map().cols() * 2 + 2 + 84 ? 2 : 1;
    Element right = vbox({fleetPanel(sim), sorterPanel(sim), dockPanel(sim, state.flash) | flex});
    Element top = hbox({floorMap(sim, state.showPaths, cellWidth), right | flex});
    Element bottom = hbox({orderBookPanel(sim) | flex_grow_factor(45), eventPanel(sim) | flex_grow_factor(55)});
    return vbox({top, bottom | flex});
}

} // namespace wh::ui
