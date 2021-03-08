set(HT_SOURCES
        hashtree.cpp hashtree.h
        hashtree_node.cpp hashtree_node.h
        bobhash32.cpp bobhash32.h)

add_library(nvmkv-ht STATIC ${HT_SOURCES})
target_link_libraries(nvmkv-ht PUBLIC nvmkv-allocator nvmkv-node)