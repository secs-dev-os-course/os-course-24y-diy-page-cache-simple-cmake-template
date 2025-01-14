cmake_minimum_required(VERSION 3.27)
project(BlockCache)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LIB_SOURCES
    app/block_cache.cpp
)

set(TEST_SOURCES
    app/test.cpp
)

set(BENCH_SOURCES
    app/bench1.cpp
)

include_directories(include)

add_library(block_cache SHARED ${LIB_SOURCES})
target_include_directories(block_cache PUBLIC include)

add_executable(test ${TEST_SOURCES})
target_link_libraries(test PRIVATE block_cache)

add_executable(bench ${BENCH_SOURCES})
target_link_libraries(bench PRIVATE block_cache)
