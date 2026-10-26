#pragma once
/**
 * @file Receipt.hpp
 * @brief Text receipts. render() builds the text (easy to unit-test); save() writes it to a file.
 */
#include <string>

#include "shop/Order.hpp"

namespace wh {

namespace Receipt {
std::string reference(const Order& order);   ///< "ORD-000042"
std::string render(const Order& order);
/// Writes receipt_ORD-xxxxxx.txt into `directory` (created if missing). Returns the file path.
/// Throws std::runtime_error if the file cannot be written.
std::string save(const Order& order, const std::string& directory);
} // namespace Receipt

/// Local time as "YYYY-MM-DD HH:MM:SS".
std::string currentTimestamp();

} // namespace wh
