cmake_minimum_required (VERSION 3.5 FATAL_ERROR)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (
    CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} \
    -Werror \
    -Wall \
    -Wextra \
    -Wnon-virtual-dtor \
    -Wold-style-cast \
    -Wcast-align \
    -Wunused \
    -Woverloaded-virtual \
    -Wpedantic \
    -Wmisleading-indentation \
    -Wduplicated-cond \
    -Wduplicated-branches \
    -Wlogical-op \
    -Wnull-dereference \
    -Wuseless-cast \
    -Wdouble-promotion \
    -Wformat=2 \
    -Wno-sign-compare \
    -Wno-reorder \
    -Wshadow \
    -Wconversion \
"
)

project (guid-generator CXX)

add_executable (guid-generator main.cpp)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries (guid-generator -luuid)
target_link_libraries (guid-generator -lobmcmisc)
target_link_libraries (guid-generator -lobmcsysfs)

install (TARGETS guid-generator DESTINATION bin)
