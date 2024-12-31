cmake_minimum_required(VERSION 3.10)

project(PageCache LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SRC_FILES "src/*.cpp")
set(INCLUDE_DIR "include")

add_executable(PageCache ${SRC_FILES})

target_include_directories(PageCache PRIVATE ${INCLUDE_DIR})