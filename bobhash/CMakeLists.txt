set(BOBHASH_SOURCES bobhash32.cpp bobhash32.h)

add_library(nvmkv-bobhash32 STATIC ${BOBHASH_SOURCES})
target_compile_features(nvmkv-bobhash32 PUBLIC c_std_11 cxx_std_17)