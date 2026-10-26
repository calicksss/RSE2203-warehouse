#include "ui/App.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <random>
#include <thread>

#include "shop/Receipt.hpp"

namespace wh::ui {

using namespace ftxui;

namespace {

constexpr std::array<int, 7> kSpeeds{1, 2, 5, 10, 20, 50, 100};  // simulation ticks per second
const char* const kSectionNames[] = {"SHOP", "CART & CHECKOUT", "LIVE OPERATIONS", "ORDER HISTORY", "LOAD GENERATOR",
                                     "DATA STRUCTURES & ALGORITHMS"};

/// Bloomberg-style button: amber text on grey, black on amber when focused.
ButtonOption terminalButton(bool primary = false)
{
    ButtonOption option;
    option.transform = [primary](const EntryState& s) {
        Element e = text(" " + s.label + " ") | bold;
        if (s.focused) return e | color(theme::black()) | bgcolor(theme::amber());
        if (primary) return e | color(theme::black()) | bgcolor(theme::orange());
        return e | color(theme::amber()) | bgcolor(theme::headerBg());
    };
    return option;
}

InputOption terminalInput()
{
    InputOption option;
    option.multiline = false;
    option.transform = [](InputState s) {
        s.element |= color(s.is_placeholder ? theme::dim() : theme::white());
        if (s.focused) s.element |= bgcolor(theme::headerBg());
        return s.element;
    };
    return option;
}

Element labelValue(const std::string& label, Element value)
{
    return hbox({text(label) | color(theme::dim()) | size(WIDTH, EQUAL, 14), std::move(value)});
}

} // namespace

// ---------------------------------------------------------------------------
// Construction / main loop
// ---------------------------------------------------------------------------

App::App(const GridMap& map, const Catalogue& catalogue, const AppOptions& options)
    : map_(map),
      options_(options),
      shop_(catalogue, options_.receiptDirectory),
      sim_(std::make_unique<Simulation>(map_, shop_.catalogue(), options_.config)),
      tabLabels_{" F1 SHOP ", " F2 CART ", " F3 LIVE ", " F4 ORDERS ", " F5 LOAD ", " F6 ALGORITHMS "}
{
    const Catalogue& cat = shop_.catalogue();
    categoryLabels_.push_back("All products (" + std::to_string(cat.size()) + ")");
    for (const std::string& c : cat.categories()) {
        const long n = std::count_if(cat.all().begin(), cat.all().end(), [&](const Product& p) { return p.category == c; });
        categoryLabels_.push_back(c + " (" + std::to_string(n) + ")");
    }
    live_.ticksPerSecond = kSpeeds[static_cast<std::size_t>(speedIndex_)];
    refreshProducts();
}

int App::run()
{
    auto screen = ftxui::App::Fullscreen();
    screen_ = &screen;

    MenuOption tabOption = MenuOption::Horizontal();
    tabOption.entries_option.transform = [](const EntryState& s) {
        Element e = text(s.label) | bold;
        if (s.active) return e | color(theme::black()) | bgcolor(theme::amber());
        if (s.focused) return e | color(theme::black()) | bgcolor(theme::orange());
        return e | color(theme::amber()) | bgcolor(theme::headerBg());
    };
    tabOption.elements_infix = [] { return text(" "); };
    tabMenu_ = Menu(&tabLabels_, &tab_, tabOption);

    Component tabs = Container::Tab({buildShop(), buildCart(), buildLive(), buildOrders(), buildLoad(), buildAlgorithms()}, &tab_);
    Component root = Container::Vertical({tabMenu_, tabs}, &rootFocus_);
    Component ui = Renderer(root, [this, tabs] { return chrome(tabs->Render()); });

    // Quit confirmation (only shown when the cart still has items).
    Component quitButtons = Container::Horizontal({
        Button("QUIT ANYWAY", [this] { screen_->Exit(); }, terminalButton(true)),
        Button("KEEP SHOPPING", [this] { quitOpen_ = false; }, terminalButton()),
    });
    Component quitDialog = Renderer(quitButtons, [this, quitButtons] {
        return panel("EXIT", vbox({text("Your cart still has " + std::to_string(shop_.cart().units()) + " unit(s)."),
                                   text("Quit without checking out?"), text(""), quitButtons->Render() | hcenter})) |
               size(WIDTH, EQUAL, 46) | bgcolor(theme::black());
    });
    ui = ui | Modal(quitDialog, &quitOpen_);
    ui = CatchEvent(ui, [this](const ftxui::Event& e) { return handleGlobalKey(e); });

    // Simulation clock: post a step request to the UI thread every 50 ms.
    lastTick_ = std::chrono::steady_clock::now();
    std::atomic<bool> running{true};
    std::thread clock([&] {
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            screen.Post([this] { advanceSimulation(); });
            screen.PostEvent(ftxui::Event::Custom);
        }
    });

    screen.Loop(ui);

    running = false;
    clock.join();
    screen_ = nullptr;
    return 0;
}

