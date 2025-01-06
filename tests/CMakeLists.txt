cmake_minimum_required(VERSION 3.10)

project(PageCacheTests LANGUAGES CXX)

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY  https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)
FetchContent_MakeAvailable(googletest)

file(GLOB TEST_SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(PageCacheTests ${TEST_SRC_FILES})
target_link_libraries(PageCacheTests gtest gtest_main PageCacheLib)