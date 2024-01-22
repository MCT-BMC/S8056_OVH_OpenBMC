#pragma once

#include <iostream>
#include <variant>
#include <boost/asio/io_context.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

std::shared_ptr<sdbusplus::asio::connection> bus;
std::shared_ptr<sdbusplus::asio::dbus_interface> iface;

int faultLed = 0;

static constexpr const char* PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";
static constexpr const char* LED_ENABLE = "xyz.openbmc_project.Led.Physical.Action.On";
static constexpr const char* LED_DISABLE = "xyz.openbmc_project.Led.Physical.Action.Off";

namespace led
{
    static constexpr const char* busName = "xyz.openbmc_project.mct.led";
    static constexpr const char* path = "/xyz/openbmc_project/mct/led";
    static constexpr const char* interface = "xyz.openbmc_project.mct.led";
    static constexpr const char* faultLedprop = "faultLed";
    static constexpr const char* increaseFaultLedMethod = "increaseFaultLed";
    static constexpr const char* decreaseFaultLedMethod = "decreaseFaultLed";
} // namespace led

namespace sysLed
{
    static constexpr const char* busName = "xyz.openbmc_project.LED.Controller.sys_fault";
    static constexpr const char* path = "/xyz/openbmc_project/led/physical/sys_fault";
    static constexpr const char* interface = "xyz.openbmc_project.Led.Physical";
} // namespace sysLed

namespace hwLed
{
    static constexpr const char* busName = "xyz.openbmc_project.LED.Controller.hw_fault";
    static constexpr const char* path = "/xyz/openbmc_project/led/physical/hw_fault";
    static constexpr const char* interface = "xyz.openbmc_project.Led.Physical";
} // namespace hwLed