bool App::handleGlobalKey(const ftxui::Event& e)
{
    if (quitOpen_ || confirmOpen_) return false;  // let the dialog handle keys
    static const std::array<ftxui::Event, 6> kTabKeys{ftxui::Event::F1, ftxui::Event::F2, ftxui::Event::F3, ftxui::Event::F4, ftxui::Event::F5, ftxui::Event::F6};
    for (std::size_t i = 0; i < kTabKeys.size(); ++i) {
        if (e == kTabKeys[i]) {
            tab_ = static_cast<int>(i);
            return true;
        }
    }
    if (e == ftxui::Event::F10) {
        if (shop_.cart().empty()) screen_->Exit();
        else quitOpen_ = true;
        return true;
    }
    if (tab_ == kShop) {
        const bool typing = searchInput_ && searchInput_->Focused();
        if (!typing && e == ftxui::Event::Character('/')) {  // jump to the search box
            rootFocus_ = 1;
            shopFocus_ = 0;
            filterFocus_ = 0;
            return true;
        }
        if (typing && (e == ftxui::Event::Escape || e == ftxui::Event::Return)) {  // back to the product list
            shopFocus_ = 1;
            return true;
        }
    }
    if (tab_ == kLive) return handleLiveKey(e);
    return false;
}

void App::setStatus(std::string message, bool ok)
{
    status_ = std::move(message);
    statusOk_ = ok;
}

// ---------------------------------------------------------------------------
// Simulation clock and live controls
// ---------------------------------------------------------------------------

void App::advanceSimulation()
{
    const auto now = std::chrono::steady_clock::now();
    const double elapsed = std::chrono::duration<double>(now - lastTick_).count();
    lastTick_ = now;
    if (!live_.paused) pendingTicks_ += elapsed * kSpeeds[static_cast<std::size_t>(speedIndex_)];
    int steps = std::min(static_cast<int>(pendingTicks_), 500);  // never stall the UI catching up
    pendingTicks_ -= steps;
    for (; steps > 0 && !sim_->isQuiescent(); --steps) sim_->step();
    if (sim_->isQuiescent()) pendingTicks_ = 0.0;
}

void App::changeSpeed(int delta)
{
    speedIndex_ = std::clamp(speedIndex_ + delta, 0, static_cast<int>(kSpeeds.size()) - 1);
    live_.ticksPerSecond = kSpeeds[static_cast<std::size_t>(speedIndex_)];
    live_.flash = "Speed " + std::to_string(live_.ticksPerSecond) + " ticks/s";
}

bool App::handleLiveKey(const ftxui::Event& e)
{
    if (e == ftxui::Event::Character(' ')) {
        live_.paused = !live_.paused;
        live_.flash = live_.paused ? "Simulation paused" : "Simulation resumed";
    } else if (e == ftxui::Event::Character('+') || e == ftxui::Event::Character('=')) {
        changeSpeed(+1);
    } else if (e == ftxui::Event::Character('-') || e == ftxui::Event::Character('_')) {
        changeSpeed(-1);
    } else if (e == ftxui::Event::Character('s') || e == ftxui::Event::Character('S')) {
        if (live_.paused) { sim_->step(); live_.flash = "Stepped one tick"; }
        else live_.flash = "Pause first (SPACE) to step one tick at a time";
    } else if (e == ftxui::Event::Character('o') || e == ftxui::Event::Character('O')) {
        live_.flash = sim_->spawnObstacle() ? "Obstacle dropped on the floor" : "No free floor cell for an obstacle";
    } else if (e == ftxui::Event::Character('r') || e == ftxui::Event::Character('R')) {
        sim_->setRandomObstacles(!sim_->config().randomObstacles);
        live_.flash = std::string("Random obstacles ") + (sim_->config().randomObstacles ? "ON" : "OFF");
    } else if (e == ftxui::Event::Character('p') || e == ftxui::Event::Character('P')) {
        live_.showPaths = !live_.showPaths;
        live_.flash = std::string("Route preview ") + (live_.showPaths ? "ON" : "OFF");
    } else {
        return false;
    }
    return true;
}

Component App::buildLive()
{
    Component controls = Container::Horizontal({
        Button(&pauseLabel_, [this] { handleLiveKey(ftxui::Event::Character(' ')); }, terminalButton()),
        Button("SLOWER", [this] { changeSpeed(-1); }, terminalButton()),
        Button("FASTER", [this] { changeSpeed(+1); }, terminalButton()),
        Button("STEP", [this] { handleLiveKey(ftxui::Event::Character('s')); }, terminalButton()),
        Button("DROP OBSTACLE", [this] { handleLiveKey(ftxui::Event::Character('o')); }, terminalButton()),
        Button(&obstaclesLabel_, [this] { handleLiveKey(ftxui::Event::Character('r')); }, terminalButton()),
        Button(&pathsLabel_, [this] { handleLiveKey(ftxui::Event::Character('p')); }, terminalButton()),
    });
    return Renderer(controls, [this, controls] {
        pauseLabel_ = live_.paused ? "▶ RESUME" : "❚❚ PAUSE";
        obstaclesLabel_ = std::string("RANDOM OBSTACLES: ") + (sim_->config().randomObstacles ? "ON" : "OFF");
        pathsLabel_ = std::string("ROUTES: ") + (live_.showPaths ? "ON" : "OFF");
        Element bar = hbox({controls->Render(), filler(),
                            text("SPEED " + std::to_string(live_.ticksPerSecond) + " t/s ") | bold | color(theme::amber())});
        return vbox({bar, liveDashboard(*sim_, live_, Terminal::Size().dimx) | flex});
    });
}

// ---------------------------------------------------------------------------
// SHOP
// ---------------------------------------------------------------------------

