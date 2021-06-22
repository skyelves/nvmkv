set(CHT_SOURCES
        concurrency_hashtree.cpp concurrency_hashtree.h
        concurrency_hashtree_node.cpp concurrency_hashtree_node.h)

add_library(nvmkv-cht STATIC ${CHT_SOURCES})
target_link_libraries(nvmkv-cht PUBLIC nvmkv-fastalloc nvmkv-murmur nvmkv-bobhash32)