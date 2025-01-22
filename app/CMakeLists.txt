cmake_minimum_required(VERSION 3.12)
project(app
        VERSION 0.0.0
        DESCRIPTION "Application module"
        LANGUAGES C CXX
)

cmake_policy(SET CMP0076 NEW) # avoid warning of relative paths translation

add_executable(app_without_cache
        ../SecondChanceCache/SecondChanceCache.cpp
        ../SecondChanceCache/SecondChanceCache.h
        ../SecondChanceCache/API.cpp
        ../SecondChanceCache/API.h
        ema-search-int.h
        ema-search-int.cpp
        app_without_cache.cpp)

add_executable(app_with_cache
        ../SecondChanceCache/SecondChanceCache.cpp
        ../SecondChanceCache/SecondChanceCache.h
        ../SecondChanceCache/API.cpp
        ../SecondChanceCache/API.h
        ema-search-int.h
        ema-search-int.cpp
        app_with_cache.cpp)

target_include_directories(app_with_cache PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(app_without_cache PRIVATE ${CMAKE_SOURCE_DIR})


target_sources(app_without_cache
        PRIVATE
        app_without_cache.cpp)

target_sources(app_with_cache
        PRIVATE
        app_with_cache.cpp
)