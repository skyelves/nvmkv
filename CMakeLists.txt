cmake_minimum_required(VERSION 3.10...3.19)
project(nvmkv)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_subdirectory(node)
add_subdirectory(allocator)
add_subdirectory(fastalloc)
add_subdirectory(blink)
add_subdirectory(cart)
add_subdirectory(mass)
add_subdirectory(rng)
add_subdirectory(hashtree)
add_subdirectory(murmur)
add_subdirectory(bobhash)
add_subdirectory(extendibleHash)
add_subdirectory(art)


add_executable(nvmkv main.cpp)
#target_link_libraries(nvmkv PRIVATE nvmkv-blink nvmkv-cart nvmkv-mass nvmkv-rng vmmalloc)
target_link_libraries(nvmkv PRIVATE nvmkv-blink nvmkv-cart nvmkv-mass
        nvmkv-rng nvmkv-ht nvmkv-extendible_hash
        nvmkv-murmur nvmkv-bobhash32 nvmkv-art)
