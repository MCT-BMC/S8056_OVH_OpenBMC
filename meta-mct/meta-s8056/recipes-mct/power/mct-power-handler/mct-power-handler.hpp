#pragma once

#include <iostream>
#include <unistd.h>
#include <boost/asio.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>


constexpr const char* PROPERTIES_INTERFACE = "org.freedesktop.DBus.Properties";

namespace chassis
{
    static constexpr const char* busName = "xyz.openbmc_project.State.Chassis";
    static constexpr const char* path = "/xyz/openbmc_project/state/chassis0";
    static constexpr const char* interface = "xyz.openbmc_project.State.Chassis";
    static constexpr const char* request = "CurrentPowerState";
} // namespace chassis

namespace sensorStatus
{
    static constexpr const char* busName = "xyz.openbmc_project.Settings";
    static constexpr const char* path = "/xyz/openbmc_project/oem/SensorStatus";
    static constexpr const char* interface = "xyz.openbmc_project.OEM.SensorStatus";
} // namespace sensorStatus

namespace postComplete
{
static constexpr const char* busName = "xyz.openbmc_project.State.OperatingSystem";
static constexpr const char* path = "/xyz/openbmc_project/state/os";
static constexpr const char* interface = "xyz.openbmc_project.State.OperatingSystem.Status";
static constexpr const char* request = "OperatingSystemState";
} // namespace postComplete
