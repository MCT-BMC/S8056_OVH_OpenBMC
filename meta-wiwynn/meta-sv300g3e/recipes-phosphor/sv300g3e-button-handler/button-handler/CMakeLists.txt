cmake_minimum_required (VERSION 3.1 FATAL_ERROR)
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

project (button-handler CXX)

option (HUNTER_ENABLED "Enable hunter package pulling" OFF)

set (EXTERNAL_PACKAGES Boost sdbusplus-project nlohmann-json)
set (LINK_LIBS -lsystemd -lgpiod stdc++fs sdbusplus)

add_definitions (-DBOOST_ERROR_CODE_HEADER_ONLY)
add_definitions (-DBOOST_SYSTEM_NO_DEPRECATED)
add_definitions (-DBOOST_ALL_NO_LIB)
add_definitions (-DBOOST_NO_RTTI)
add_definitions (-DBOOST_NO_TYPEID)
add_definitions (-DBOOST_ASIO_DISABLE_THREADS)

link_directories (${EXTERNAL_INSTALL_LOCATION}/lib)

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/include)

set (SRC_FILES src/ButtonHandler.cpp)
add_executable (button-handler src/ButtonHandlerMain.cpp ${SRC_FILES})
target_link_libraries (button-handler ${LINK_LIBS})


set (SERVICE_FILE_SRC_DIR ${PROJECT_SOURCE_DIR}/service)
set (
    SERVICE_FILES
    ${SERVICE_FILE_SRC_DIR}/xyz.openbmc_project.button-handler.service
)
set (SERVICE_FILE_INSTALL_DIR /lib/systemd/system/)

install (TARGETS button-handler DESTINATION bin)
install (FILES ${SERVICE_FILES} DESTINATION ${SERVICE_FILE_INSTALL_DIR})
