cmake_minimum_required(VERSION 3.6)
project(caterrHandler)
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (
    CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -lstdc++fs \
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
    "
  )

option (
    PROCHOT_ENABLE_POWER_FILTER
    "Enable PROCHOT event power filter."
    OFF
  )
option (
    THERMTRIP_ENABLE_POWER_FILTER
    "Enable THERMTRIP event power filter."
    OFF
  )
option (
    ENABLE_VR_POWER_FILTER
    "Enable VRHOT event power filter."
    OFF
  )
option (
    ENABLE_PROCHOT_ASSET
    "Enable PROCHOT GPIO asset function."
    OFF
  )

set (PROCHOT_THERMTRIP_LINK_LIBS -lsystemd stdc++fs sdbusplus -lgpiod -lobmcmisc)

add_executable (prochotThermtripHandler prochotThermtripHandler.cpp)
target_link_libraries(prochotThermtripHandler ${PROCHOT_THERMTRIP_LINK_LIBS})

install(TARGETS prochotThermtripHandler
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)

target_compile_definitions (
    prochotThermtripHandler PRIVATE $<$<BOOL:${PROCHOT_ENABLE_POWER_FILTER}>: -DPROCHOT_ENABLE_POWER_FILTER>
    $<$<BOOL:${THERMTRIP_ENABLE_POWER_FILTER}>: -DTHERMTRIP_ENABLE_POWER_FILTER>
    $<$<BOOL:${ENABLE_PROCHOT_ASSET}>: -DENABLE_PROCHOT_ASSET>
)

set (VRHOT_LINK_LIBS -lsystemd stdc++fs sdbusplus)

add_executable (vrHotHandler vrHotHandler.cpp)
target_link_libraries(vrHotHandler ${VRHOT_LINK_LIBS})

install(TARGETS vrHotHandler
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)

target_compile_definitions (
    vrHotHandler PRIVATE $<$<BOOL:${ENABLE_VR_POWER_FILTER}>: -DENABLE_VR_POWER_FILTER>
    $<$<BOOL:${ENABLE_PROCHOT_ASSET}>: -DENABLE_PROCHOT_ASSET>
)

set (PROCHOT_MANAGER_LINK_LIBS -lsystemd stdc++fs sdbusplus -lgpiodcxx -pthread)

add_executable (prochot-manager prochot-manager.cpp)
target_link_libraries(prochot-manager ${PROCHOT_MANAGER_LINK_LIBS})

install(TARGETS prochot-manager
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib/static)