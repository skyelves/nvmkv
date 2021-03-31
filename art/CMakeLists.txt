set(ART_SOURCES art.cpp art.h)

add_library(nvmkv-art STATIC ${ART_SOURCES})
target_compile_features(nvmkv-art PUBLIC c_std_11 cxx_std_17)

target_link_libraries(nvmkv-art PUBLIC nvmkv-fastalloc)