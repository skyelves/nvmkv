set(MURMUR_SOURCES murmur.cpp murmur.h)

add_library(nvmkv-murmur STATIC ${MURMUR_SOURCES})
target_compile_features(nvmkv-murmur PUBLIC c_std_11 cxx_std_17)