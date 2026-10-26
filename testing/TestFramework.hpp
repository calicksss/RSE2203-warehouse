#pragma once
/**
 * @file TestFramework.hpp
 * @brief A very small unit-test framework (no downloads needed).
 *
 * Usage:
 *     TEST_CASE("queue keeps FIFO order") {
 *         LinkedQueue<int> q;
 *         q.enqueue(1);
 *         CHECK(q.front() == 1);      // records a failure and keeps going
 *         REQUIRE(!q.empty());        // records a failure and stops this test
 *         CHECK_THROWS(q.dequeue());  // passes only if the expression throws
 *     }
 *
 * How it works: TEST_CASE defines a function and a static "Registrar" object.
 * The Registrar's constructor runs before main() and adds the function to a
 * list; TestMain.cpp then runs every function in the list.
 */
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace wh::test {

struct TestCase {
    std::string name;
    std::function<void()> body;
};

inline std::vector<TestCase>& registry()
{
    static std::vector<TestCase> tests;
    return tests;
}

inline int& failureCount()
{
    static int failures = 0;
    return failures;
}

struct RequireFailed {};

struct Registrar {
    Registrar(const char* name, std::function<void()> body) { registry().push_back({name, std::move(body)}); }
};

inline void reportFailure(const char* expr, const char* file, int line)
{
    ++failureCount();
    std::cerr << "    FAILED: " << expr << "  (" << file << ":" << line << ")\n";
}

} // namespace wh::test

#define WH_CONCAT_INNER(a, b) a##b
#define WH_CONCAT(a, b) WH_CONCAT_INNER(a, b)
#define TEST_CASE(name)                                                                                  \
    static void WH_CONCAT(whTest_, __LINE__)();                                                          \
    static const ::wh::test::Registrar WH_CONCAT(whReg_, __LINE__)(name, &WH_CONCAT(whTest_, __LINE__)); \
    static void WH_CONCAT(whTest_, __LINE__)()

#define CHECK(expr)                                                        \
    do {                                                                   \
        if (!(expr)) ::wh::test::reportFailure(#expr, __FILE__, __LINE__); \
    } while (false)

#define REQUIRE(expr)                                                \
    do {                                                             \
        if (!(expr)) {                                               \
            ::wh::test::reportFailure(#expr, __FILE__, __LINE__);    \
            throw ::wh::test::RequireFailed{};                       \
        }                                                            \
    } while (false)

#define CHECK_THROWS(expr)                                                                         \
    do {                                                                                           \
        bool whThrew = false;                                                                      \
        try { (void)(expr); } catch (...) { whThrew = true; }                                      \
        if (!whThrew) ::wh::test::reportFailure("expected exception: " #expr, __FILE__, __LINE__); \
    } while (false)
