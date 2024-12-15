add_executable(page-cache-app main.c)
target_link_libraries(page-cache-app PRIVATE page-cache)
target_include_directories(page-cache-app PRIVATE page-cache)