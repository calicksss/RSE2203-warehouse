cmake_minimum_required(VERSION 3.16)
project(RSE2203_Warehouse VERSION 4.0 LANGUAGES CXX)

# ---------------------------------------------------------------------------
# RSE2203 Team Project: Automated Grocery Warehouse
#
# The code is split into modules (one folder each under modules/). Every module
# is its own static library with its own tests, so each work package can be
# built and tested on its own:
#
#     cmake --build build --target test_ds        (only the ds module + its tests)
#     ctest --test-dir build -R navigation        (only the navigation tests)
#
# Dependency order (a module may only use modules to its left):
#     core -> ds -> shop -> navigation -> picking -> fulfilment -> sim -> ui -> app
# (shop and navigation are independent of each other; both only need core + ds.)
# ---------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(WH_BUILD_UI          "Build the FTXUI terminal interface (turn OFF for faster algorithm work)" ON)
option(WH_BUILD_TESTS       "Build the unit tests"                       ON)
option(WH_BUILD_EXPERIMENTS "Build the experiments program"              ON)
option(WH_WARNINGS_AS_ERRORS "Treat compiler warnings as errors"         OFF)
option(WH_SANITIZE          "Build with AddressSanitizer + UBSan (GCC/Clang only)" OFF)

# All headers are included as "module/File.hpp", e.g. #include "ds/MinHeap.hpp".
set(WH_MODULES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/modules)
set(WH_DATA_DIR    ${CMAKE_CURRENT_SOURCE_DIR}/data)

# Shared warning flags, applied per target.
function(wh_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8)
        if(WH_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor)
        if(WH_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
        if(WH_SANITIZE)
            target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=address,undefined)
        endif()
    endif()
endfunction()

# wh_add_test(<name> <source> <libraries...>): one test program per module.
if(WH_BUILD_TESTS)
    enable_testing()
    add_library(wh_testing STATIC testing/TestMain.cpp)
    target_include_directories(wh_testing PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/testing)
    wh_set_warnings(wh_testing)
endif()

function(wh_add_test name source)
    if(NOT WH_BUILD_TESTS)
        return()
    endif()
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE wh_testing ${ARGN})
    target_compile_definitions(${name} PRIVATE WH_DATA_DIR="${WH_DATA_DIR}")
    wh_set_warnings(${name})
    add_test(NAME ${name} COMMAND ${name})
endfunction()

# ---------------------------------------------------------------------------
# Modules (each folder has its own CMakeLists.txt and README.md)
# ---------------------------------------------------------------------------
add_subdirectory(modules/core)        # WP-F  shared vocabulary
add_subdirectory(modules/ds)          # WP-A  data structures + sorting/searching
add_subdirectory(modules/shop)        # WP-B  catalogue, cart, checkout
add_subdirectory(modules/navigation)  # WP-C  map, BFS, A*, route planning
add_subdirectory(modules/picking)     # WP-D  picking robots + traffic control
add_subdirectory(modules/fulfilment)  # WP-E  job manager + sorting station
add_subdirectory(modules/sim)         # WP-F  simulation loop

if(WH_BUILD_UI)
    # FTXUI (MIT licence) vendored as its official amalgamated .hpp/.cpp pair,
    # so nothing needs to be downloaded. Only the ui module links it.
    find_package(Threads REQUIRED)
    add_library(ftxui STATIC third_party/ftxui/ftxui.cpp)
    target_include_directories(ftxui SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ftxui)
    target_compile_features(ftxui PUBLIC cxx_std_17)
    target_link_libraries(ftxui PUBLIC Threads::Threads)
    if(WIN32)
        target_compile_definitions(ftxui PUBLIC UNICODE _UNICODE NOMINMAX)
    endif()
    if(MSVC)
        target_compile_options(ftxui PRIVATE /utf-8 /bigobj)
    elseif(MINGW)
        target_compile_options(ftxui PRIVATE -Wa,-mbig-obj)
    endif()

    add_subdirectory(modules/ui)      # WP-F  terminal interface
    add_subdirectory(app)             # WP-F  main.cpp
endif()

if(WH_BUILD_EXPERIMENTS)
    add_subdirectory(experiments)     # all WPs: experiments for the report
endif()
