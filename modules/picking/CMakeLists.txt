# picking: picking robots, battery, docks and traffic control (collision + obstacle avoidance). Owner: WP-D.
add_library(wh_picking STATIC PickingRobot.cpp TrafficController.cpp)
target_link_libraries(wh_picking PUBLIC wh_core wh_ds wh_navigation)
wh_set_warnings(wh_picking)

wh_add_test(test_picking tests/test_picking.cpp wh_picking)
