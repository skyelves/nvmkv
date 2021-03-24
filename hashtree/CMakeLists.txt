set(HT_SOURCES
        hashtree.cpp hashtree.h
        hashtree_node.cpp hashtree_node.h
        extendible_hash.cpp extendible_hash.h)

add_library(nvmkv-ht STATIC ${HT_SOURCES})
target_link_libraries(nvmkv-ht PUBLIC nvmkv-fastalloc nvmkv-murmur nvmkv-bobhash32)