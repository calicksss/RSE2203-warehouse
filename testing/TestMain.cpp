/**
 * @file TestMain.cpp
 * @brief Runs every registered TEST_CASE. Optional argument: only run tests whose name contains it.
 *
 *     ./test_ds              run all tests in the ds module
 *     ./test_ds "merge"      run only tests with "merge" in the name
 */
#include <exception>
#include <iostream>
#include <string>

#include "TestFramework.hpp"

int main(int argc, char** argv)
{
    using namespace wh::test;
    const std::string filter = argc > 1 ? argv[1] : "";
    int run = 0;
    int failedTests = 0;
    for (const TestCase& t : registry()) {
        if (!filter.empty() && t.name.find(filter) == std::string::npos) continue;
        ++run;
        const int before = failureCount();
        try {
            t.body();
        } catch (const RequireFailed&) {
            // already reported
        } catch (const std::exception& e) {
            ++failureCount();
            std::cerr << "    UNEXPECTED EXCEPTION: " << e.what() << '\n';
        }
        const bool ok = failureCount() == before;
        if (!ok) ++failedTests;
        std::cout << (ok ? "[ PASS ] " : "[ FAIL ] ") << t.name << '\n';
    }
    std::cout << '\n' << (run - failedTests) << "/" << run << " tests passed\n";
    return failedTests == 0 ? 0 : 1;
}