void App::refreshProducts()
{
    const Catalogue& cat = shop_.catalogue();
    filtered_.clear();
    productLabels_.clear();
    for (int i : cat.search(search_)) {   // linear scan: matches part of a name or category
        const Product& p = cat.at(i);
        if (category_ > 0 && p.category != cat.categories()[static_cast<std::size_t>(category_ - 1)]) continue;
        filtered_.push_back(i);
        productLabels_.push_back(p.name);
    }
    productSel_ = std::clamp(productSel_, 0, std::max(0, static_cast<int>(filtered_.size()) - 1));
}

Element App::productRow(int idx, bool active, bool focused) const
{
    const Product& p = shop_.catalogue().at(idx);
    const int inCart = shop_.cart().quantityOf(idx);
    const int available = p.stock - inCart;
    Element row = hbox({
        text(focused ? "▶" : " ") | color(theme::amber()),
        cell(std::to_string(idx + 1), 4, true) | bold | color(theme::amber()),
        text(" "),
        cell(p.name, 20) | bold | color(theme::white()),
        cell(p.category, 16) | color(theme::dim()),
        cell(p.price.str(), 8, true) | color(theme::green()),
        cell(fixed(p.weightKg, 2), 6, true),
        cell(available > 0 ? std::to_string(available) : "OUT", 7, true) | color(available > 5 ? theme::text() : theme::red()),
        cell(inCart ? std::to_string(inCart) : "", 8, true) | bold | color(theme::yellow()),
        cell(toString(p.shelf), 9, true) | color(Color::RGB(110, 110, 110)),
        filler(),
    });
    if (active) row = row | bgcolor(Color::RGB(0, 55, 95));
    return row;
}

void App::addSelectedToCart()
{
    if (filtered_.empty()) return;
    const int idx = filtered_[static_cast<std::size_t>(productSel_)];
    const auto result = shop_.cart().add(shop_.catalogue(), idx, qty_);
    setStatus(result.message, result.ok);
    if (result.ok) qty_ = 1;
}

Element App::cartSummary() const
{
    const Cart& cart = shop_.cart();
    const Catalogue& cat = shop_.catalogue();
    Elements lines;
    for (const auto& line : cart.lines())
        lines.push_back(hbox({text(padLeft(std::to_string(line.second), 3) + " x " + cat.at(line.first).name), filler(),
                              text((cat.at(line.first).price * line.second).str()) | color(theme::white())}));
    if (lines.empty()) lines.push_back(text("Cart is empty.") | color(theme::dim()));
    const double w = cart.weightKg(cat);
    return panel("CART", vbox({
                             vbox(std::move(lines)) | yframe | flex,
                             separatorLight() | color(theme::faint()),
                             hbox({text("Weight ") | color(theme::dim()),
                                   meter(w / cart.weightLimitKg(), w > cart.weightLimitKg() * 0.85 ? theme::red() : theme::green(), 14),
                                   filler(), text(fixed(w, 1) + "/" + fixed(cart.weightLimitKg(), 0) + "kg")}),
                             hbox({text("Total") | bold | color(theme::amber()), filler(), text(cart.total(cat).str()) | bold | color(theme::white())}),
                         }),
                 std::to_string(cart.units()) + " UNITS");
}

