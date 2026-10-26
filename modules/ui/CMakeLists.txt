# ui: FTXUI terminal interface (Views = drawing, App = keys/buttons/clock). Owner: WP-F.
add_library(wh_ui STATIC Views.cpp App.cpp)
target_link_libraries(wh_ui PUBLIC wh_sim ftxui)
wh_set_warnings(wh_ui)
