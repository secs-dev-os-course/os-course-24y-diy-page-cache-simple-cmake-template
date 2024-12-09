cmake_minimum_required(VERSION 3.12)
project(app
        VERSION 0.0.0
        DESCRIPTION "Application module"
        LANGUAGES C CXX
)

cmake_policy(SET CMP0076 NEW) # avoid warning of relative paths translation

# Создание динамической библиотеки (cache)
add_library(cache SHARED
    cache.cpp  # Исходный файл для кэш библиотеки
)

# Указываем директорию с заголовочными файлами для cache
target_include_directories(cache PRIVATE ${CMAKE_SOURCE_DIR})

# Добавляем исполнимый файл для приложения
add_executable(app
    app.cpp          # Главный файл с реализацией выбора и запуска нагрузчика
    ema_search_int.cpp  # Добавляем исходник с реализацией нагрузки
    dedup.cpp
    combined.cpp
)

# Линкуем исполнимый файл с библиотекой cache
target_link_libraries(app PRIVATE cache)

# Добавляем директорию с заголовочными файлами для исполнимого файла
target_include_directories(app PRIVATE ${CMAKE_SOURCE_DIR})
