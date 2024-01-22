#pragma once

#include <iostream>
#include <unistd.h>
#include <boost/asio.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <openbmc/libmisc.h>
#include <gpiod.hpp>
#include <chrono>

boost::asio::io_service io;
std::shared_ptr<sdbusplus::asio::connection> systemBus;
std::shared_ptr<sdbusplus::asio::dbus_interface> iface;

static int biosStartTimeMs = 10000;
static boost::asio::steady_timer biosStartTimer(io);

static int biosEndTimesMs = 900000 + biosStartTimeMs;
static boost::asio::steady_timer biosEndTimer(io);

static auto biosStartTime = std::chrono::high_resolution_clock::now();
boost::system::error_code cancelErrorCode = boost::asio::error::operation_aborted;

static std::string postStartName = "POST_START";
static gpiod::line postStartLine;
static boost::asio::posix::stream_descriptor postStartEvent(io);

static std::string biosSelectName = "BIOS_SELECT";
static gpiod::line biosSelectLine;

uint8_t postStartValue;
int biosSelectValue;
static bool amdBistState = false;

enum biosBootFailCause
{
    NORMAL = 0,
    POST_START = 1, 
    BIOS_RECOVERY = 2, 
    FRB2_TIMEOUT = 3,
    POST_END = 4,
};

constexpr const char* PROPERTIES_INTERFACE = "org.freedesktop.DBus.Properties";

namespace dualBios
{
    static constexpr const char* busName = "xyz.openbmc_project.mct.DualBios";
    static constexpr const char* path = "/xyz/openbmc_project/mct/DualBios";
    static constexpr const char* interface = "xyz.openbmc_project.mct.DualBios";
    static constexpr const char* setCurrentBootBiosMethod = "SetCurrentBootBios";
    static constexpr const char* enableBiosStartTimerMethod = "enableBiosStartTimer";
    static constexpr const char* biosActionMethod = "BiosAction";
    static constexpr const char* biosSelectProp = "BiosSelect";
} // namespace dualBios

namespace chassis
{
    static constexpr const char* busName = "xyz.openbmc_project.State.Chassis";
    static constexpr const char* path = "/xyz/openbmc_project/state/chassis0";
    static constexpr const char* interface = "xyz.openbmc_project.State.Chassis";
    static constexpr const char* request = "CurrentPowerState";
} // namespace chassis

namespace host
{
static constexpr const char* busName = "xyz.openbmc_project.State.Host";
static constexpr const char* path = "/xyz/openbmc_project/state/host0";
static constexpr const char* interface = "xyz.openbmc_project.State.Host";
static constexpr const char* request = "RequestedHostTransition";
} // namespace host

namespace postComplete
{
static constexpr const char* busName = "xyz.openbmc_project.State.OperatingSystem";
static constexpr const char* path = "/xyz/openbmc_project/state/os";
static constexpr const char* interface = "xyz.openbmc_project.State.OperatingSystem.Status";
static constexpr const char* request = "OperatingSystemState";
} // namespace postComplete

namespace ipmi
{
static constexpr const char* busName = "xyz.openbmc_project.Logging.IPMI";
static constexpr const char* path = "/xyz/openbmc_project/Logging/IPMI";
static constexpr const char* interface = "xyz.openbmc_project.Logging.IPMI";
static constexpr const char* request = "IpmiSelAdd";
} // namespace ipmi

namespace post
{
static constexpr const char* busName = "xyz.openbmc_project.State.Boot.PostCode0";
static constexpr const char* path = "/xyz/openbmc_project/State/Boot/PostCode0";
static constexpr const char* interface = "xyz.openbmc_project.State.Boot.PostCode";
static constexpr const char* requestCurrentCycle = "CurrentBootCycleCount";
static constexpr const char* requestPostCode= "GetPostCodes";
} // namespace post

namespace amdMbistStatus
{
static constexpr const char* busName = "xyz.openbmc_project.Settings";
static constexpr const char* path = "/xyz/openbmc_project/oem/HostStatus";
static constexpr const char* interface = "xyz.openbmc_project.OEM.HostStatus";
static constexpr const char* requestMbistStatus = "AmdMbistStatus";
} // namespace amdMbistStatus
