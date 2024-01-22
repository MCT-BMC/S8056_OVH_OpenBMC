cmake_minimum_required(VERSION 3.5 FATAL_ERROR)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

project(obmcsysfs VERSION 0.0.1)

add_library(obmcsysfs SHARED libsysfs.cpp)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

add_definitions (-Wno-psabi)

set_target_properties(obmcsysfs PROPERTIES VERSION ${PROJECT_VERSION})
set_target_properties(obmcsysfs PROPERTIES SOVERSION "0")

target_compile_definitions(
    obmcsysfs PRIVATE
    $<$<BOOL:$<MAKE_C_IDENTIFIER:"${MOTHERBOARD_CONFIG_PATH}">>: -DMOTHERBOARD_CONFIG_PATH="${MOTHERBOARD_CONFIG_PATH}">
)

install(TARGETS obmcsysfs DESTINATION lib)
install(FILES libsysfs.hpp DESTINATION include/openbmc)
