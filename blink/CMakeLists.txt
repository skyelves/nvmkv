set(BLINK_SOURCES
        latch.h
        blink_tree.cpp blink_tree.h
        mapping_array.cpp mapping_array.h
        blink_node.cpp blink_node.h)

add_library(nvmkv-blink STATIC ${BLINK_SOURCES})
target_link_libraries(nvmkv-blink PUBLIC nvmkv-allocator nvmkv-node)
