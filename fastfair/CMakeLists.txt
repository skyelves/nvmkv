set(FASTFAIR_SOURCES fastfair.cpp fastfair.h concurrency_fastfair.cpp concurrency_fastfair.h varlengthfastfair.h varlengthfastfair.cpp)

add_library(nvmkv-fastfair STATIC ${FASTFAIR_SOURCES})

include_directories( /usr/local/include)

target_link_libraries(nvmkv-fastfair PUBLIC nvmkv-fastalloc)