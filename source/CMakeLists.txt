cmake_minimum_required(VERSION 3.10)
project(block_cache)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(block_cache SHARED
    monolith/cache.cpp
    monolith/cache_api.cpp
)

target_include_directories(block_cache PRIVATE
    ${CMAKE_SOURCE_DIR}/monolith
)