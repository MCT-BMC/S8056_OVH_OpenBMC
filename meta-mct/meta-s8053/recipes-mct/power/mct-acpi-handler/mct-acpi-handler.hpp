#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>


constexpr const char* PROPERTIES_INTERFACE = "org.freedesktop.DBus.Properties";

static constexpr char const* IPMI_SERVICE = "xyz.openbmc_project.Logging.IPMI";
static constexpr char const* IPMI_PATH = "/xyz/openbmc_project/Logging/IPMI";
static constexpr char const* IPMI_INTERFACE = "xyz.openbmc_project.Logging.IPMI";

static const std::string CHASSIS_STATE_PATH = "/xyz/openbmc_project/state/chassis0";
static const std::string CHASSIS_STATE_INTERFACE = "xyz.openbmc_project.State.Chassis";

static const std::string ACPI_SENSOR_PATH = "/xyz/openbmc_project/sensors/acpi/ACPI_POWER_STATE";