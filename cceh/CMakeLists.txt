set(CCEH_SOURCES cacheline_concious_extendible_hash.cpp cacheline_concious_extendible_hash.h concurrency_cceh.cpp concurrency_cceh.h)

add_library(nvmkv-cceh STATIC ${CCEH_SOURCES})
target_compile_features(nvmkv-cceh PUBLIC c_std_11 cxx_std_17)

target_link_libraries(nvmkv-cceh PUBLIC nvmkv-fastalloc)