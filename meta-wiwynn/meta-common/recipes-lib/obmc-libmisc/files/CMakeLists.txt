cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

project(obmcmisc VERSION 0.0.1)

add_library(obmcmisc SHARED libdbus.c libmath.c libregister.c libsemaphore.c)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

add_definitions (-Wno-psabi)

set_target_properties(obmcmisc PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(obmcmisc PROPERTIES SOVERSION "0")

target_link_libraries (obmcmisc -lpthread)

install(TARGETS obmcmisc DESTINATION lib)
install(FILES libmisc.h DESTINATION include/openbmc)
