set(VLHT_SOURCES
        varLengthHashTree.cpp varLengthHashTree.h
        varLengthHashTree_node.cpp varLengthHashTree_node.h)

add_library(nvmkv-vlht STATIC ${VLHT_SOURCES})
target_link_libraries(nvmkv-vlht PUBLIC nvmkv-fastalloc nvmkv-murmur nvmkv-bobhash32)