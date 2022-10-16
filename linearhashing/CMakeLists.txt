set(LINEAR_HASHING_SOURCES linearhashing.h LinearRadixTree.h)

add_library(nvmkv-linear-hashing STATIC ${LINEAR_HASHING_SOURCES})
target_compile_features(nvmkv-linear-hashing PUBLIC c_std_11 cxx_std_17)

target_link_libraries(nvmkv-linear-hashing PUBLIC nvmkv-fastalloc)