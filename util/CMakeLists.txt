set(UTIL_SOURCES util.cpp util.h)

add_library(nvmkv-util STATIC ${UTIL_SOURCES})
target_compile_features(nvmkv-util PUBLIC c_std_11 cxx_std_17)