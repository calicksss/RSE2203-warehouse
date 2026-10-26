/**
 * @file test_shop.cpp
 * @brief Tests for the shop module: Money, Catalogue (incl. binary search), Cart, checkout, receipts.
 */
#include <filesystem>
#include <random>
#include <sstream>
#include <string>

#include "TestFramework.hpp"
#include "core/Errors.hpp"
#include "shop/Catalogue.hpp"
#include "shop/Receipt.hpp"
#include "shop/Shop.hpp"

using namespace wh;

namespace {

Catalogue smallCatalogue()
{
    std::istringstream in(
        "# Name Price Weight Stock Row Col Category\n"
        "Apple   1.50 0.20 10 1 1 Produce\n"
        "Milk    3.45 1.00  2 1 2 Dairy\n"
        "Rice   12.00 2.00  5 1 3 Dry_Goods\n"
        "Bread   2.10 0.50  4 1 4\n");
    return Catalogue::parse(in, "test");
}

Money money(const std::string& text)
{
    Money m;
    if (!Money::parse(text, m)) throw std::runtime_error("bad money in test: " + text);
    return m;
}

} // namespace

// ---------------------------------------------------------------------------
// Money
// ---------------------------------------------------------------------------

TEST_CASE("Money: parses and formats prices exactly")
{
    CHECK(money("12").cents() == 1200);
    CHECK(money("12.5").cents() == 1250);
    CHECK(money("0.05").cents() == 5);
    CHECK(money("3.45").str() == "$3.45");
    CHECK(Money::fromCents(7).str() == "$0.07");
    Money m;
    CHECK(!Money::parse("", m));
    CHECK(!Money::parse("abc", m));
    CHECK(!Money::parse("1.234", m));   // more than two decimals
    CHECK(!Money::parse("-1", m));      // prices cannot be negative
    CHECK(!Money::parse(".", m));
}

TEST_CASE("Money: adding 0.10 one thousand times is exactly 100.00 (a double is not)")
{
    Money sum;
    double asDouble = 0.0;
    for (int i = 0; i < 1000; ++i) {
        sum += money("0.10");
        asDouble += 0.10;
    }
    CHECK(sum == money("100.00"));
    CHECK(asDouble != 100.0);   // shows WHY we use integer cents
}

// ---------------------------------------------------------------------------
// Catalogue
// ---------------------------------------------------------------------------

TEST_CASE("Catalogue: parses names, categories and default category")
{
    const Catalogue c = smallCatalogue();
    REQUIRE(c.size() == 4);
    CHECK(c.at(2).name == "Rice");
    CHECK(c.at(2).category == "Dry Goods");   // underscores become spaces
    CHECK(c.at(3).category == "General");
    CHECK(c.at(1).price == money("3.45"));
    CHECK(c.categories().size() == 4);
}

TEST_CASE("Catalogue: bad lines are rejected with the line number")
{
    auto parseText = [](const std::string& text) {
        std::istringstream in(text);
        return Catalogue::parse(in, "bad");
    };
    CHECK_THROWS(parseText("Apple 1.50 0.2 10 1\n"));                          // too few fields
    CHECK_THROWS(parseText("Apple abc 0.2 10 1 1\n"));                         // bad price
    CHECK_THROWS(parseText("Apple 1.50 -2 10 1 1\n"));                         // negative weight
    CHECK_THROWS(parseText("Apple 1.50 0.2 10 1 1\napple 2.00 0.2 1 1 2\n"));  // duplicate name
    CHECK_THROWS(parseText("Apple 1.50 0.2 10 1 1\nPear 2.00 0.2 1 1 1\n"));   // shelf used twice
    CHECK_THROWS(parseText("# only a comment\n"));                             // no products
    try {
        parseText("Apple 1.50 0.2 10 1 1\nPear x 0.2 1 1 2\n");
        CHECK(false);
    } catch (const DataError& e) {
        CHECK(std::string(e.what()).find("bad:2") != std::string::npos);      // line 2 reported
    }
}

TEST_CASE("Catalogue: findByName (binary search) is case-insensitive and returns -1 when missing")
{
    const Catalogue c = smallCatalogue();
    CHECK(c.findByName("Milk") == 1);
    CHECK(c.findByName("mIlK") == 1);
    CHECK(c.findByName("Apple") == 0);
    CHECK(c.findByName("Bread") == 3);
    CHECK(c.findByName("Banana") == -1);
    CHECK(c.findByName("") == -1);
}

TEST_CASE("Catalogue: binary search agrees with a linear scan for every real product")
{
    const Catalogue c = Catalogue::loadFromFile(std::string(WH_DATA_DIR) + "/Products.txt");
    REQUIRE(c.size() == 91);
    for (int i = 0; i < c.size(); ++i) {
        CHECK(c.findByName(c.at(i).name) == i);
        CHECK(c.findByName(toLower(c.at(i).name)) == i);
    }
    CHECK(c.findByName("Unicorn steak") == -1);
}