Component App::buildShop()
{
    InputOption searchOption = terminalInput();
    searchOption.on_change = [this] { productSel_ = 0; };
    Component search = Input(&search_, "press / to search…", searchOption);
    searchInput_ = search;

    MenuOption categoryOption = MenuOption::Vertical();
    categoryOption.entries = &categoryLabels_;
    categoryOption.selected = &category_;
    categoryOption.on_change = [this] { productSel_ = 0; };
    categoryOption.entries_option.transform = [](const EntryState& s) {
        Element e = text((s.focused ? "▶ " : "  ") + s.label);
        if (s.active) return e | bold | color(theme::black()) | bgcolor(theme::amber());
        return e | color(s.focused ? theme::white() : theme::text());
    };
    Component categories = Menu(categoryOption);

    MenuOption productOption = MenuOption::Vertical();
    productOption.entries = &productLabels_;
    productOption.selected = &productSel_;
    productOption.on_enter = [this] { addSelectedToCart(); };
    productOption.entries_option.transform = [this](const EntryState& s) {
        if (s.index < 0 || static_cast<std::size_t>(s.index) >= filtered_.size()) return text("");
        return productRow(filtered_[static_cast<std::size_t>(s.index)], s.active, s.focused);
    };
    Component products = Menu(productOption);

    Component minus = Button("−", [this] { qty_ = std::max(1, qty_ - 1); }, terminalButton());
    Component plus = Button("+", [this] { qty_ = std::min(99, qty_ + 1); }, terminalButton());
    Component add = Button("ADD TO CART", [this] { addSelectedToCart(); }, terminalButton(true));
    Component actions = Container::Vertical({Container::Horizontal({minus, plus}), add});

    Component layout = Container::Horizontal({Container::Vertical({search, categories}, &filterFocus_), products, actions}, &shopFocus_);

    return Renderer(layout, [=] {
        refreshProducts();
        const Catalogue& cat = shop_.catalogue();

        Element filters = vbox({
            panel("SEARCH", search->Render()),
            panel("CATEGORIES", categories->Render()) | flex,
        }) | size(WIDTH, EQUAL, 30);

        Element table = panel(
            "CATALOGUE · " + (category_ == 0 ? std::string("ALL") : cat.categories()[static_cast<std::size_t>(category_ - 1)]),
            vbox({
                headerRow({text(" "), cell("ID", 4, true), text(" "), cell("PRODUCT", 20), cell("CATEGORY", 16), cell("PRICE", 8, true),
                           cell("KG", 6, true), cell("STOCK", 7, true), cell("IN CART", 8, true), cell("SHELF", 9, true), filler()}),
                filtered_.empty() ? text("No products match. Clear the search box.") | color(theme::dim())
                                  : products->Render() | vscroll_indicator | yframe | flex,
            }),
            std::to_string(filtered_.size()) + " ITEMS");

        Element detail = text("Select a product.") | color(theme::dim());
        if (!filtered_.empty()) {
            const int idx = filtered_[static_cast<std::size_t>(productSel_)];
            const Product& p = cat.at(idx);
            const int available = p.stock - shop_.cart().quantityOf(idx);
            detail = vbox({
                text(p.name) | bold | color(theme::white()),
                text(p.category + "  ·  product #" + std::to_string(idx + 1)) | color(theme::dim()),
                text(""),
                labelValue("Price", text(p.price.str()) | color(theme::green())),
                labelValue("Weight", text(fixed(p.weightKg, 2) + " kg")),
                labelValue("Available", text(std::to_string(available)) | color(available > 5 ? theme::white() : theme::red())),
                labelValue("Shelf", text(toString(p.shelf) + " on the floor map")),
                labelValue("Lookup", text(cat.findByName(p.name) == idx ? "binary search OK" : "binary search MISMATCH") |
                                         color(cat.findByName(p.name) == idx ? theme::dim() : theme::red())),
                text(""),
                hbox({text("Quantity  ") | color(theme::dim()), minus->Render(), text(" " + padLeft(std::to_string(qty_), 2) + " ") | bold,
                      plus->Render(), filler(), text("= " + (p.price * qty_).str()) | color(theme::green())}),
                text(""),
                add->Render() | hcenter,
            });
        }
        Element side = vbox({panel("SELECTED", detail), cartSummary() | flex}) | size(WIDTH, EQUAL, 40);
        return hbox({filters, table | flex, side});
    });
}

// ---------------------------------------------------------------------------
// CART
// ---------------------------------------------------------------------------

void App::refreshCart()
{
    cartIndex_.clear();
    cartLabels_.clear();
    for (const auto& line : shop_.cart().lines()) {
        cartIndex_.push_back(line.first);
        cartLabels_.push_back(shop_.catalogue().at(line.first).name);
    }
    cartSel_ = std::clamp(cartSel_, 0, std::max(0, static_cast<int>(cartIndex_.size()) - 1));
}

void App::checkout(Priority priority)
{
    const auto result = shop_.checkout(priority);
    confirmOpen_ = false;
    if (result.ok) {
        sim_->submit(result.order);
        setStatus(result.message + " Watch it being picked on F3 LIVE.", true);
    } else {
        setStatus(result.message, false);
    }
}

