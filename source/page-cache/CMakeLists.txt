add_library(page-cache SHARED page_cache.c page_cache.h)
target_include_directories(page-cache PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
