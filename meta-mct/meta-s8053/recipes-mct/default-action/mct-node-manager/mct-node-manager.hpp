#pragma once

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <boost/asio/io_context.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <gpiod.hpp>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>

extern "C" {
    #include <linux/i2c-dev.h>
    #include <i2c/smbus.h>
}

#define IPMB_NODE_BUS 10
#define LEFT_NODE_ADDR 0x18
#define RIGHT_NODE_ADDR 0x19
#define NODE_RESPONSE_TIMEOUT 300

// Node Status
#define NODE_PRESENT 1
#define NODE_HANGED 0
#define NODE_NOT_PRESENT -1

#define S8053_NON_GPU_SKU 0
#define S8053_GPU_SKU 1

std::string selloggerReadyFile = "/run/sel-logger-ready";
std::string frontRiserEeporm = "/sys/bus/i2c/devices/34-0056/eeprom";
std::string rearRiserEeporm = "/sys/bus/i2c/devices/36-0056/eeprom";

std::shared_ptr<sdbusplus::asio::connection> bus;
std::shared_ptr<sdbusplus::asio::dbus_interface> iface;

bool handlerReady = false;
bool isNodeHanged = false;
int8_t previousNodeState = 1; // Default: node present and alive
uint16_t NodeHangedRetry = 0;

uint8_t currentSku = 0;
std::string currentNode = "NA";
uint8_t currentNodeId = 0;
std::string anotherNodeState = "nodePresent"; // Default: node present and alive

static std::string gpioNameNode = "FM_NODE_ID";
static std::string gpioNameNodePresent = "FM_NODE_PRSNT";
gpiod::line gpioLineNode;

static constexpr const char* PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";
static constexpr char const* generalSensorPath = "/xyz/openbmc_project/sensors/board/";
static constexpr size_t selEvtDataMaxSize = 3;

namespace sel
{
    static constexpr const char* busName = "xyz.openbmc_project.Logging.IPMI";
    static constexpr const char* path = "/xyz/openbmc_project/Logging/IPMI";
    static constexpr const char* interface = "xyz.openbmc_project.Logging.IPMI";
} // namespace sel

namespace node
{
    static constexpr const char* busName = "xyz.openbmc_project.mct.node";
    static constexpr const char* path = "/xyz/openbmc_project/mct/node";
    static constexpr const char* interface = "xyz.openbmc_project.mct.node";
    static constexpr const char* currentNodeProp = "CurrentNode";
    static constexpr const char* currentNodeIdProp = "CurrentNodeID";
    static constexpr const char* anotherNodeStateProp = "AnotherNodeState";
} // namespace node