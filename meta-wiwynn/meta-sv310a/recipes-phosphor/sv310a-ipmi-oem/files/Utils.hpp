/*
// Copyright (c) 2019 Wiwynn Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#pragma once

#include <systemd/sd-journal.h>

#include <phosphor-logging/log.hpp>
#include <sdbusplus/bus.hpp>

#include <string>
#include <vector>

using namespace phosphor::logging;

static constexpr auto propertiesInterface = "org.freedesktop.DBus.Properties";

static constexpr auto leakyBucketService = "xyz.openbmc_project.LeakyBucket";
static constexpr auto leakyBucketThresholdPath =
    "/xyz/openbmc_project/leaky_bucket/threshold";
static constexpr auto leakyBucketThresholdIntf =
    "xyz.openbmc_project.LeakyBucket.Threshold";

static constexpr auto leakyBucketDimmPathRoot =
    "/xyz/openbmc_project/leaky_bucket/dimm_slot/";
static constexpr auto leakyBucketEccErrorIntf =
    "xyz.openbmc_project.DimmEcc.Count";

static uint8_t tempDimmSensorNum;

// IPMI Completion Code define
enum sv310aIPMICompletionCode
{
    IPMI_CC_FILE_ERROR = 0x07,
    IPMI_CC_NOT_SUPPORTED_IN_PRESENT_STATE = 0xD5,
    IPMI_CC_PARAMETER_IS_ILLEGAL = 0xD6,
    IPMI_CC_DATA_NOT_AVAILABLE = 0x80,
};

using namespace std;

/**
 *  @brief Function of getting files in specified directory.
 *
 *  @param[in] dirPath - Specified absolute directory path.
 *  @param[in] regexStr - Regular expression for filename filter.
 *                        If this parameter is not given,
 *                        Return all filenames in the directory.
 *
 *  @return All filenames in the directory that match the refular expression.
 **/
vector<string> getDirFiles(string dirPath, string regexStr = ".*");

using DbusIntf = std::string;
using DbusService = std::string;
using DbusPath = std::string;
using DbusIntfList = std::vector<DbusIntf>;
using DbusSubTree = std::map<DbusPath, std::map<DbusService, DbusIntfList>>;

/**
 *  @brief Get dbus sub tree function.
 *
 *  @param[in] bus - The bus to register on.
 *  @param[in] pathRoot - The root of the tree.
 *                        Using "/" will search the whole tree
 *  @param[in] depth - The maximum depth of the tree past the root to search.
 *                     Use 0 to search all.
 *  @param[in] intf - An optional list of interfaces to constrain the search to.
 *
 *  @return - DbusSubTree, including object path, service, interfaces.
 *             map<DbusPath, map<DbusService, DbusIntfList>>
 **/
DbusSubTree getSubTree(sdbusplus::bus::bus& bus, const std::string& pathRoot,
                       int depth, const std::string& intf);

/**
 *  @brief Get DIMM configuration function.
 *  @param[in] dimmConfig - The vector to store dimm configuration.
 *  @param[in] path - The DIMM configuration path.
 *
 *  @return - Successful or Not
 **/
bool getDimmConfig(std::vector<uint8_t>& dimmConfig, const std::string& path);

static constexpr int GPIO_BASE = 792;

enum GPIO_DIRECTION : uint8_t
{
    GPIO_DIRECTION_IN = 0x0,
    GPIO_DIRECTION_OUT = 0x1,
};

enum GPIO_VALUE : uint8_t
{
    GPIO_VALUE_LOW = 0x0,
    GPIO_VALUE_HIGH = 0x1,
};

void msleep(int32_t msec);
int exportGPIO(int gpioNum);
int unexportGPIO(int gpioNum);
int getGPIOValue(int gpioNum, uint8_t& value);
int getGPIODirection(int gpioNum, uint8_t& direction);
int setGPIOValue(int gpioNum, uint8_t value);
int setGPIODirection(int gpioNum, uint8_t direction);

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

template <typename T>
bool setProperty(sdbusplus::bus::bus& bus, const std::string& service,
                 const std::string& path, const std::string& interface,
                 const std::string& property, const T& value)
{
    try
    {
        auto method = bus.new_method_call(service.c_str(), path.c_str(),
                                          propertiesInterface, "Set");
        method.append(interface, property, std::variant<T>(value));
        bus.call_noreply(method);
    }
    catch (const sdbusplus::exception_t& e)
    {
        log<level::ERR>("Failed to set property.",
                        entry("SERVICE=%s", service.c_str()),
                        entry("PATH=%s", path.c_str()),
                        entry("INTERFACE=%s", interface.c_str()),
                        entry("PROPERTY=%s", property.c_str()),
                        entry("ERROR=%s", e.what()));
        return false;
    }

    return true;
}

template <typename T>
bool getProperty(sdbusplus::bus::bus& bus, const std::string& service,
                 const std::string& path, const std::string& interface,
                 const std::string& property, T& value)
{
    try
    {
        auto method = bus.new_method_call(service.c_str(), path.c_str(),
                                          propertiesInterface, "Get");
        method.append(interface, property);
        auto reply = bus.call(method);

        std::variant<T> valueVariant;
        reply.read(valueVariant);

        value = std::get<T>(valueVariant);
    }
    catch (const sdbusplus::exception_t& e)
    {
        log<level::ERR>("Failed to get property.",
                        entry("SERVICE=%s", service.c_str()),
                        entry("PATH=%s", path.c_str()),
                        entry("INTERFACE=%s", interface.c_str()),
                        entry("PROPERTY=%s", property.c_str()),
                        entry("ERROR=%s", e.what()));
        return false;
    }
    catch (const std::bad_variant_access&)
    {
        log<level::ERR>("Failed to get property. Bad variant access");
        return false;
    }

    return true;
}

int getCurrentNode();
