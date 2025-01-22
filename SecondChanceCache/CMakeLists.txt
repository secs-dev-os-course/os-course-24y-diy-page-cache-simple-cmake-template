cmake_minimum_required(VERSION 3.12)
project(SecondChanceCache
        VERSION 0.0.0
        DESCRIPTION "My DLL module"
        LANGUAGES C CXX
)

cmake_policy(SET CMP0076 NEW) # avoid warning of relative paths translation

add_library(SecondChanceCache SHARED ""
        SecondChanceCache.cpp
        SecondChanceCache.h)
target_include_directories(SecondChanceCache PRIVATE ${CMAKE_SOURCE_DIR})

target_sources(SecondChanceCache PRIVATE
        SecondChanceCache.h
        SecondChanceCache.cpp
)

target_compile_definitions(SecondChanceCache PRIVATE
        SECOND_CHANCE_CACHE_EXPORTS)

set(COPY_TO_APP OFF)

if (COPY_TO_APP)
    add_custom_command(TARGET SecondChanceCache POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${PROJECT_BINARY_DIR}/SecondChanceCache.dll"
            "${CMAKE_BINARY_DIR}/app")
endif ()