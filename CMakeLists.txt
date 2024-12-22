cmake_minimum_required(VERSION 3.12)

project(MyBestProject
        VERSION 0.0.0
        DESCRIPTION ""
        LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(blockcache)
add_subdirectory(app)
