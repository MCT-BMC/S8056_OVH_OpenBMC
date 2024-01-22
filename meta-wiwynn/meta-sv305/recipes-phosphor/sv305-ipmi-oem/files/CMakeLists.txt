cmake_minimum_required (VERSION 3.5 FATAL_ERROR)

cmake_policy (SET CMP0054 NEW)

option (YOCTO "Use YOCTO depedencies system" OFF)
include (ExternalProject)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

project (sv305-ipmi-oem CXX)

add_definitions (-DBOOST_ERROR_CODE_HEADER_ONLY)
add_definitions (-DBOOST_SYSTEM_NO_DEPRECATED)
add_definitions (-DBOOST_ALL_NO_LIB)
add_definitions (-DBOOST_NO_RTTI)
add_definitions (-DBOOST_NO_TYPEID)
add_definitions (-DBOOST_ASIO_DISABLE_THREADS)
add_definitions (-DBOOST_COROUTINES_NO_DEPRECATION_WARNING)
add_definitions (-Wno-psabi)

if (NOT YOCTO)
    # headers that can't be built without yocto
    include_directories (non-yocto)

    configure_file (CMakeLists.txt)
    execute_process (
        COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/3rdparty
    )
    execute_process (
        COMMAND ${CMAKE_COMMAND} --build .
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/3rdparty
    )

    set (CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/prefix ${CMAKE_PREFIX_PATH})
    include_directories (${CMAKE_BINARY_DIR}/prefix/include)
    link_directories (${CMAKE_BINARY_DIR}/prefix/lib)

    include_directories (${CMAKE_BINARY_DIR}/sdbusplus-src)
    link_directories (${CMAKE_BINARY_DIR}/sdbusplus-src/.libs)
    include_directories (${CMAKE_BINARY_DIR}/phosphor-logging-src)
    link_directories (${CMAKE_BINARY_DIR}/phosphor-logging-src/.libs)
    # include_directories (${CMAKE_BINARY_DIR}/intel-dbus-interfaces-src)
    # link_directories (${CMAKE_BINARY_DIR}/intel-dbus-interfaces-src/.libs)
    include_directories (${CMAKE_BINARY_DIR}/ipmid/user_channel)
    include_directories (${CMAKE_BINARY_DIR}) # link_directories
                                              # (${CMAKE_BINARY_DIR}/sdbusplus-
                                              # src/.libs)
endif ()

if (YOCTO)
    find_package (PkgConfig REQUIRED)
    pkg_check_modules (LOGGING phosphor-logging REQUIRED)
    include_directories (${LOGGING_INCLUDE_DIRS})
    link_directories (${LOGGING_LIBRARY_DIRS})

    pkg_check_modules (LIBIPMID libipmid REQUIRED)
    include_directories (${LIBIPMID_INCLUDE_DIRS})
    link_directories (${LIBIPMID_LIBRARY_DIRS})

endif ()

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

#
# import OpenSSL (crypto)
find_package (OpenSSL REQUIRED)
include_directories (${OPENSSL_INCLUDE_DIR})

add_library (
    zsv305oemcmds SHARED appcommands.cpp chassishandler.cpp oemcommands.cpp storagecommands.cpp Utils.cpp
)
set_target_properties (zsv305oemcmds PROPERTIES VERSION "0.1.0")
set_target_properties (zsv305oemcmds PROPERTIES SOVERSION "0")
target_link_libraries (zsv305oemcmds stdc++fs)
target_link_libraries (zsv305oemcmds ipmid)
target_link_libraries (zsv305oemcmds sdbusplus)
target_link_libraries (zsv305oemcmds phosphor_logging)
target_link_libraries (zsv305oemcmds -luserlayer)
target_link_libraries (zsv305oemcmds -lchannellayer)
target_link_libraries (zsv305oemcmds ${OPENSSL_CRYPTO_LIBRARY})
target_link_libraries (zsv305oemcmds -lobmci2c)
target_link_libraries (zsv305oemcmds -lobmcsysfs)
target_link_libraries (zsv305oemcmds -lobmcjtag)
target_link_libraries (zsv305oemcmds -lgpiod)
target_link_libraries (zsv305oemcmds -lobmcdbus)

install (TARGETS zsv305oemcmds DESTINATION lib/ipmid-providers)
