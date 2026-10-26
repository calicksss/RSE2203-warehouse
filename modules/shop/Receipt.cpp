#include "shop/Receipt.hpp"

#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace wh {

std::string currentTimestamp()
{
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);   // Windows version of the thread-safe call
#else
    localtime_r(&now, &local);   // std::localtime is not thread-safe
#endif
    char buf[32];
    std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M:%S", &local);
    return buf;
}

std::string Receipt::reference(const Order& order)
{
    std::ostringstream oss;
    oss << "ORD-" << std::setw(6) << std::setfill('0') << order.id;
    return oss.str();
}

std::string Receipt::render(const Order& order)
{
    const std::string rule(48, '-');
    const std::string heavy(48, '=');
    std::ostringstream out;
    out << heavy << '\n'
        << "        AUTOMATED GROCERY FULFILMENT CENTRE\n"
        << heavy << '\n'
        << "Order ref : " << reference(order) << '\n'
        << "Placed    : " << order.placedAt << '\n'
        << "Service   : " << toString(order.priority) << '\n'
        << rule << '\n'
        << std::left << std::setw(22) << "Item" << std::right << std::setw(5) << "Qty" << std::setw(10) << "Unit"
        << std::setw(11) << "Subtotal" << '\n'
        << rule << '\n';
    for (const OrderLine& line : order.lines) {
        const std::string name = line.name.size() > 21 ? line.name.substr(0, 21) : line.name;
        out << std::left << std::setw(22) << name << std::right << std::setw(5) << line.qty << std::setw(10)
            << line.unitPrice.str() << std::setw(11) << (line.unitPrice * line.qty).str() << '\n';
    }
    out << rule << '\n'
        << std::left << std::setw(22) << "Items" << std::right << std::setw(26) << order.units() << '\n'
        << std::left << std::setw(22) << "Total weight (kg)" << std::right << std::setw(26) << std::fixed
        << std::setprecision(2) << order.weightKg() << '\n'
        << std::left << std::setw(22) << "GRAND TOTAL" << std::right << std::setw(26) << order.total().str() << '\n'
        << heavy << '\n';
    return out.str();
}

std::string Receipt::save(const Order& order, const std::string& directory)
{
    std::filesystem::create_directories(directory);
    const std::string path = (std::filesystem::path(directory) / ("receipt_" + reference(order) + ".txt")).string();
    std::ofstream file(path);
    if (!file) throw std::runtime_error("cannot write receipt to " + path);
    file << render(order);
    return path;
}

} // namespace wh
