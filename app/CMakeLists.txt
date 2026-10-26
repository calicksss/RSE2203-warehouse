# The Warehouse executable. Owner: WP-F.
add_executable(Warehouse main.cpp)
target_link_libraries(Warehouse PRIVATE wh_ui)
target_compile_definitions(Warehouse PRIVATE WH_DEFAULT_DATA_DIR="${WH_DATA_DIR}")
wh_set_warnings(Warehouse)
