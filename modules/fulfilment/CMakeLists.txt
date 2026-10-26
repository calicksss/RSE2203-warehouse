# fulfilment: job manager (queues, scheduling, charging) and the sorting station. Owner: WP-E.
add_library(wh_fulfilment STATIC ShelfRegistry.cpp SortingRobot.cpp JobManager.cpp)
target_link_libraries(wh_fulfilment PUBLIC wh_core wh_ds wh_shop wh_navigation wh_picking)
wh_set_warnings(wh_fulfilment)

wh_add_test(test_fulfilment tests/test_fulfilment.cpp wh_fulfilment)
