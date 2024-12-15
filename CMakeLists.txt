cmake_minimum_required(VERSION 3.12)

project(os-lab2 VERSION 0.0.0 LANGUAGES C)

option(OS_LAB2_DEVELOPER "Enable ${PROJECT_NAME} developer mode" False)

include(cmake/CompileOptions.cmake)
include(cmake/ClangFormat.cmake)
include(cmake/ClangTidy.cmake)

add_subdirectory(source)