Component App::buildCart()
{
    MenuOption lineOption = MenuOption::Vertical();
    lineOption.entries = &cartLabels_;
    lineOption.selected = &cartSel_;
    lineOption.entries_option.transform = [this](const EntryState& s) {
        if (s.index < 0 || static_cast<std::size_t>(s.index) >= cartIndex_.size()) return text("");
        const int idx = cartIndex_[static_cast<std::size_t>(s.index)];
        const Product& p = shop_.catalogue().at(idx);
        const int qty = shop_.cart().quantityOf(idx);
        Element row = hbox({
            text(s.focused ? "▶" : " ") | color(theme::amber()),
            cell(std::to_string(idx + 1), 4, true) | bold | color(theme::amber()),
            text(" "),
            cell(p.name, 22) | bold | color(theme::white()),
            cell(std::to_string(qty), 5, true) | bold | color(theme::yellow()),
            cell(p.price.str(), 9, true),
            cell((p.price * qty).str(), 10, true) | color(theme::green()),
            cell(fixed(p.weightKg * qty, 2), 7, true),
            text("  "),
            text(p.category) | color(theme::dim()),
            filler(),
        });
        return s.active ? row | bgcolor(Color::RGB(0, 55, 95)) : row;
    };
    Component lines = Menu(lineOption);

    // Product index of the highlighted cart line, or -1 if the cart is empty.
    auto selected = [this]() { return cartIndex_.empty() ? -1 : cartIndex_[static_cast<std::size_t>(cartSel_)]; };
    auto changeLine = [this, selected](int delta) {
        const int i = selected();
        if (i < 0) return;
        const Cart::Result r = delta > 0 ? shop_.cart().add(shop_.catalogue(), i, delta)
                                         : shop_.cart().remove(shop_.catalogue(), i, -delta);
        setStatus(r.message, r.ok);
    };
    Component less = Button("− 1", [=] { changeLine(-1); }, terminalButton());
    Component more = Button("+ 1", [=] { changeLine(+1); }, terminalButton());
    Component removeLine = Button("REMOVE LINE", [=] {
        const int i = selected();
        if (i >= 0) changeLine(-shop_.cart().quantityOf(i));
    }, terminalButton());
    Component clear = Button("CLEAR CART", [this] { shop_.cart().clear(); setStatus("Cart emptied.", true); }, terminalButton());
    auto openCheckout = [this](Priority p) {
        if (shop_.cart().empty()) { setStatus("Your cart is empty. Add products on F1 SHOP.", false); return; }
        pendingPriority_ = p;
        confirmOpen_ = true;
    };
    Component standard = Button("CHECKOUT · STANDARD", [=] { openCheckout(Priority::Standard); }, terminalButton(true));
    Component express = Button("CHECKOUT · EXPRESS", [=] { openCheckout(Priority::Express); }, terminalButton(true));

    Component buttons = Container::Vertical({Container::Horizontal({less, more, removeLine, clear}), standard, express});
    Component layout = Container::Horizontal({lines, buttons});

    Component page = Renderer(layout, [=] {
        refreshCart();
        const Cart& cart = shop_.cart();
        const Catalogue& cat = shop_.catalogue();
        Element table = panel("SHOPPING CART",
                              vbox({
                                  headerRow({text(" "), cell("ID", 4, true), text(" "), cell("PRODUCT", 22), cell("QTY", 5, true), cell("UNIT", 9, true),
                                             cell("SUBTOTAL", 10, true), cell("KG", 7, true), text("  CATEGORY"), filler()}),
                                  cartIndex_.empty() ? vbox({text(""), text("  Your cart is empty. Press F1 to shop.") | color(theme::dim())})
                                                     : lines->Render() | vscroll_indicator | yframe | flex,
                              }),
                              std::to_string(cartIndex_.size()) + " LINES");
        const double w = cart.weightKg(cat);
        Element side = panel(
            "CHECKOUT",
            vbox({
                labelValue("Units", text(std::to_string(cart.units())) | bold | color(theme::white())),
                labelValue("Weight", text(fixed(w, 2) + " / " + fixed(cart.weightLimitKg(), 2) + " kg")),
                meter(w / cart.weightLimitKg(), w > cart.weightLimitKg() * 0.85 ? theme::red() : theme::green(), 40),
                labelValue("Grand total", text(cart.total(cat).str()) | bold | color(theme::green())),
                text(""),
                text("Selected line") | bold | color(theme::amber()),
                hbox({less->Render(), text(" "), more->Render(), text(" "), removeLine->Render()}),
                text(""),
                clear->Render(),
                text(""),
                text("Service level") | bold | color(theme::amber()),
                standard->Render(),
                text("  First come, first served.") | color(theme::dim()),
                express->Render(),
                text("  Jumps the queue by " + std::to_string(sim_->config().expressHeadStart) + " ticks.") | color(theme::orange()),
                text(""),
                paragraph("Stock is reserved only at checkout. A receipt is written to ./receipts/.") | color(theme::dim()),
            }) | size(WIDTH, EQUAL, 44));
        return hbox({table | flex, side});
    });

    Component confirmButtons = Container::Horizontal({
        Button("CONFIRM ORDER", [this] { checkout(pendingPriority_); }, terminalButton(true)),
        Button("CANCEL", [this] { confirmOpen_ = false; setStatus("Checkout cancelled.", false); }, terminalButton()),
    });
    Component confirm = Renderer(confirmButtons, [this, confirmButtons] {
        const Cart& cart = shop_.cart();
        return panel("CONFIRM CHECKOUT",
                     vbox({
                         labelValue("Service", text(toString(pendingPriority_)) | bold |
                                                   color(pendingPriority_ == Priority::Express ? theme::orange() : theme::white())),
                         labelValue("Units", text(std::to_string(cart.units()))),
                         labelValue("Weight", text(fixed(cart.weightKg(shop_.catalogue()), 2) + " kg")),
                         labelValue("Total", text(cart.total(shop_.catalogue()).str()) | bold | color(theme::green())),
                         text(""),
                         confirmButtons->Render() | hcenter,
                     })) |
               size(WIDTH, EQUAL, 46) | bgcolor(theme::black());
    });
    return page | Modal(confirm, &confirmOpen_);
}

// ---------------------------------------------------------------------------
// ORDERS
// ---------------------------------------------------------------------------

void App::refreshOrders()
{
    const auto& history = shop_.history();
    if (orderIds_.size() != history.size()) {
        orderIds_.clear();
        orderLabels_.clear();
        for (auto it = history.rbegin(); it != history.rend(); ++it) {  // newest first
            orderIds_.push_back(it->id);
            orderLabels_.push_back(Receipt::reference(*it));
        }
    }
    orderSel_ = std::clamp(orderSel_, 0, std::max(0, static_cast<int>(orderIds_.size()) - 1));
}

