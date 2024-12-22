
cmake_minimum_required(VERSION 3.12)

project(BlockCache
        VERSION 0.0.0
        DESCRIPTION "BlockCache Library"
        LANGUAGES C CXX
)

add_library(blockcache SHARED blockcache.c)

target_include_directories(blockcache PRIVATE ${CMAKE_SOURCE_DIR}/include)
