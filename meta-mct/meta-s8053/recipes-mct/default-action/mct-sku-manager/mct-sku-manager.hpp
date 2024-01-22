#pragma once

#include <iostream>
#include <boost/asio/io_context.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/asio/object_server.hpp>

extern "C" {
    #include <linux/i2c-dev.h>
    #include <i2c/smbus.h>
}

std::shared_ptr<sdbusplus::asio::connection> bus;
std::shared_ptr<sdbusplus::asio::dbus_interface> iface;

uint8_t currentSku = 1;

static constexpr const uint8_t BUS_ID = 0x02;
static constexpr const uint8_t SLAVE_ADDR = 0x50;
static constexpr const uint8_t OFFSET_MSB = 0x00;
static constexpr const uint8_t OFFSET_LSB = 0x0f;

static constexpr const char* PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";

namespace sku
{
    static constexpr const char* busName = "xyz.openbmc_project.mct.sku";
    static constexpr const char* path = "/xyz/openbmc_project/mct/sku";
    static constexpr const char* interface = "xyz.openbmc_project.mct.sku";
    static constexpr const char* currentSkuProp = "CurrentSku";
} // namespace sku