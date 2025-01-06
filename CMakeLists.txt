cmake_minimum_required(VERSION 3.10)

project(PageCacheLib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB LIB_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/lib/src/*.cpp)
file(GLOB LIB_HEADER_FILES ${CMAKE_CURRENT_SOURCE_DIR}/lib/include/*.h)
set(LIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/include)

add_library(PageCacheLib STATIC ${LIB_SRC_FILES} ${LIB_HEADER_FILES})
target_include_directories(PageCacheLib PUBLIC ${LIB_INCLUDE_DIR})

add_subdirectory(tests)