#pragma once
/**
 * @file Views.hpp
 * @brief Functions that DRAW the warehouse (model -> FTXUI Element), plus the colour theme.
 *
 * Every function takes the model by const reference and returns an
 * ftxui::Element (a description of what to draw). They cannot change the shop
 * or the simulation. Keys, buttons and focus live in App.
 *
 * Look: a "financial terminal" style (black background, amber labels, white
 * numbers, green = good, red = bad, orange title bar, a strip of key numbers).
 *
 * FTXUI basics used here: text("..."), hbox({...}) = side by side,
 * vbox({...}) = stacked, window(title, body) = box with a border,
 * `element | color(c)` = apply a style. https://github.com/ArthurSonzogni/FTXUI
 */
#include <string>
#include <utility>
#include <vector>

#include "ftxui.hpp"
#include "sim/Simulation.hpp"

namespace wh::ui {

namespace theme {
ftxui::Color black();
ftxui::Color amber();
ftxui::Color orange();
ftxui::Color text();
ftxui::Color white();
ftxui::Color dim();
ftxui::Color faint();
ftxui::Color border();
ftxui::Color green();
ftxui::Color red();
ftxui::Color cyan();
ftxui::Color blue();
ftxui::Color yellow();
ftxui::Color magenta();
ftxui::Color headerBg();
ftxui::Color tickerBg();
ftxui::Color rowAlt();
ftxui::Color gaugeBg();

ftxui::Color robot(int number);       ///< Identity colour of picker n.
ftxui::Color battery(double percent); ///< Green / amber / red.
ftxui::Color status(OrderStatus s);
ftxui::Color severity(Severity s);
} // namespace theme

// ---- Building blocks -------------------------------------------------------
struct TickerItem {
    std::string label;
    std::string value;
    ftxui::Color colour;
};

/// Fixed-width table cell (text is clipped to the width).
ftxui::Element cell(const std::string& text, int width, bool alignRight = false);
/// Bordered panel with an amber title tag and optional right-aligned info.
ftxui::Element panel(const std::string& title, ftxui::Element content, const std::string& info = "");
/// Orange full-width title bar.
ftxui::Element titleBar(const std::string& left, const std::string& right);
/// Grey strip of "LABEL value" KPIs.
ftxui::Element tickerBar(const std::vector<TickerItem>& items);
/// Function-key strip: (key, label) pairs.
ftxui::Element keyHints(const std::vector<std::pair<std::string, std::string>>& keys);
/// Coloured horizontal gauge of fixed width.
ftxui::Element meter(double fraction, ftxui::Color colour, int width);
/// Column header row on a grey band.
ftxui::Element headerRow(ftxui::Elements cells);

// ---- Live operations dashboard --------------------------------------------
struct LiveViewState {
    bool paused = false;
    int ticksPerSecond = 10;
    bool showPaths = true;
    std::string flash;  ///< Feedback for the last command.
};

std::vector<TickerItem> simTickerItems(const Simulation& sim);
ftxui::Element floorMap(const Simulation& sim, bool showPaths, int cellWidth);
ftxui::Element fleetPanel(const Simulation& sim);
ftxui::Element sorterPanel(const Simulation& sim);
ftxui::Element dockPanel(const Simulation& sim, const std::string& flash);
ftxui::Element orderBookPanel(const Simulation& sim);
ftxui::Element eventPanel(const Simulation& sim);
/// The whole dashboard body. `terminalWidth` decides the map cell width.
ftxui::Element liveDashboard(const Simulation& sim, const LiveViewState& state, int terminalWidth);
/// "LIVE", "PAUSED", "ALL ORDERS COMPLETE", ... for the title bar.
std::string simStateLabel(const Simulation& sim, bool paused);

// ---- Formatting ------------------------------------------------------------
std::string fixed(double value, int decimals);
std::string withCommas(long value);
std::string padLeft(std::string s, std::size_t width, char fill = ' ');
std::string wallClock();
int pickerNumber(const PickingRobot& p);

} // namespace wh::ui