Element App::orderDetail() const
{
    if (orderIds_.empty()) return text("No orders yet.") | color(theme::dim());
    const OrderId id = orderIds_[static_cast<std::size_t>(orderSel_)];
    const Order* order = nullptr;
    for (const Order& o : shop_.history())
        if (o.id == id) order = &o;
    if (!order) return text("");
    const OrderRecord* p = sim_->jobs().record(id);
    const OrderStatus s = p ? p->status : OrderStatus::Queued;

    Elements rows{
        labelValue("Status", text(toString(s)) | bold | color(theme::status(s))),
        labelValue("Placed", text(order->placedAt)),
        labelValue("Service", text(toString(order->priority)) | color(order->priority == Priority::Express ? theme::orange() : theme::text())),
        labelValue("Source", text(order->generated ? "Synthetic load test" : "Web shop")),
    };
    if (p && p->sorterIndex >= 0) rows.push_back(labelValue("Sorter", text("SRT-" + std::to_string(p->sorterIndex + 1)) | color(theme::magenta())));
    if (p) {
        rows.push_back(labelValue("Progress", text(std::to_string(p->unitsPicked) + " picked · " + std::to_string(p->unitsSorted) + " sorted of " +
                                                   std::to_string(p->unitsTotal))));
        const double frac = p->unitsTotal ? (p->unitsPicked + p->unitsSorted) / (2.0 * p->unitsTotal) : 0.0;
        rows.push_back(meter(s == OrderStatus::Completed ? 1.0 : frac, theme::status(s), 40));
    }
    if (p && p->completed >= 0)
        rows.push_back(labelValue("Timing", text("lead " + std::to_string(p->completed - p->submitted) + "t · in warehouse " +
                                                 std::to_string(p->completed - p->released) + "t") | color(theme::green())));
    rows.push_back(text(""));
    rows.push_back(text("LINES") | bold | color(theme::amber()));
    for (const OrderLine& l : order->lines)
        rows.push_back(hbox({text(padLeft(std::to_string(l.qty), 3) + " x " + l.name), filler(), text((l.unitPrice * l.qty).str()) | color(theme::green())}));
    rows.push_back(hbox({text("Total") | bold, filler(), text(order->total().str()) | bold | color(theme::white())}));
    if (p && !p->packingList.empty()) {
        std::string list;
        for (const auto& name : p->packingList) list += (list.empty() ? "" : " › ") + name;
        rows.push_back(text(""));
        rows.push_back(text("PACKING ORDER (heaviest first)") | bold | color(theme::amber()));
        rows.push_back(paragraph(list));
    }
    return vbox(std::move(rows));
}

Component App::buildOrders()
{
    MenuOption option = MenuOption::Vertical();
    option.entries = &orderLabels_;
    option.selected = &orderSel_;
    option.entries_option.transform = [this](const EntryState& s) {
        if (s.index < 0 || static_cast<std::size_t>(s.index) >= orderIds_.size()) return text("");
        const OrderId id = orderIds_[static_cast<std::size_t>(s.index)];
        const Order* o = nullptr;
        for (const Order& h : shop_.history())
            if (h.id == id) o = &h;
        if (!o) return text("");
        const OrderRecord* p = sim_->jobs().record(id);
        const OrderStatus st = p ? p->status : OrderStatus::Queued;
        const bool express = o->priority == Priority::Express;
        Element row = hbox({
            text(s.focused ? "▶" : " ") | color(theme::amber()),
            cell(Receipt::reference(*o), 11) | bold | color(theme::amber()),
            cell(o->placedAt.size() >= 19 ? o->placedAt.substr(11, 8) : o->placedAt, 9) | color(theme::dim()),
            cell(o->generated ? "AUTO" : "WEB", 5) | color(o->generated ? theme::dim() : theme::cyan()),
            cell(express ? "EXP" : "STD", 4) | color(express ? theme::orange() : theme::dim()),
            cell(std::to_string(o->units()), 6, true),
            cell(o->total().str(), 10, true) | color(theme::green()),
            text("  "),
            cell(toString(st), 9) | bold | color(theme::status(st)),
            cell(p && p->completed >= 0 ? std::to_string(p->completed - p->submitted) + "t" : "-", 7, true),
            filler(),
        });
        return s.active ? row | bgcolor(Color::RGB(0, 55, 95)) : row;
    };
    Component list = Menu(option);
    return Renderer(list, [this, list] {
        refreshOrders();
        Element table = panel("ORDER HISTORY",
                              vbox({
                                  headerRow({text(" "), cell("REF", 11), cell("PLACED", 9), cell("SRC", 5), cell("PRI", 4), cell("UNITS", 6, true),
                                             cell("TOTAL", 10, true), text("  "), cell("STATUS", 9), cell("LEAD", 7, true), filler()}),
                                  orderIds_.empty() ? text("  No orders yet. Check out on F2 or generate orders on F5.") | color(theme::dim())
                                                    : list->Render() | vscroll_indicator | yframe | flex,
                              }),
                              std::to_string(orderIds_.size()) + " ORDERS");
        return hbox({table | flex, panel("ORDER DETAIL", orderDetail() | yframe) | size(WIDTH, EQUAL, 58)});
    });
}

// ---------------------------------------------------------------------------
// LOAD generator
// ---------------------------------------------------------------------------

void App::generateOrders()
{
    static std::mt19937 rng(options_.config.seed + 17u);
    const auto created = shop_.generateOrders(loadCount_, rng, expressPct_ / 100.0);
    int units = 0, express = 0;
    for (const Order& o : created) {
        sim_->submit(o);
        units += o.units();
        express += o.priority == Priority::Express ? 1 : 0;
    }
    if (created.empty()) {
        setStatus("The catalogue is out of stock: no orders generated.", false);
        return;
    }
    lastLoad_ = "Generated " + std::to_string(created.size()) + " orders (" + std::to_string(units) + " units, " +
                std::to_string(express) + " express).";
    setStatus(lastLoad_ + " Press F3 to watch the fleet.", true);
}

