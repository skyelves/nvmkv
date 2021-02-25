set(ALLOCATOR_SOURCES allocator.cpp allocator.h)

add_library (nvmkv-allocator STATIC ${ALLOCATOR_SOURCES})
target_compile_features(nvmkv-allocator PUBLIC c_std_11 cxx_std_17)

find_package(Threads REQUIRED)
target_link_libraries(nvmkv-allocator PUBLIC Threads::Threads)
