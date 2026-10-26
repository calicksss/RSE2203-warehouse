# sim: the simulation loop that ties every module together. Owner: WP-F (integration).
add_library(wh_sim STATIC Simulation.cpp)
target_link_libraries(wh_sim PUBLIC wh_fulfilment wh_picking wh_navigation wh_shop wh_core)
wh_set_warnings(wh_sim)

wh_add_test(test_sim tests/test_sim.cpp wh_sim)
