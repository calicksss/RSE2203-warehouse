# ds: data structures and sorting/searching algorithms (header-only templates).
# Owner: WP-A. Knows nothing about the warehouse, so it can be tested on plain ints.
add_library(wh_ds INTERFACE)
target_include_directories(wh_ds INTERFACE ${WH_MODULES_DIR})
target_compile_features(wh_ds INTERFACE cxx_std_17)

wh_add_test(test_ds tests/test_ds.cpp wh_ds)