TEST_CASE("Catalogue: search matches part of a name or a category")
{
    const Catalogue c = smallCatalogue();
    CHECK((c.search("ric") == std::vector<int>{2}));      // "Rice"
    CHECK((c.search("DAIRY") == std::vector<int>{1}));    // category, any case
    CHECK(c.search("zzz").empty());
    CHECK(c.search("").size() == 4);                      // empty text matches everything
}

TEST_CASE("Catalogue: consumeStock never goes below zero")
{
    Catalogue c = smallCatalogue();
    CHECK(c.consumeStock(1, 2));
    CHECK(c.at(1).stock == 0);
    CHECK(!c.consumeStock(1, 1));
    CHECK(!c.consumeStock(0, 0));
    CHECK(!c.consumeStock(0, -3));
    CHECK(c.at(0).stock == 10);
}

// ---------------------------------------------------------------------------
// Cart
// ---------------------------------------------------------------------------

TEST_CASE("Cart: rejects zero, negative, unknown, over-stock and over-weight additions")
{
    const Catalogue c = smallCatalogue();
    Cart cart(5.0);
    CHECK(!cart.add(c, 0, 0).ok);
    CHECK(!cart.add(c, 0, -5).ok);
    CHECK(!cart.add(c, 99, 1).ok);
    CHECK(!cart.add(c, -1, 1).ok);
    CHECK(!cart.add(c, 1, 3).ok);    // only 2 milk in stock
    CHECK(cart.add(c, 1, 2).ok);
    CHECK(!cart.add(c, 1, 1).ok);    // the 2 in the cart count towards stock
    CHECK(!cart.add(c, 2, 2).ok);    // 2 kg milk + 4 kg rice > 5 kg
    CHECK(cart.add(c, 2, 1).ok);     // 2 + 2 = 4 kg is fine
    CHECK(cart.units() == 3);
    CHECK(cart.total(c) == money("18.90"));
}

TEST_CASE("Cart: remove some, then all")
{
    const Catalogue c = smallCatalogue();
    Cart cart;
    cart.add(c, 0, 5);
    CHECK(cart.remove(c, 0, 2).ok);
    CHECK(cart.quantityOf(0) == 3);
    CHECK(cart.remove(c, 0, 10).ok);   // removing more than there is removes the line
    CHECK(cart.empty());
    CHECK(!cart.remove(c, 0, 1).ok);
}

// ---------------------------------------------------------------------------
// Shop + receipts
// ---------------------------------------------------------------------------

TEST_CASE("Shop: checkout creates an order, takes stock and empties the cart")
{
    Shop shop(smallCatalogue(), "");
    CHECK(!shop.checkout(Priority::Standard).ok);   // empty cart
    shop.cart().add(shop.catalogue(), 0, 3);
    shop.cart().add(shop.catalogue(), 3, 1);
    const Shop::CheckoutResult r = shop.checkout(Priority::Express);
    REQUIRE(r.ok);
    CHECK(r.order.id == 1);
    CHECK(r.order.priority == Priority::Express);
    CHECK(r.order.units() == 4);
    CHECK(r.order.total() == money("6.60"));
    CHECK(shop.catalogue().at(0).stock == 7);
    CHECK(shop.cart().empty());
    CHECK(shop.history().size() == 1);
}

TEST_CASE("Shop: receipt text contains the reference, every line and the total; file is written")
{
    const std::string dir = (std::filesystem::temp_directory_path() / "wh_test_receipts").string();
    std::filesystem::remove_all(dir);
    Shop shop(smallCatalogue(), dir);
    shop.cart().add(shop.catalogue(), 2, 1);
    const Shop::CheckoutResult r = shop.checkout(Priority::Standard);
    REQUIRE(r.ok);
    const std::string text = Receipt::render(r.order);
    CHECK(text.find("ORD-000001") != std::string::npos);
    CHECK(text.find("Rice") != std::string::npos);
    CHECK(text.find("$12.00") != std::string::npos);
    CHECK(std::filesystem::exists(r.receiptPath));
    std::filesystem::remove_all(dir);
}

TEST_CASE("Shop: generated orders respect stock and the weight limit")
{
    Shop shop(Catalogue::loadFromFile(std::string(WH_DATA_DIR) + "/Products.txt"), "");
    std::vector<int> stockBefore;
    for (const Product& p : shop.catalogue().all()) stockBefore.push_back(p.stock);

    std::mt19937 rng(5);
    const std::vector<Order> orders = shop.generateOrders(50, rng);
    CHECK(orders.size() == 50);
    std::vector<int> taken(stockBefore.size(), 0);
    for (const Order& o : orders) {
        CHECK(!o.lines.empty());
        CHECK(o.weightKg() <= shop.cart().weightLimitKg() + 1e-9);
        for (const OrderLine& l : o.lines) taken[static_cast<std::size_t>(l.productIndex)] += l.qty;
    }
    for (std::size_t i = 0; i < taken.size(); ++i)
        CHECK(shop.catalogue().at(static_cast<int>(i)).stock == stockBefore[i] - taken[i]);
}
