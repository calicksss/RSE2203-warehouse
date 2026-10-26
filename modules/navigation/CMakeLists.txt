# navigation: map, BFS, A*, distance cache, route planning, obstacles. Owner: WP-C.
add_library(wh_navigation STATIC GridMap.cpp Pathfinding.cpp DistanceTable.cpp RoutePlanner.cpp ObstacleManager.cpp)
target_link_libraries(wh_navigation PUBLIC wh_core wh_ds)
wh_set_warnings(wh_navigation)

wh_add_test(test_navigation tests/test_navigation.cpp wh_navigation)
