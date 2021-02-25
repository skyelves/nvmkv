set(ART_SOURCES
        art.cpp art.h
        art_node.cpp art_node.h)

add_library(nvmkv-art STATIC ${ART_SOURCES})
target_link_libraries(nvmkv-art PUBLIC nvmkv-allocator nvmkv-node)