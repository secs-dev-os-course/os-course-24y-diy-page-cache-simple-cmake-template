cmake_minimum_required(VERSION 3.10)
project(block_cache)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Указываем использование MinGW
set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

# Библиотека кэша
add_library(block_cache SHARED
    source/monolith/cache.cpp
    source/monolith/cache_api.cpp
)

target_include_directories(block_cache PRIVATE
    ${CMAKE_SOURCE_DIR}/source/monolith
)

# Оригинальная версия тестера
add_executable(io_test_std
    source/monolith/io-thpt-read.cpp
)

# Модифицированная версия тестера с кэшем
add_executable(io_test_cache
    source/monolith/io-thpt-read-mod.cpp
)

target_link_libraries(io_test_cache PRIVATE block_cache)
target_include_directories(io_test_cache PRIVATE
    ${CMAKE_SOURCE_DIR}/source/monolith
)

# Модифицированная версия тестера с кэшем
add_executable(test_prefetching
    source/monolith/test-prefetching.cpp
)

target_link_libraries(test_prefetching PRIVATE block_cache)
target_include_directories(test_prefetching PRIVATE
    ${CMAKE_SOURCE_DIR}/source/monolith
)