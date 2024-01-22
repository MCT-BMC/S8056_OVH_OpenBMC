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

#include "Utils.hpp"
#include "xyz/openbmc_project/Common/error.hpp"

#include <openbmc/libobmci2c.h>
#include <systemd/sd-journal.h>

#include <appcommands.hpp>
#include <ipmid/api.hpp>
#include <ipmid/utils.hpp>
#include <nlohmann/json.hpp>
#include <openbmc/libsysfs.hpp>
#include <phosphor-logging/log.hpp>
#include <xyz/openbmc_project/Software/Activation/server.hpp>
#include <xyz/openbmc_project/Software/Version/server.hpp>
#include <xyz/openbmc_project/State/BMC/server.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

namespace Log = phosphor::logging;
namespace Error = sdbusplus::xyz::openbmc_project::Common::Error;
using Version = sdbusplus::xyz::openbmc_project::Software::server::Version;
using Activation =
    sdbusplus::xyz::openbmc_project::Software::server::Activation;
using BMC = sdbusplus::xyz::openbmc_project::State::server::BMC;

static constexpr auto redundancyIntf =
    "xyz.openbmc_project.Software.RedundancyPriority";
static constexpr auto versionIntf = "xyz.openbmc_project.Software.Version";
static constexpr auto activationIntf =
    "xyz.openbmc_project.Software.Activation";
static constexpr auto softwareRoot = "/xyz/openbmc_project/software";
constexpr auto bmc_state_interface = "xyz.openbmc_project.State.BMC";
constexpr auto bmc_state_property = "CurrentBMCState";

static void register_app_functions() __attribute__((constructor));

