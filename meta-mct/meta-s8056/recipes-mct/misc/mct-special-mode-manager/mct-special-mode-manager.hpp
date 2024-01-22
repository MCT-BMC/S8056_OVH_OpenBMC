#pragma once

#include <iostream>
#include <boost/asio/io_context.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <xyz/openbmc_project/Control/Security/SpecialMode/server.hpp>

std::shared_ptr<sdbusplus::asio::connection> bus;
std::shared_ptr<sdbusplus::asio::dbus_interface> iface;

namespace secCtrl = sdbusplus::xyz::openbmc_project::Control::Security::server;

secCtrl::SpecialMode::Modes specialMode = secCtrl::SpecialMode::Modes::None;

static constexpr const char* PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";

namespace obmcSpMode
{
    static constexpr const char* busName = "xyz.openbmc_project.SpecialMode";
    static constexpr const char* path = "/xyz/openbmc_project/security/special_mode";
    static constexpr const char* interface = "xyz.openbmc_project.Security.SpecialMode";
    static constexpr const char* prop = "SpecialMode";
} // namespace obmcSpMode