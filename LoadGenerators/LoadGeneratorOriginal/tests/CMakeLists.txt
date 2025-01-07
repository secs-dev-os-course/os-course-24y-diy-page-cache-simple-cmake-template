cmake_minimum_required(VERSION 3.10)

project(Tests VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FetchContent)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
)
FetchContent_MakeAvailable(googletest)

file(GLOB_RECURSE TESTS *.cpp)

add_executable(Tests ${TESTS})
target_include_directories(Tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(Tests gtest TapeSortl)
