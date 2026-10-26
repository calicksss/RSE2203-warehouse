# Experiments for the report. Every work package owns one experiment (see experiments/README.md).
add_executable(experiments experiments.cpp)
target_link_libraries(experiments PRIVATE wh_sim)
target_compile_definitions(experiments PRIVATE WH_DATA_DIR="${WH_DATA_DIR}" WH_RESULTS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/results")
wh_set_warnings(experiments)
