set(CART_SOURCES
        cart.cpp cart.h
        cart_node.cpp cart_node.h)

add_library(nvmkv-cart STATIC ${CART_SOURCES})
target_link_libraries(nvmkv-cart PUBLIC nvmkv-fastalloc nvmkv-node)