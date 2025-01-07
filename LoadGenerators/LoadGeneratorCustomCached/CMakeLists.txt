cmake_minimum_required(VERSION 3.10)

project(TapeSort VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

file(GLOB_RECURSE LIB_SOURCES src/*.cpp)
list(REMOVE_ITEM LIB_SOURCES src/main.cpp)

add_library(TapeSortl STATIC
        ${LIB_SOURCES}
        include
)
target_include_directories(TapeSortl PUBLIC include)

add_dependencies(TapeSortl PageCacheLib)
target_link_libraries(TapeSortl PageCacheLib)