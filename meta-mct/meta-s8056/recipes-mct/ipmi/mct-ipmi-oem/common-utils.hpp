#pragma once

#include <iostream>
#include <fstream>
#include <cstdio>
#include <gpiod.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <boost/algorithm/string.hpp>
#include <nlohmann/json.hpp>
#include <openbmc/libmisc.h>

#include <phosphor-logging/log.hpp>

#include <ipmid/api.hpp>
#include <ipmid/utils.hpp>

#include <xyz/openbmc_project/Control/Security/SpecialMode/server.hpp>

using namespace phosphor::logging;

namespace secCtrl = sdbusplus::xyz::openbmc_project::Control::Security::server;

static constexpr const char* fruConfig = "/usr/share/entity-manager/configurations/fru.json";

#define PROPERTY_INTERFACE "org.freedesktop.DBus.Properties"

namespace chassis
{
    static constexpr const char* busName = "xyz.openbmc_project.State.Chassis";
    static constexpr const char* path = "/xyz/openbmc_project/state/chassis0";
    static constexpr const char* interface = "xyz.openbmc_project.State.Chassis";
    static constexpr const char* request = "CurrentPowerState";
} // namespace chassis

namespace post
{
    static constexpr const char* busName = "xyz.openbmc_project.State.OperatingSystem";
    static constexpr const char* path = "/xyz/openbmc_project/state/os";
    static constexpr const char* interface = "xyz.openbmc_project.State.OperatingSystem.Status";
    static constexpr const char* request = "OperatingSystemState";
} // namespace post

namespace fsc
{
    static constexpr const char* busName = "xyz.openbmc_project.EntityManager";
    static constexpr const char* path = "/xyz/openbmc_project/inventory/system/board/s8033_Fan_Table/Pid_";
    static constexpr const char* interface = "xyz.openbmc_project.Configuration.Pid.Zone";
}

namespace sol
{
    static constexpr const char* busName = "xyz.openbmc_project.SolPatternSensor";
    static constexpr const char* path = "/xyz/openbmc_project/sensors/pattern/Pattern";
    static constexpr const char* interface = "xyz.openbmc_project.Sensor.Value";
    static constexpr const char* request = "Pattern";
} // namespace sol

namespace node
{
    static constexpr const char* busName = "xyz.openbmc_project.mct.node";
    static constexpr const char* path = "/xyz/openbmc_project/mct/node";
    static constexpr const char* interface = "xyz.openbmc_project.mct.node";
    static constexpr const char* request_node = "CurrentNode";
    static constexpr const char* request_nodeID = "CurrentNodeID";
} // namespace node

namespace obmcSpMode
{
    static constexpr const char* busName = "xyz.openbmc_project.SpecialMode";
    static constexpr const char* path = "/xyz/openbmc_project/security/special_mode";
    static constexpr const char* interface = "xyz.openbmc_project.Security.SpecialMode";
    static constexpr const char* prop = "SpecialMode";
} // namespace obmcSpMode

namespace common
{
static int getProperty(sdbusplus::bus::bus& bus, const std::string& path,
                       const std::string& property, double& value,
                       const std::string service,const std::string interface);

static void setProperty(sdbusplus::bus::bus& bus, const std::string& path,
                 const std::string& property, const double value);

int logSelEvent(std::string enrty, std::string path ,
                     uint8_t eventData0, uint8_t eventData1, uint8_t eventData2);

int execmd(char* cmd,char* result);

std::string intToHexString(int data);

int setBmcService(std::string service,std::string status, std::string setting);

bool isPowerOn();

bool hasBiosPost();

bool setAst2500Register(uint32_t address,uint32_t setRegister);

void setGpioOutput(std::string lineName, int setValue);

int getBaseboardFruAddress(uint8_t& bus, uint8_t& addr);

/** @brief Calculate zero checksum value.
 *  @param[in] data - Data to calculate checksum.
 *  @return - The zero checksum value.
 */
uint8_t calculateZeroChecksum(const std::vector<uint8_t>& data);

/** @brief Validate zero checksum.
 *  @param[in] data - Data to validate checksum.
 *  @return - Retrun true if valid zero checksum data, otherwise return false.
 */
bool validateZeroChecksum(const std::vector<uint8_t>& data);

int getCurrentNode(uint8_t& nodeId);

int getCurrentSpecialMode(secCtrl::SpecialMode::Modes& mode);

int setCurrentSpecialMode(secCtrl::SpecialMode::Modes mode);
} // namespace common