/*
    Get Self Test Result
    NetFn: App (0x6) / CMD: 0x4
*/
ipmi_ret_t ipmiGetSelfTestResult(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                 ipmi_request_t request,
                                 ipmi_response_t response,
                                 ipmi_data_len_t dataLen,
                                 ipmi_context_t context)
{
    int32_t reqDataLen = (int32_t)*dataLen;
    *dataLen = 0;

    /* Data Length - 0 */
    if (reqDataLen != 0)
    {
        sd_journal_print(LOG_ERR, "[%s] invalid cmd data length %d\n",
                         __FUNCTION__, reqDataLen);
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    GetSelfTestResultRes* resData =
        reinterpret_cast<GetSelfTestResultRes*>(response);

    *dataLen = sizeof(GetSelfTestResultRes);

    if (std::filesystem::exists("/run/bmc-ready"))
    {
        resData->data1 = 0x55; // No error
    }
    else
    {
        resData->data1 = 0x57; // Corrupted or inaccessible data or devices
    }

    resData->data2 = 0x00;

    return IPMI_CC_OK;
}

/**
 *  @brief Function of get device guid from eeprom.
 *  @brief NetFn: 0x06, Cmd: 0x08
 *
 *  @return 16 bytes guid.
 **/
ipmi_ret_t ipmiGetDevGUID(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                          ipmi_request_t request, ipmi_response_t response,
                          ipmi_data_len_t dataLen, ipmi_context_t context)
{
    try
    {
        auto data = readEeprom(mbFruIndex, deviceGuidOffset, guidLength);
        auto res = reinterpret_cast<OEMLibGetDeviceGuidResponse*>(response);
        std::copy(data.begin(), data.end(), res->guid);
    }
    catch (const std::exception& e)
    {
        sd_journal_print(LOG_ERR, "[%s] Failed to get guid %s(%s)\n",
                         __FUNCTION__, e.what(), strerror(errno));
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    *dataLen = guidLength;
    return IPMI_CC_OK;
}

/**
 *  @brief Function of get system guid from eeprom.
 *  @brief NetFn: 0x06, Cmd: 0x37
 *
 *  @return 16 bytes guid.
 **/
ipmi_ret_t ipmiGetSystemGUID(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                             ipmi_request_t request, ipmi_response_t response,
                             ipmi_data_len_t dataLen, ipmi_context_t context)
{
    try
    {
        auto data = readEeprom(mbFruIndex, systemGuidOffset, guidLength);
        auto res = reinterpret_cast<OEMLibGetSystemGuidResponse*>(response);
        std::copy(data.begin(), data.end(), res->guid);
    }
    catch (const std::exception& e)
    {
        sd_journal_print(LOG_ERR, "[%s] Failed to get guid %s(%s)\n",
                         __FUNCTION__, e.what(), strerror(errno));
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    *dataLen = guidLength;
    return IPMI_CC_OK;
}

typedef struct
{
    uint8_t major;
    uint8_t minor;
    uint32_t buildNo;
} BmcRevision;

bool getCurrentBmcState()
{
    sdbusplus::bus::bus bus{ipmid_get_sd_bus_connection()};

    // Get the Inventory object implementing the BMC interface
    ipmi::DbusObjectInfo bmcObject =
        ipmi::getDbusObject(bus, bmc_state_interface);
    auto variant =
        ipmi::getDbusProperty(bus, bmcObject.second, bmcObject.first,
                              bmc_state_interface, bmc_state_property);

    return std::holds_alternative<std::string>(variant) &&
           BMC::convertBMCStateFromString(std::get<std::string>(variant)) ==
               BMC::BMCState::Ready;
}

bool getCurrentBmcStateWithFallback(const bool fallbackAvailability)
{
    try
    {
        return getCurrentBmcState();
    }
    catch (...)
    {
        // Nothing provided the BMC interface, therefore return whatever was
        // configured as the default.
        return fallbackAvailability;
    }
}

std::string getActiveSoftwareVersionInfo()
{
    auto busp = getSdBus();
    std::string revision{};
    ipmi::ObjectTree objectTree;
    try
    {
        objectTree =
            ipmi::getAllDbusObjects(*busp, softwareRoot, redundancyIntf);
    }
    catch (sdbusplus::exception::SdBusError& e)
    {
        Log::log<Log::level::ERR>("Failed to fetch redundancy object from dbus",
                                  Log::entry("INTERFACE=%s", redundancyIntf),
                                  Log::entry("ERRMSG=%s", e.what()));
    }

    auto objectFound = false;
    for (auto& softObject : objectTree)
    {
        auto service =
            ipmi::getService(*busp, redundancyIntf, softObject.first);
        auto objValueTree =
            ipmi::getManagedObjects(*busp, service, softwareRoot);

        auto minPriority = 0xFF;
        for (const auto& objIter : objValueTree)
        {
            try
            {
                auto& intfMap = objIter.second;
                auto& redundancyPriorityProps = intfMap.at(redundancyIntf);
                auto& versionProps = intfMap.at(versionIntf);
                auto& activationProps = intfMap.at(activationIntf);
                auto priority =
                    std::get<uint8_t>(redundancyPriorityProps.at("Priority"));
                auto purpose =
                    std::get<std::string>(versionProps.at("Purpose"));
                auto activation =
                    std::get<std::string>(activationProps.at("Activation"));
                auto version =
                    std::get<std::string>(versionProps.at("Version"));
                if ((Version::convertVersionPurposeFromString(purpose) ==
                     Version::VersionPurpose::BMC) &&
                    (Activation::convertActivationsFromString(activation) ==
                     Activation::Activations::Active))
                {
                    if (priority < minPriority)
                    {
                        minPriority = priority;
                        objectFound = true;
                        revision = std::move(version);
                    }
                }
            }
            catch (const std::exception& e)
            {
                Log::log<Log::level::ERR>(e.what());
            }
        }
    }

    if (!objectFound)
    {
        Log::log<Log::level::ERR>("Could not found a BMC software Object");
    }

    return revision;
}

// Support OpenBMC version format: x.x.x (major.minor.auxiliary)
bool convertBMCVersion(std::string& s, BmcRevision& rev)
{
    std::smatch results;
    std::regex BmcVersionPattern("(\\d+?)\\.(\\d+?)\\.(\\d+?)");
    constexpr size_t bmcVersionSize = 4;
    if (std::regex_match(s, results, BmcVersionPattern))
    {
        if (results.size() == bmcVersionSize)
        {
            rev.major = static_cast<uint8_t>(std::stoi(results[1]));
            rev.minor = static_cast<uint8_t>(std::stoi(results[2]));
            rev.buildNo = static_cast<uint32_t>(std::stoi(results[3]));
            std::string versionString = std::to_string(rev.major) + ":" +
                                        std::to_string(rev.minor) + ":" +
                                        std::to_string(rev.buildNo);
            phosphor::logging::log<phosphor::logging::level::INFO>(
                versionString.c_str());
            return true;
        }
    }

    return false;
}

/*
    Get Device ID
    NetFn: App (0x6) / CMD: 0x01
*/
ipmi_ret_t ipmiGetDevID(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                        ipmi_request_t request, ipmi_response_t response,
                        ipmi_data_len_t dataLen, ipmi_context_t context)
{
    OEMLibGetDevIDResponse* res =
        reinterpret_cast<OEMLibGetDevIDResponse*>(response);
    int r = -1;
    BmcRevision rev = {0};
    static struct
    {
        uint8_t id;
        uint8_t revision;
        uint8_t fw[2];
        uint8_t ipmiVer;
        uint8_t addnDevSupport;
        uint24_t manufId;
        uint16_t prodId;
        uint32_t aux;
    } devId;
    static bool dev_id_initialized = false;
    static bool defaultActivationSetting = true;
    const char* filename = "/usr/share/ipmi-providers/dev_id.json";
    constexpr auto ipmiDevIdStateShift = 7;
    constexpr auto ipmiDevIdFw1Mask = ~(1 << ipmiDevIdStateShift);

    if (!dev_id_initialized)
    {
        try
        {
            auto version = getActiveSoftwareVersionInfo();
            r = convertBMCVersion(version, rev);
        }
        catch (const std::exception& e)
        {
            Log::log<Log::level::ERR>(e.what());
        }

        if (r >= 0)
        {
            // bit7 identifies if the device is available
            // 0=normal operation
            // 1=device firmware, SDR update,
            // or self-initialization in progress.
            // The availability may change in run time, so mask here
            // and initialize later.
            devId.fw[0] = rev.major & ipmiDevIdFw1Mask;

            rev.minor = (rev.minor > 99 ? 99 : rev.minor);
            devId.fw[1] = rev.minor % 10 + (rev.minor / 10) * 16;
            devId.aux = rev.buildNo;
        }

        // IPMI Spec version 2.0
        devId.ipmiVer = 2;

        std::ifstream devIdFile(filename);
        if (devIdFile.is_open())
        {
            auto data = nlohmann::json::parse(devIdFile, nullptr, false);
            if (!data.is_discarded())
            {
                devId.id = data.value("id", 0);
                devId.revision = data.value("revision", 0);
                devId.addnDevSupport = data.value("addn_dev_support", 0);
                devId.manufId = data.value("manuf_id", 0);
                devId.prodId = data.value("prod_id", 0);
                devId.aux = data.value("aux", 0);

                // Set the availablitity of the BMC.
                defaultActivationSetting = data.value("availability", true);

                // Don't read the file every time if successful
                dev_id_initialized = true;
            }
            else
            {
                Log::log<Log::level::ERR>("Device ID JSON parser failure");
                return IPMI_CC_UNSPECIFIED_ERROR;
            }
        }
        else
        {
            Log::log<Log::level::ERR>("Device ID file not found");
            return IPMI_CC_UNSPECIFIED_ERROR;
        }
    }

    // Set availability to the actual current BMC state
    devId.fw[0] &= ipmiDevIdFw1Mask;
    if (!getCurrentBmcStateWithFallback(defaultActivationSetting))
    {
        devId.fw[0] |= (1 << ipmiDevIdStateShift);
    }

    res->devId = devId.id;
    res->devRev = devId.revision;
    res->fwRev[0] = devId.fw[0];
    res->fwRev[1] = devId.fw[1];
    res->ipmiVer = devId.ipmiVer;
    res->addnDevSupport = devId.addnDevSupport;
    std::memcpy(res->manufId, &devId.manufId, sizeof(res->manufId));
    std::memcpy(res->prodId, &devId.prodId, sizeof(res->prodId));
    std::memcpy(res->auxInfo, &devId.aux, sizeof(res->auxInfo));
    *dataLen = 15;

    return IPMI_CC_OK;
}

static void register_app_functions(void)
{
    // < Get Self Test Result >
    ipmi_register_callback(ipmi::netFnApp, ipmi::app::cmdGetSelfTestResults,
                           NULL, ipmiGetSelfTestResult, PRIVILEGE_USER);

    // < Get Device Guid >
    ipmi_register_callback(ipmi::netFnApp, ipmi::app::cmdGetDeviceGuid, NULL,
                           ipmiGetDevGUID, PRIVILEGE_USER);

    // < Get System Guid >
    ipmi_register_callback(ipmi::netFnApp, ipmi::app::cmdGetSystemGuid, NULL,
                           ipmiGetSystemGUID, PRIVILEGE_USER);

    // <Get Device ID>
    ipmi_register_callback(ipmi::netFnApp, ipmi::app::cmdGetDeviceId, NULL,
                           ipmiGetDevID, PRIVILEGE_USER);
}
