cmake_minimum_required(VERSION 3.12)
project(lab2
        VERSION 0.0.0
        DESCRIPTION "Lab2 DLL module"
        LANGUAGES C CXX
)

cmake_policy(SET CMP0076 NEW) # Avoid warning of relative paths translation

# Создаем библиотеку (DLL)
add_library(lab2 SHARED)

# Указываем заголовочные файлы и исходники
target_sources(lab2
        PRIVATE
        lab2.cpp
        cache.cpp
)

# Указываем директории для заголовочных файлов
target_include_directories(lab2 PUBLIC ${CMAKE_SOURCE_DIR})

# Установка стандартов C++
set_target_properties(lab2 PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)

# Установка имени библиотеки и версии (для платформы Windows имя будет lab2.dll)
set_target_properties(lab2 PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    OUTPUT_NAME "lab2"
)
