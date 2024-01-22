#pragma once

#include <iostream>
#include <boost/asio/io_context.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <openbmc/libobmci2c.h>

std::shared_ptr<sdbusplus::asio::connection> bus;
std::shared_ptr<sdbusplus::asio::dbus_interface> iface;

static constexpr const char* PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";

namespace mbCpld
{
    uint8_t bus = 0x04;
    uint8_t address = 0x59;
} // namespace mbCpld

namespace dcScmCpld
{
    uint8_t bus = 0x09;
    uint8_t address = 0x59;
} // namespace dcScmCpld

enum cpldCommand
{
    CMD_BUTTON_CTL = 0x50,
};

enum button
{
    REG_BTN_UNLOCK = 0x00,
    REG_BTN_LOCK = 0x01,
};

namespace cpld
{
    static constexpr const char* busName = "xyz.openbmc_project.mct.cpld";
    static constexpr const char* path = "/xyz/openbmc_project/mct/cpld";
    static constexpr const char* interface = "xyz.openbmc_project.mct.cpld";
    static constexpr const char* buttonLockMethod = "buttonLockMethod";
    static constexpr const char* buttonUnlockMethod = "buttonUnlockMethod";
} // namespace cpld