Component App::buildLoad()
{
    SliderOption<int> countOption;
    countOption.value = &loadCount_;
    countOption.min = 1;
    countOption.max = 200;
    countOption.increment = 1;
    countOption.color_active = theme::amber();
    countOption.color_inactive = theme::border();
    SliderOption<int> expressOption;
    expressOption.value = &expressPct_;
    expressOption.min = 0;
    expressOption.max = 100;
    expressOption.increment = 5;
    expressOption.color_active = theme::orange();
    expressOption.color_inactive = theme::border();
    Component count = Slider(countOption);
    Component express = Slider(expressOption);
    Component go = Button("GENERATE ORDERS", [this] { generateOrders(); }, terminalButton(true));
    Component layout = Container::Vertical({count, express, go});

    return Renderer(layout, [=] {
        long stock = 0;
        for (const Product& p : shop_.catalogue().all()) stock += p.stock;
        Element form = panel(
            "LOAD GENERATOR",
            vbox({
                paragraph("Creates synthetic customer orders (1-4 random products, 1-3 units each) and queues them for the "
                          "warehouse. Stock is consumed exactly as for real orders; no receipts are written. Use it to watch the "
                          "job manager spread work across the whole fleet."),
                text(""),
                hbox({text("Orders       ") | color(theme::dim()), count->Render() | flex, text(" " + padLeft(std::to_string(loadCount_), 4)) | bold}),
                hbox({text("Express %    ") | color(theme::dim()), express->Render() | flex,
                      text(" " + padLeft(std::to_string(expressPct_), 3) + "%") | bold | color(theme::orange())}),
                text(""),
                go->Render() | hcenter,
                text(""),
                labelValue("Units in stock", text(withCommas(stock))),
                labelValue("Open orders", text(std::to_string(sim_->jobs().openOrders())) | color(theme::yellow())),
                lastLoad_.empty() ? text("") : text(lastLoad_) | bold | color(theme::green()),
            }));
        return hbox({form | size(WIDTH, EQUAL, 92), filler()}) | vcenter;
    });
}

// ---------------------------------------------------------------------------
// ALGORITHMS: where each data structure / algorithm from the course runs, with live numbers
// ---------------------------------------------------------------------------

Component App::buildAlgorithms()
{
    Component routing = Button(&routingLabel_, [this] {
        const RoutingMethod next = sim_->config().routing == RoutingMethod::Backtracking ? RoutingMethod::Greedy
                                 : sim_->config().routing == RoutingMethod::Greedy        ? RoutingMethod::AsGiven
                                                                                         : RoutingMethod::Backtracking;
        sim_->setRouting(next);
        setStatus("Route planning switched to " + toString(next) + " (applies to new trips).", true);
    }, terminalButton(true));
    Component assignment = Button(&assignmentLabel_, [this] {
        const AssignmentMethod next = sim_->config().assignment == AssignmentMethod::Nearest ? AssignmentMethod::FirstAvailable
                                                                                            : AssignmentMethod::Nearest;
        sim_->setAssignment(next);
        setStatus("Robot choice switched to " + toString(next) + " (applies to new trips).", true);
    }, terminalButton(true));
    Component buttons = Container::Horizontal({routing, assignment});

    return Renderer(buttons, [this, buttons] {
        routingLabel_ = "ROUTE PLANNING: " + toString(sim_->config().routing);
        assignmentLabel_ = "ROBOT CHOICE: " + toString(sim_->config().assignment);
        const Metrics& m = sim_->metrics();
        int queued = 0;
        for (const SortingRobot* s : sim_->sorters()) queued += s->queuedUnits();
        const std::string perPlan = m.astarCalls ? fixed(double(m.astarExpanded) / double(m.astarCalls), 1) : "-";

        struct Row {
            const char* where;
            const char* what;
            const char* lecture;
            const char* complexity;
            std::string live;
        };
        const std::vector<Row> rows{
            {"Shop search box", "Linear search over names/categories", "L3", "O(n·m)", std::to_string(shop_.catalogue().size()) + " products"},
            {"Product by name", "Binary search on a merge-sorted name index", "L4, L5a", "O(log n)", "see F1 detail panel"},
            {"Order backlog", "Binary min-heap (priority queue)", "L4 heap", "O(log n) push/pop", std::to_string(sim_->jobs().backlogSize()) + " waiting"},
            {"Sorter choice", "Greedy: least-loaded sorter with a free bin", "L5b", "O(sorters)", std::to_string(sim_->sorters().size()) + " sorters"},
            {"Trip filling", "Greedy first-fit by urgency (8 items / 12 kg)", "L5b", "O(pool)", std::to_string(m.tripsDispatched) + " trips"},
            {"Robot choice", "Greedy nearest idle robot (or first idle)", "L5b", "O(robots·stops)", toString(sim_->config().assignment)},
            {"Shelf order", "Backtracking + pruning (≤8 shelves) / greedy NN", "L5a, L5b", "O(k!) / O(k²)", toString(sim_->config().routing)},
            {"Shelf distances", "BFS from each cell, cached (memoisation)", "Wk10-11, L5a", "O(V+E) per new cell", "linked-list queue"},
            {"Robot driving", "A* with Manhattan heuristic on a min-heap", "Wk10-11", "O(E log V)", std::to_string(m.astarCalls) + " plans, " + perPlan + " cells/plan"},
            {"Traffic order", "Insertion sort of waiting robots", "L4", "O(n) if nearly sorted", std::to_string(m.waits) + " waits"},
            {"Collisions", "Move only into empty cells; detour; sidestep", "-", "O(robots) per tick", std::to_string(m.collisions) + " collisions"},
            {"Sorter conveyor", "Singly linked-list FIFO queue", "L2", "O(1) enqueue/dequeue", std::to_string(queued) + " items queued"},
            {"Packing list", "Merge sort, heaviest first (stable)", "L4", "O(n log n)", std::to_string(m.ordersCompleted) + " orders packed"},
        };
        Elements table{headerRow({cell("WHERE", 17), cell("DATA STRUCTURE / ALGORITHM", 50), cell("LECTURE", 15), cell("COST", 26),
                                  text("LIVE") | flex})};
        int k = 0;
        for (const Row& r : rows) {
            table.push_back(hbox({cell(r.where, 17) | bold | color(theme::amber()), cell(r.what, 50) | color(theme::white()),
                                  cell(r.lecture, 15) | color(theme::cyan()), cell(r.complexity, 26), text(r.live) | color(theme::green()) | flex}) |
                            bgcolor(k++ % 2 ? theme::rowAlt() : theme::black()));
        }
        return vbox({
            panel("WHERE THE COURSE TOPICS RUN", vbox(std::move(table)), "LIVE COUNTERS"),
            panel("TRY IT", vbox({
                                buttons->Render(),
                                text(""),
                                paragraph("Switch the algorithm, generate orders on F5, then watch F3. Compare the LEAD time and "
                                          "DIST (distance driven) in the ticker. For proper measurements use the experiments "
                                          "program (see experiments/README.md).") | color(theme::dim()),
                            })),
        });
    });
}

