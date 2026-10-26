#pragma once
/**
 * @file Errors.hpp
 * @brief Exception thrown when an input file (map, products) is invalid.
 *
 * main() catches it and prints "file:line: message", so a typo in a data file
 * gives a clear message instead of a crash.
 */
#include <stdexcept>
#include <string>

namespace wh {

class DataError : public std::runtime_error {
public:
    DataError(const std::string& source, int line, const std::string& message)
        : std::runtime_error(source + (line > 0 ? ":" + std::to_string(line) : std::string()) + ": " + message)
    {}
};

} // namespace wh
