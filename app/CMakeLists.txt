cmake_minimum_required(VERSION 3.12)

project(App
        VERSION 0.0.0
        DESCRIPTION "Application module"
        LANGUAGES C CXX
)

cmake_policy(SET CMP0076 NEW)

add_executable(app app.cpp)

target_include_directories(app PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(app PRIVATE blockcache)
