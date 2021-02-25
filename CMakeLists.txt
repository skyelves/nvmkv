cmake_minimum_required(VERSION 3.10...3.19)
project(nvmkv)

add_subdirectory(node)
add_subdirectory(allocator)
add_subdirectory(blink)
add_subdirectory(art)
add_subdirectory(mass)
add_subdirectory(rng)

add_executable(nvmkv main.cpp)
target_link_libraries(nvmkv PRIVATE nvmkv-blink nvmkv-art nvmkv-mass nvmkv-rng)