// ---------------------------------------------------------------------------
// Chrome shared by every screen
// ---------------------------------------------------------------------------

Element App::chrome(Element body)
{
    const std::string right = "TICK " + padLeft(std::to_string(sim_->now()), 6, '0') + "  │  " + std::to_string(live_.ticksPerSecond) +
                              " t/s  │  " + simStateLabel(*sim_, live_.paused) + "  │  " + wallClock();
    Element title = titleBar("WHSE <GO>  │  AUTOMATED GROCERY FULFILMENT CENTRE  │  " + std::string(kSectionNames[tab_]), right);

    std::vector<TickerItem> items;
    if (tab_ == kLive) {
        items = simTickerItems(*sim_);
    } else {
        const Cart& cart = shop_.cart();
        int open = 0, done = 0, busy = 0;
        for (const Order& o : shop_.history()) {
            const OrderRecord* p = sim_->jobs().record(o.id);
            (p && p->status == OrderStatus::Completed ? done : open)++;
        }
        for (const PickingRobot* p : sim_->pickers()) busy += p->hasWork() ? 1 : 0;
        items = {
            {"CART", std::to_string(cart.units()) + " units", theme::white()},
            {"WEIGHT", fixed(cart.weightKg(shop_.catalogue()), 2) + "/" + fixed(cart.weightLimitKg(), 0) + "kg", theme::white()},
            {"TOTAL", cart.total(shop_.catalogue()).str(), theme::green()},
            {"ORDERS OPEN", std::to_string(open), open ? theme::yellow() : theme::white()},
            {"DONE", std::to_string(done), theme::green()},
            {"FLEET", std::to_string(busy) + "/" + std::to_string(sim_->pickers().size()) + " busy", theme::white()},
            {"SKUs", std::to_string(shop_.catalogue().size()), theme::white()},
        };
    }

    Element statusLine = status_.empty() ? text(" ")
                                         : text((statusOk_ ? " ✔ " : " ✖ ") + status_) | bold | color(statusOk_ ? theme::green() : theme::red());

    std::vector<std::pair<std::string, std::string>> hints;
    switch (tab_) {
        case kShop:   hints = {{"↑↓", "Select"}, {"Enter", "Add qty"}, {"/", "Search"}, {"Esc", "Back to list"}, {"←→", "Panels"}, {"F2", "Cart"}, {"F10", "Exit"}}; break;
        case kCart:   hints = {{"↑↓", "Select line"}, {"Tab", "Buttons"}, {"Enter", "Press"}, {"F1", "Shop"}, {"F3", "Live"}, {"F10", "Exit"}}; break;
        case kLive:   hints = {{"SPACE", "Pause"}, {"+/-", "Speed"}, {"S", "Step"}, {"O", "Obstacle"}, {"R", "Random obstacles"}, {"P", "Routes"}, {"F10", "Exit"}}; break;
        case kOrders: hints = {{"↑↓", "Select order"}, {"F3", "Live"}, {"F10", "Exit"}}; break;
        case kLoad:   hints = {{"←→", "Adjust"}, {"Tab", "Next"}, {"Enter", "Generate"}, {"F3", "Live"}, {"F10", "Exit"}}; break;
        default:      hints = {{"F3", "Live"}, {"F10", "Exit"}}; break;
    }
    hints.emplace_back("Mouse", "click anything");

    return vbox({
               title,
               hbox({tabMenu_->Render(), filler(), text(" F10 EXIT ") | bold | color(theme::amber()) | bgcolor(theme::headerBg())}) |
                   bgcolor(theme::tickerBg()),
               tickerBar(items),
               std::move(body) | flex,
               statusLine,
               keyHints(hints),
           }) |
           color(theme::text()) | bgcolor(theme::black());
}

} // namespace wh::ui
