cmake_minimum_required (VERSION 2.8.10 FATAL_ERROR)
set (BUILD_SHARED_LIBRARIES OFF)
include (ExternalProject)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (
    CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -lstdc++fs \
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

set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

project (discrete-sensor CXX)

set (EXTERNAL_PACKAGES Boost sdbusplus-project nlohmann-json)
set (LINK_LIBS stdc++fs -lobmcdbus -lobmcmisc -lobmci2c -lsystemd -lgpiod sdbusplus)

add_definitions (-DBOOST_ERROR_CODE_HEADER_ONLY)
add_definitions (-DBOOST_SYSTEM_NO_DEPRECATED)
add_definitions (-DBOOST_ALL_NO_LIB)
add_definitions (-DBOOST_NO_RTTI)
add_definitions (-DBOOST_NO_TYPEID)
add_definitions (-DBOOST_ASIO_DISABLE_THREADS)
add_definitions (-DISCRETE_SENSOR_DBUS_PATH="/")

link_directories (${EXTERNAL_INSTALL_LOCATION}/lib)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

add_executable (discrete-sensor main.cpp Sensor.cpp PsuStatusMonitor.cpp)
add_dependencies (discrete-sensor ${EXTERNAL_PACKAGES})
target_link_libraries (discrete-sensor ${LINK_LIBS})

set (SERVICE_FILE_INSTALL_DIR /lib/systemd/system/)

install (TARGETS discrete-sensor DESTINATION bin)
install (FILES ${SERVICE_FILES} DESTINATION ${SERVICE_FILE_INSTALL_DIR})

target_compile_definitions  (
    discrete-sensor PRIVATE $<$<BOOL:$<MAKE_C_IDENTIFIER:"${DISCRETE_SENSOR_DBUS_PATH}">>:
    -DDISCRETE_SENSOR_DBUS_PATH="${DISCRETE_SENSOR_DBUS_PATH}">
)

