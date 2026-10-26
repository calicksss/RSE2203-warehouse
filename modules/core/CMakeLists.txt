# core: shared vocabulary used by every module (types, config, events, robot base class).
# Owner: WP-F (integration). Changes here affect everyone: ask the whole team to review.
add_library(wh_core STATIC Config.cpp)
target_include_directories(wh_core PUBLIC ${WH_MODULES_DIR})
target_compile_features(wh_core PUBLIC cxx_std_17)
wh_set_warnings(wh_core)
