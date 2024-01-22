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

#include <sdbusplus/bus.hpp>

#include <string>
#include <vector>

static constexpr auto leakyBucketService = "xyz.openbmc_project.LeakyBucket";
static constexpr auto leakyBucketThresholdPath =
    "/xyz/openbmc_project/leaky_bucket/threshold";
static constexpr auto leakyBucketThresholdIntf =
    "xyz.openbmc_project.LeakyBucket.Threshold";

static constexpr auto leakyBucketDimmPathRoot =
    "/xyz/openbmc_project/leaky_bucket/dimm_slot/";
static constexpr auto leakyBucketEccErrorIntf =
    "xyz.openbmc_project.DimmEcc.Count";

// IPMI Completion Code define
enum sv305IPMICompletionCode
{
    IPMI_CC_FILE_ERROR = 0x07,
    IPMI_CC_NOT_SUPPORTED_IN_PRESENT_STATE = 0xD5,
    IPMI_CC_PARAMETER_IS_ILLEGAL = 0xD6,
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
