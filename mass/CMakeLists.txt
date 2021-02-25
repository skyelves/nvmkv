set(MASS_SOURCES
        mass_tree.cpp mass_tree.h
        mass_node.cpp mass_node.h)

add_library(nvmkv-mass STATIC ${MASS_SOURCES})
target_link_libraries(nvmkv-mass PUBLIC nvmkv-allocator nvmkv-node)