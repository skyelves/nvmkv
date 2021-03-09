cmake_minimum_required(VERSION 3.10...3.19)
project(nvmkv)

add_subdirectory(node)
add_subdirectory(allocator)
add_subdirectory(blink)
add_subdirectory(cart)
add_subdirectory(mass)
add_subdirectory(rng)
add_subdirectory(hashtree)


add_executable(nvmkv main.cpp)
#target_link_libraries(nvmkv PRIVATE nvmkv-blink nvmkv-cart nvmkv-mass nvmkv-rng vmmalloc)
target_link_libraries(nvmkv PRIVATE nvmkv-blink nvmkv-cart nvmkv-mass nvmkv-rng nvmkv-ht)
