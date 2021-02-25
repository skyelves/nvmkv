set(NODE_SOURCES node.cpp node.h)

add_library(nvmkv-node STATIC ${NODE_SOURCES})
target_compile_features(nvmkv-node PUBLIC c_std_11 cxx_std_17)