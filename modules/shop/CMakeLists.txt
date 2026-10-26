# shop: catalogue, cart, checkout, receipts. Owner: WP-B.
add_library(wh_shop STATIC Money.cpp Catalogue.cpp Cart.cpp Receipt.cpp Shop.cpp)
target_link_libraries(wh_shop PUBLIC wh_core wh_ds)
wh_set_warnings(wh_shop)

wh_add_test(test_shop tests/test_shop.cpp wh_shop)
