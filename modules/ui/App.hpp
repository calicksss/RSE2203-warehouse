#pragma once
/**
 * @file App.hpp
 * @brief The interactive program: tabs, buttons, lists, and the clock that runs the simulation.
 *
 * Screens (F1-F6 or click the tabs): SHOP, CART, LIVE, ORDERS, LOAD, ALGORITHMS. F10 quits.
 *
 * Threads: FTXUI runs the screen on the main ("UI") thread. A small clock
 * thread wakes up every 50 ms and POSTS a job to the UI thread; that job
 * advances the simulation. So the Shop and the Simulation are only ever
 * touched by ONE thread and need no locks.
 */
#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "core/Config.hpp"
#include "ftxui.hpp"
#include "navigation/GridMap.hpp"
#include "shop/Shop.hpp"
#include "sim/Simulation.hpp"
#include "ui/Views.hpp"

namespace wh::ui {

struct AppOptions {
    SimConfig config;
    std::string receiptDirectory = "receipts";
};

class App {
public:
    App(const GridMap& map, const Catalogue& catalogue, const AppOptions& options);
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    /// Runs the full-screen UI until the user quits. Returns the exit code.
    int run();

private:
    enum Tab { kShop, kCart, kLive, kOrders, kLoad, kAlgorithms, kTabCount };

    // Component builders (one per screen)
    ftxui::Component buildShop();
    ftxui::Component buildCart();
    ftxui::Component buildLive();
    ftxui::Component buildOrders();
    ftxui::Component buildLoad();
    ftxui::Component buildAlgorithms();

    // Actions
    void advanceSimulation();
    void refreshProducts();
    void refreshCart();
    void refreshOrders();
    void addSelectedToCart();
    void checkout(Priority priority);
    void generateOrders();
    void changeSpeed(int delta);
    void setStatus(std::string message, bool ok);
    bool handleGlobalKey(const ftxui::Event& event);
    bool handleLiveKey(const ftxui::Event& event);

    // Rendering helpers
    ftxui::Element chrome(ftxui::Element body);
    ftxui::Element productRow(int productIndex, bool active, bool focused) const;
    ftxui::Element cartSummary() const;
    ftxui::Element orderDetail() const;

    // ---- Model -------------------------------------------------------------
    GridMap map_;
    AppOptions options_;
    Shop shop_;
    std::unique_ptr<Simulation> sim_;

    // ---- UI state ----------------------------------------------------------
    ftxui::App* screen_ = nullptr;
    int tab_ = kShop;
    std::vector<std::string> tabLabels_;
    ftxui::Component tabMenu_;
    int rootFocus_ = 1;        ///< 0 = tab bar, 1 = screen content (start in the content)
    std::string status_;
    bool statusOk_ = true;

    // Shop
    ftxui::Component searchInput_;
    int shopFocus_ = 1;        ///< 0 = filters, 1 = product list, 2 = buttons
    int filterFocus_ = 0;      ///< 0 = search box, 1 = categories
    std::string search_;
    int category_ = 0;
    std::vector<std::string> categoryLabels_;
    std::vector<int> filtered_;
    std::vector<std::string> productLabels_;
    int productSel_ = 0;
    int qty_ = 1;
    std::string lastSearch_;
    int lastCategory_ = -1;

    // Cart
    std::vector<int> cartIndex_;
    std::vector<std::string> cartLabels_;
    int cartSel_ = 0;
    bool confirmOpen_ = false;
    Priority pendingPriority_ = Priority::Standard;

    // Orders
    std::vector<OrderId> orderIds_;
    std::vector<std::string> orderLabels_;
    int orderSel_ = 0;

    // Live
    LiveViewState live_;
    int speedIndex_ = 3;
    double pendingTicks_ = 0.0;
    std::chrono::steady_clock::time_point lastTick_;
    std::string pauseLabel_ = "❚❚ PAUSE";
    std::string obstaclesLabel_;
    std::string pathsLabel_;

    // Load generator
    int loadCount_ = 20;
    int expressPct_ = 20;
    std::string lastLoad_;

    // Algorithms tab
    std::string routingLabel_;
    std::string assignmentLabel_;

    bool quitOpen_ = false;
};

} // namespace wh::ui
