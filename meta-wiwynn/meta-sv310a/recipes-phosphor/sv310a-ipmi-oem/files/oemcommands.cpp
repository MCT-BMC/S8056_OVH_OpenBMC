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

#include "oemcommands.hpp"

#include "openbmc/libobmci2c.h"

#include "Utils.hpp"
#include "openbmc/libobmccpld.hpp"
#include "openbmc/libobmcdbus.hpp"
#include "xyz/openbmc_project/Common/error.hpp"
#include "xyz/openbmc_project/Control/Power/RestorePolicy/server.hpp"
#include "xyz/openbmc_project/Led/Physical/server.hpp"

#include <gpiod.h>
#include <systemd/sd-journal.h>

#include <boost/container/flat_map.hpp>
#include <boost/process/child.hpp>
#include <boost/process/io.hpp>
#include <ipmid/utils.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message/types.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdbusplus/timer.hpp>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <variant>
#include <vector>

const static constexpr char* solPatternService =
    "xyz.openbmc_project.SolPatternSensor";
const static constexpr char* solPatternInterface =
    "xyz.openbmc_project.Sensor.SOLPattern";
const static constexpr char* solPatternObjPrefix =
    "/xyz/openbmc_project/sensors/hit/pattern_";

const static constexpr char* propertyInterface =
    "org.freedesktop.DBus.Properties";

const static constexpr char* timeSyncMethodPath =
    "/xyz/openbmc_project/time/sync_method";
const static constexpr char* timeSyncMethodIntf =
    "xyz.openbmc_project.Time.Synchronization";

const static constexpr char* systemdTimeService = "org.freedesktop.timedate1";
const static constexpr char* systemdTimePath = "/org/freedesktop/timedate1";
const static constexpr char* systemdTimeInterface = "org.freedesktop.timedate1";

const static constexpr int cpuSocketID = 0;
constexpr auto cpuPwrCtrlService = "xyz.openbmc_project.AMD_CPU.Control";
constexpr auto cpuPwrCtrlPath = "/xyz/openbmc_project/AMD_CPU/Control";
constexpr auto parameterIntf = "xyz.openbmc_project.AMD_CPU.Control.Parameters";
constexpr auto systemPowerLimitProperty = "systemPowerLimit";
constexpr auto operationalStatusIntf =
    "xyz.openbmc_project.PowerCapping.OperationalStatus";
constexpr auto autoPowerCapProperty = "autoPowerCap";
constexpr auto manualPowerCapProperty = "manualPowerCap";
constexpr auto cpuPwrCtrlInterface = "xyz.openbmc_project.PowerStatus.Control";
constexpr auto setDefaultCpuPowerLimit = "SetDefaultCPUPowerLimit";
constexpr auto setSystemPowerLimit = "SetSystemPowerLimit";
constexpr auto setCpuPowerLimit = "SetCPUPowerLimit";
constexpr auto getCpuPowerLimit = "GetCPUPowerLimit";
constexpr auto getCpuPowerConsumption = "GetCPUPowerConsumption";

static constexpr auto FanModeService = "xyz.openbmc_project.State.FanCtrl";
static constexpr auto FanModePathRoot =
    "/xyz/openbmc_project/settings/fanctrl/";
static constexpr auto FanModeInterface = "xyz.openbmc_project.Control.Mode";

static void register_oem_functions() __attribute__((constructor));

/**
 *  @brief Function of GPIO Set
 *  @brief NetFn: 0x30, Cmd: 0x17
 *
 *  @param[in] GPIO Number
 *  @param[in] Direction
 *  @param[in] Value
 *
 *  @return Byte 1: Completion Code.
 **/
ipmi_ret_t IpmiSetGpio(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                       ipmi_request_t request, ipmi_response_t response,
                       ipmi_data_len_t data_len, ipmi_context_t context)
{
    auto req = reinterpret_cast<SetGpioReq*>(request);

    // If set direction to input, value can be ignored.
    if (req->direction == GPIO_DIRECTION_IN)
    {
        if (*data_len != sizeof(SetGpioReq) &&
            *data_len != sizeof(SetGpioReq) - 1)
        {
            sd_journal_print(LOG_ERR, "[%s] invalid data length %d\n",
                             __FUNCTION__, *data_len);
            return IPMI_CC_REQ_DATA_LEN_INVALID;
        }
    }
    else if (req->direction == GPIO_DIRECTION_OUT)
    {
        if (*data_len != sizeof(SetGpioReq))
        {
            sd_journal_print(LOG_ERR, "[%s] invalid data length %d\n",
                             __FUNCTION__, *data_len);
            return IPMI_CC_REQ_DATA_LEN_INVALID;
        }
        if (req->value != GPIO_VALUE_LOW && req->value != GPIO_VALUE_HIGH)
        {
            sd_journal_print(LOG_ERR, "[%s] invalid gpio value %d\n",
                             __FUNCTION__, req->value);
            return IPMI_CC_PARM_OUT_OF_RANGE;
        }
    }
    else
    {
        sd_journal_print(LOG_ERR, "[%s] invalid gpio direction %d\n",
                         __FUNCTION__, req->direction);
        return IPMI_CC_PARM_OUT_OF_RANGE;
    }

    int ret = -1;
    ret = exportGPIO(GPIO_BASE + req->gpioNum);
    if (ret < 0)
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    ret = setGPIODirection(GPIO_BASE + req->gpioNum, req->direction);
    if (ret < 0)
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    if (req->direction == GPIO_DIRECTION_OUT)
    {
        ret = setGPIOValue(GPIO_BASE + req->gpioNum, req->value);
        if (ret < 0)
        {
            return IPMI_CC_UNSPECIFIED_ERROR;
        }
    }

    ret = unexportGPIO(GPIO_BASE + req->gpioNum);
    if (ret < 0)
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    *data_len = 0;
    return IPMI_CC_OK;
}

/**
 *  @brief Function of GPIO Get
 *  @brief NetFn: 0x30, Cmd: 0x18
 *
 *  @param[in] GPIO Number
 *
 *  @return Byte 1: Completion Code
 *          Byte 2: Direction
 *          Byte 3: Value
 **/
ipmi_ret_t IpmiGetGpio(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                       ipmi_request_t request, ipmi_response_t response,
                       ipmi_data_len_t data_len, ipmi_context_t context)
{
    if (*data_len != sizeof(GetGpioReq))
    {
        sd_journal_print(LOG_ERR, "[%s] invalid data length %d\n", __FUNCTION__,
                         *data_len);
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    auto req = reinterpret_cast<GetGpioReq*>(request);
    auto res = reinterpret_cast<GetGpioRes*>(response);

    int ret = -1;
    ret = exportGPIO(GPIO_BASE + req->gpioNum);
    if (ret < 0)
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    ret = getGPIODirection(GPIO_BASE + req->gpioNum, res->direction);
    if (ret < 0)
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    ret = getGPIOValue(GPIO_BASE + req->gpioNum, res->value);
    if (ret < 0)
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    ret = unexportGPIO(GPIO_BASE + req->gpioNum);
    if (ret < 0)
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    *data_len = sizeof(GetGpioRes);
    return IPMI_CC_OK;
}

static ipmi::ObjectValueTree getFanControlManagedObjects()
{
    // To remove the last slash.
    std::string fanModePath = FanModePathRoot;
    fanModePath.pop_back();

    ipmi::ObjectValueTree objects;
    try
    {
        auto bus = getSdBus();
        auto msg = bus->new_method_call(FanModeService, fanModePath.c_str(),
                                        "org.freedesktop.DBus.ObjectManager",
                                        "GetManagedObjects");
        auto reply = bus->call(msg);
        reply.read(objects);
    }
    catch (const sdbusplus::exception_t& e)
    {
        log<level::ERR>("Failed to get fan zone D-Bus managed objects",
                        entry("ERROR=%s", e.what()));
    }

    return objects;
}

/**
 *  @brief Function of setting fan speed duty.
 *  @brief NetFn: 0x30, Cmd: 0x11
 *
 *  @param[in] pwmIndex - Index of PWM.
 *  @param[in] pwmValue - PWM value for the specified index.
 *
 *  @return Size of command response - Completion Code.
 **/
ipmi::RspType<> IpmiSetPwm(uint8_t pwmId, uint8_t pwmValue)
{
    if ((pwmValue > 100) || (pwmValue < 10))
    {
        log<level::ERR>("Cannot set pwm value over 100% or below 10%");
        return ipmi::responseInvalidFieldRequest();
    }

    if ((pwmId != 0xFF) && ((getCurrentNode() == 1 && pwmId > 2) ||
                            (getCurrentNode() == 0 && pwmId <= 2)))
    {
        return ipmi::responseInvalidFieldRequest();
    }
    else if (getCurrentNode() == -1)
    {
        return ipmi::responseUnspecifiedError();
    }

    auto objects = getFanControlManagedObjects();
    if (objects.empty())
    {
        log<level::ERR>("Fan control D-Bus is empty");
        return ipmi::responseUnspecifiedError();
    }

    // The pair first is D-Bus zone path, second is zone manual file path.
    std::vector<std::tuple<std::string, std::string>> pathPairs;
    // Check fan zone is in manual mode
    for (const auto& [path, interfaces] : objects)
    {
        for (const auto& [interface, properties] : interfaces)
        {
            if (interface != FanModeInterface)
            {
                continue;
            }

            auto findManual = properties.find("Manual");
            if (findManual == properties.end())
            {
                log<level::ERR>("Failed to find manual property");
                return ipmi::responseUnspecifiedError();
            }
            if (!std::get<bool>(findManual->second))
            {
                log<level::ERR>(
                    "Fan zone should be in manual mode before set pwm");
                return ipmi::responseCommandNotAvailable();
            }

            auto manualFilePath = manualModeFilePath + path.filename();
            if (!std::filesystem::exists(manualFilePath))
            {
                log<level::ERR>("Fan manual file doesn't exist.",
                                entry("PATH=%s", manualFilePath.c_str()));
                return ipmi::responseUnspecifiedError();
            }

            pathPairs.emplace_back(std::make_tuple(path.str, manualFilePath));
        }
    }
    if (pathPairs.empty())
    {
        log<level::ERR>("Empty zone in fan control D-Bus");
        return ipmi::responseInvalidFieldRequest();
    }

    // For checking whether the pwm ID exists.
    bool isPwmIdFound = false;
    for (const auto& [dbusPath, manualFilePath] : pathPairs)
    {
        std::ifstream finManual(manualFilePath);
        if (!finManual.is_open())
        {
            log<level::ERR>("Failed to open fan manual file.",
                            entry("PATH=%s", manualFilePath.c_str()));
            return ipmi::responseUnspecifiedError();
        }

        bool isChange = false;
        // For storing new fan manual file data.
        std::stringstream ss;
        std::string line;
        while (std::getline(finManual, line))
        {
            std::vector<std::string> data;
            boost::split(data, line, boost::is_any_of(" "));
            if (data.size() != 3)
            {
                finManual.close();
                log<level::ERR>("Invalid format in fan manual file.",
                                entry("PATH=%s", manualFilePath.c_str()));
                return ipmi::responseUnspecifiedError();
            }
            int64_t pwmNumber = std::stoll(data[0]);
            std::string sensorName = data[1];

            if ((pwmId == 0xFF) || (pwmNumber == static_cast<int64_t>(pwmId)))
            {
                isChange = true;
                isPwmIdFound = true;
                if (!setProperty(
                        *getSdBus(), FanModeService, dbusPath, FanModeInterface,
                        "SetPwm",
                        std::tuple<std::string, double>(sensorName, pwmValue)))
                {
                    finManual.close();
                    log<level::ERR>("Failed to set pwm",
                                    entry("PWM_NUMBER=%u", pwmNumber));
                    return ipmi::responseResponseError();
                }

                ss << pwmNumber << " " << sensorName << " "
                   << static_cast<int64_t>(pwmValue) << "\n";
            }
            else
            {
                ss << pwmNumber << " " << sensorName << " ";
                if (data[2] == "nan")
                {
                    ss << "nan";
                }
                else
                {
                    ss << std::stoll(data[2]);
                }
                ss << "\n";
            }
        }
        finManual.close();

        // This zone has no changes.
        if (!isChange)
        {
            continue;
        }

        // Write new fan manual file data.
        std::ofstream foutManual(manualFilePath);
        if (!foutManual.is_open())
        {
            log<level::ERR>("Failed to open fan manual file.",
                            entry("PATH=%s", manualFilePath.c_str()));
            return ipmi::responseUnspecifiedError();
        }

        foutManual << ss.str();
        if (foutManual.fail())
        {
            foutManual.close();
            log<level::ERR>("Failed to write fan manual file.",
                            entry("PATH=%s", manualFilePath.c_str()));
            return ipmi::responseUnspecifiedError();
        }
        foutManual.close();
    }

    if (!isPwmIdFound)
    {
        log<level::ERR>("Given pwmId not found in fan zone.",
                        entry("PWM_ID=%u", pwmId));
        return ipmi::responseInvalidFieldRequest();
    }

    return ipmi::responseSuccess();
}


/**
 *  @brief Function of setting fan speed control mode.
 *  @brief NetFn: 0x30, Cmd: 0x12
 *
 *  @param[in] ControlMode - Manual mode or auto mode.
 *
 *  @return Byte 1: Completion Code.
 **/
ipmi::RspType<> IpmiSetFscMode(uint8_t mode)
{
    static constexpr uint8_t AutoMode = 0;
    static constexpr uint8_t ManualMode = 1;
    if ((mode != AutoMode) && (mode != ManualMode))
    {
        return ipmi::responseInvalidFieldRequest();
    }

    auto objects = getFanControlManagedObjects();
    if (objects.empty())
    {
        log<level::ERR>("Fan control D-Bus is empty");
        return ipmi::responseUnspecifiedError();
    }

    for (const auto& [path, interfaces] : objects)
    {
        if (!setProperty(*getSdBus(), FanModeService, path.str,
                         FanModeInterface, "Manual", static_cast<bool>(mode)))
        {
            return ipmi::responseResponseError();
        }
    }

    return ipmi::responseSuccess();
}

/**
 *  @brief Function of setting leaky bucket threshold
 *  @brief NetFn: 0x3E, Cmd: 0xB4
 *
 *  @param[in] Threshold number - The number of threshold
 *  @param[in] Value (LSB) - The least significant bit value of threshold
 *  @param[in] Value (MSB) - The most significant bit value of threshold
 *
 *  @return Byte 1: Completion Code.
 **/
ipmi_ret_t ipmiSetLBAThreshold(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                               ipmi_request_t request, ipmi_response_t response,
                               ipmi_data_len_t dataLen, ipmi_context_t context)
{
    if (*dataLen != sizeof(SetLBAThresholdRequest))
    {
        sd_journal_print(LOG_ERR, "[%s] invalid data length %d\n", __FUNCTION__,
                         *dataLen);
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    auto req = reinterpret_cast<SetLBAThresholdRequest*>(request);

    if (req->thresholdNumber < 1 || req->thresholdNumber > maxLBAThresholdNum)
    {
        sd_journal_print(LOG_ERR, "[%s] invalid threshold number %d\n",
                         __FUNCTION__, req->thresholdNumber);
        return IPMI_CC_PARM_OUT_OF_RANGE;
    }

    // T1 & T3 MUST NOT BE 0
    if ((req->thresholdValue == 0) &&
        (req->thresholdNumber == 1 || req->thresholdNumber == 3))
    {
        sd_journal_print(LOG_ERR, "[%s] invalid T%d value %d\n", __FUNCTION__,
                         req->thresholdNumber, req->thresholdValue);
        return IPMI_CC_PARM_OUT_OF_RANGE;
    }

    try
    {
        std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
        auto thresholdPropName =
            "Threshold" + std::to_string(req->thresholdNumber);
        ipmi::setDbusProperty(
            *bus, leakyBucketService, leakyBucketThresholdPath,
            leakyBucketThresholdIntf, thresholdPropName.c_str(),
            static_cast<uint16_t>(req->thresholdValue));
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        sd_journal_print(LOG_ERR, "[%s] failed to set property %s\n",
                         __FUNCTION__, e.what());
        return IPMI_CC_RESPONSE_ERROR;
    }

    *dataLen = 0;
    return IPMI_CC_OK;
}

/**
 *  @brief Function of getting leaky bucket threshold
 *  @brief NetFn: 0x3E, Cmd: 0xB5
 *
 *  @param[in] Threshold number - The number of threshold
 *
 *  @return Byte 1: Completion Code.
 *          Byte 2: Threshold value (LSB)
 *          Byte 3: Threshold value (MSB)
 **/
ipmi_ret_t ipmiGetLBAThreshold(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                               ipmi_request_t request, ipmi_response_t response,
                               ipmi_data_len_t dataLen, ipmi_context_t context)
{
    if (*dataLen != sizeof(GetLBAThresholdRequest))
    {
        sd_journal_print(LOG_ERR, "[%s] invalid data length %d\n", __FUNCTION__,
                         *dataLen);
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    auto req = reinterpret_cast<GetLBAThresholdRequest*>(request);
    auto res = reinterpret_cast<GetLBAThresholdResponse*>(response);

    if (req->thresholdNumber < 1 || req->thresholdNumber > maxLBAThresholdNum)
    {
        sd_journal_print(LOG_ERR, "[%s] invalid threshold number %d\n",
                         __FUNCTION__, req->thresholdNumber);
        return IPMI_CC_PARM_OUT_OF_RANGE;
    }

    try
    {
        std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
        auto thresholdPropName =
            "Threshold" + std::to_string(req->thresholdNumber);
        auto value = ipmi::getDbusProperty(
            *bus, leakyBucketService, leakyBucketThresholdPath,
            leakyBucketThresholdIntf, thresholdPropName.c_str());
        res->thresholdValue = std::get<uint16_t>(value);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        sd_journal_print(LOG_ERR, "[%s] failed to get property %s\n",
                         __FUNCTION__, e.what());
        return IPMI_CC_RESPONSE_ERROR;
    }

    *dataLen = sizeof(GetLBAThresholdResponse);
    return IPMI_CC_OK;
}

static std::vector<uint8_t> dimmConfig; // DIMM configuration
/**
 *  @brief Function of getting total correctable ECC counter value
 *  @brief NetFn: 0x3E, Cmd: 0xB6
 *
 *  @param[in] DIMM Number - DIMM Index
 *
 *  @return Byte 1: Completion Code.
 *          Byte 2: Total correctable ECC counter value byte 1 (LSB)
 *          Byte 3: Total correctable ECC counter value byte 2
 *          Byte 4: Total correctable ECC counter value byte 3
 *          Byte 5: Total correctable ECC counter value byte 4 (MSB)
 **/
ipmi_ret_t ipmiGetLBATotalCounter(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                  ipmi_request_t request,
                                  ipmi_response_t response,
                                  ipmi_data_len_t dataLen,
                                  ipmi_context_t context)
{
    if (*dataLen != sizeof(GetLBATotalCounterRequest))
    {
        sd_journal_print(LOG_ERR, "[%s] invalid data length %d\n", __FUNCTION__,
                         *dataLen);
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    if (getDimmConfig(dimmConfig, leckyBucketDimmConfigPath) == false)
    {
        sd_journal_print(LOG_ERR, "[%s] failed to get DIMM configuration\n",
                         __FUNCTION__);
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    auto req = reinterpret_cast<GetLBATotalCounterRequest*>(request);
    auto res = reinterpret_cast<GetLBATotalCounterResponse*>(response);

    if (req->dimmIndex == 0 || req->dimmIndex > dimmConfig.size())
    {
        sd_journal_print(LOG_ERR, "[%s] invalid DIMM number %d\n", __FUNCTION__,
                         req->dimmIndex);
        return IPMI_CC_PARM_OUT_OF_RANGE;
    }

    try
    {
        uint8_t dimmNumber = dimmConfig.at(req->dimmIndex - 1);
        std::string dimmPath =
            leakyBucketDimmPathRoot + std::to_string(dimmNumber);
        std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
        auto value =
            ipmi::getDbusProperty(*bus, leakyBucketService, dimmPath,
                                  leakyBucketEccErrorIntf, "TotalEccCount");
        res->totalCounter = std::get<uint32_t>(value);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        sd_journal_print(LOG_ERR, "[%s] failed to get property %s\n",
                         __FUNCTION__, e.what());
        return IPMI_CC_RESPONSE_ERROR;
    }

    *dataLen = sizeof(GetLBATotalCounterResponse);
    return IPMI_CC_OK;
}

/**
 *  @brief Function of getting relative correctable ECC counter value
 *  @brief NetFn: 0x3E, Cmd: 0xB7
 *
 *  @param[in] DIMM number - DIMM Index
 *
 *  @return Byte 1: Completion Code.
 *          Byte 2: Relative correctable ECC counter value byte 1 (LSB)
 *          Byte 3: Relative correctable ECC counter value byte 2
 *          Byte 4: Relative correctable ECC counter value byte 3
 *          Byte 5: Relative correctable ECC counter value byte 4 (MSB)
 **/
ipmi_ret_t ipmiGetLBARelativeCounter(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                     ipmi_request_t request,
                                     ipmi_response_t response,
                                     ipmi_data_len_t dataLen,
                                     ipmi_context_t context)
{
    if (*dataLen != sizeof(GetLBATotalCounterRequest))
    {
        sd_journal_print(LOG_ERR, "[%s] invalid data length %d\n", __FUNCTION__,
                         *dataLen);
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    if (getDimmConfig(dimmConfig, leckyBucketDimmConfigPath) == false)
    {
        sd_journal_print(LOG_ERR, "[%s] failed to get DIMM configuration\n",
                         __FUNCTION__);
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    auto req = reinterpret_cast<GetLBARelativeCounterRequest*>(request);
    auto res = reinterpret_cast<GetLBARelativeCounterResonse*>(response);

    if (req->dimmIndex == 0 || req->dimmIndex > dimmConfig.size())
    {
        sd_journal_print(LOG_ERR, "[%s] invalid DIMM number %d\n", __FUNCTION__,
                         req->dimmIndex);
        return IPMI_CC_PARM_OUT_OF_RANGE;
    }

    try
    {
        uint8_t dimmNumber = dimmConfig.at(req->dimmIndex - 1);
        std::string dimmPath =
            leakyBucketDimmPathRoot + std::to_string(dimmNumber);
        std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
        auto value =
            ipmi::getDbusProperty(*bus, leakyBucketService, dimmPath,
                                  leakyBucketEccErrorIntf, "RelativeEccCount");
        res->relativeCounter = std::get<uint32_t>(value);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        sd_journal_print(LOG_ERR, "[%s] failed to get property %s\n",
                         __FUNCTION__, e.what());
        return IPMI_CC_RESPONSE_ERROR;
    }

    *dataLen = sizeof(GetLBARelativeCounterResonse);
    return IPMI_CC_OK;
}

/**
 *  @brief Function of random power on status
 *  @brief NetFn: 0x2e, Cmd: 0x15
 *
 *  @return - Byte 1: Completion Code
 *          - Byte 2: Random power on minutes
 **/
ipmi_ret_t IpmiRandomPowerOnStatus(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                   ipmi_request_t request,
                                   ipmi_response_t response,
                                   ipmi_data_len_t dataLen,
                                   ipmi_context_t context)
{
    using namespace sdbusplus::xyz::openbmc_project::Control::Power::server;
    if (*dataLen != 0)
    {
        sd_journal_print(LOG_ERR, "[%s] invalid data length %d\n", __FUNCTION__,
                         *dataLen);
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    std::string delayPolicy;
    try
    {
        std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
        auto value = ipmi::getDbusProperty(
            *dbus, settingMgtService, powerRestorePolicyPath,
            powerRestorePolicyIntf, "PowerRestoreDelay");
        delayPolicy = std::get<std::string>(value);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        sd_journal_print(LOG_ERR, "[%s] failed to get property, %s",
                         __FUNCTION__, e.what());
        return IPMI_CC_RESPONSE_ERROR;
    }

    try
    {
        auto res = reinterpret_cast<GetRandomPowerOnStatusResonse*>(response);
        auto delay = RestorePolicy::convertDelayFromString(delayPolicy);
        res->delayMin = static_cast<uint8_t>(delay);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        sd_journal_print(LOG_ERR, "[%s] failed to convert string to delay, %s",
                         __FUNCTION__, e.what());
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    *dataLen = sizeof(GetRandomPowerOnStatusResonse);
    return IPMI_CC_OK;
}

/**
 *  @brief Function of random power on switch
 *  @brief NetFn: 0x2e, Cmd: 0x16
 *
 *  @param[In]: Delay policy
 *
 *  @return - Byte 1: Completion Code
 **/
ipmi_ret_t IpmiRandomPowerOnSwitch(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                   ipmi_request_t request,
                                   ipmi_response_t response,
                                   ipmi_data_len_t dataLen,
                                   ipmi_context_t context)
{
    using namespace sdbusplus::xyz::openbmc_project::Control::Power::server;

    if (*dataLen != sizeof(SetRandomPowerOnSwitchRequest))
    {
        sd_journal_print(LOG_ERR, "[%s] invalid data length %d\n", __FUNCTION__,
                         *dataLen);
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    auto req = reinterpret_cast<SetRandomPowerOnSwitchRequest*>(request);

    if ((req->delayMin != 0x00) && (req->delayMin != 0x01) &&
        (req->delayMin != 0x03))
    {
        sd_journal_print(LOG_ERR, "[%s] invalid delay policy %d", __FUNCTION__,
                         req->delayMin);
        return IPMI_CC_PARM_OUT_OF_RANGE;
    }

    std::string delayPolicy;
    try
    {
        auto delay = static_cast<RestorePolicy::Delay>(req->delayMin);
        delayPolicy = RestorePolicy::convertDelayToString(delay);
    }
    catch (const std::exception& e)
    {
        sd_journal_print(LOG_ERR, "[%s] failed to convert delay policy %d",
                         __FUNCTION__, req->delayMin);
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    try
    {
        std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
        ipmi::setDbusProperty(*dbus, settingMgtService, powerRestorePolicyPath,
                              powerRestorePolicyIntf, "PowerRestoreDelay",
                              delayPolicy);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        sd_journal_print(LOG_ERR, "[%s] failed to set property, %s",
                         __FUNCTION__, e.what());
        return IPMI_CC_RESPONSE_ERROR;
    }

    *dataLen = 0;
    return IPMI_CC_OK;
}

/**
 *  @brief Get BIOS post end status
 *  @brief NetFn: 0x30, Cmd: 0xaa
 *
 *  @return - Byte 1: Completion Code
 *          - Byte 2: Status
 **/
ipmi_ret_t IpmiGetPostEndStatus(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                ipmi_request_t request,
                                ipmi_response_t response,
                                ipmi_data_len_t dataLen, ipmi_context_t context)
{
    if (*dataLen != 0)
    {
        sd_journal_print(LOG_ERR,
                         "IPMI GetPostEndStatus request data len invalid, "
                         "received: %d, required: %d\n",
                         *dataLen, 0);
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    GetPostEndStatusRes* resData =
        reinterpret_cast<GetPostEndStatusRes*>(response);

    constexpr auto postcmplInterface = "org.openbmc.control.PostComplete";
    constexpr auto pgoodInterface = "org.openbmc.control.Power";
    constexpr auto powerObjPath = "/org/openbmc/control/power0";
    constexpr auto powerService = "org.openbmc.control.Power";

    static bool pgood = false;
    static bool postcomplete = false;

    std::shared_ptr<sdbusplus::asio::connection> systemBus = getSdBus();
    try
    {
        /* Get pgood property */
        auto powerDbus = systemBus->new_method_call(powerService, powerObjPath,
                                                    propertyInterface, "Get");
        powerDbus.append(pgoodInterface, "pgood");

        std::variant<int> state;
        auto reply = systemBus->call(powerDbus);
        reply.read(state);
        pgood = (gpioHi == std::get<int>(state)) ? true : false;

        /* Get postcomplete property */
        auto postDbus = systemBus->new_method_call(powerService, powerObjPath,
                                                   propertyInterface, "Get");

        postDbus.append(postcmplInterface, "postcomplete");

        reply = systemBus->call(postDbus);
        reply.read(state);
        postcomplete = (gpioLo == std::get<int>(state)) ? true : false;
    }
    catch (sdbusplus::exception_t& e)
    {
        sd_journal_print(LOG_ERR,
                         "[%s] Unable to get pgood/postcomplete property\n",
                         __FUNCTION__);
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    if (pgood && postcomplete)
    {
        resData->postEndStatus = BiosPostEnd;
    }
    else if (pgood && !postcomplete)
    {
        resData->postEndStatus = DuringBiosPost;
    }
    else
    {
        resData->postEndStatus = PowerOff;
    }
    *dataLen = 1;

    return IPMI_CC_OK;
}

/**
 *  @brief Get the latest 20 BIOS post codes
 *  @brief NetFn: 0x30, Cmd: 0x10
 *
 *  @return Size of command response
 *          - Completion Code, 20 post codes
 **/
ipmi_ret_t IpmiGetPostCode(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                           ipmi_request_t request, ipmi_response_t response,
                           ipmi_data_len_t dataLen, ipmi_context_t context)
{
    if (*dataLen != sizeof(GetPostCodeReq))
    {
        sd_journal_print(LOG_ERR,
                         "IPMI GetPostCode request data len invalid, "
                         "received: %d, required: %d\n",
                         *dataLen, sizeof(GetPostCodeReq));
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    GetPostCodeReq* reqData = reinterpret_cast<GetPostCodeReq*>(request);
    GetPostCodeRes* resData = reinterpret_cast<GetPostCodeRes*>(response);

    constexpr auto postCodeInterface =
        "xyz.openbmc_project.State.Boot.PostCode";
    constexpr auto postCodeObjPath =
        "/xyz/openbmc_project/State/Boot/PostCode0";
    constexpr auto postCodeService = "xyz.openbmc_project.State.Boot.PostCode0";
    const char* filename = "/usr/share/ipmi-providers/dev_id.json";
    static vector<int> manufId;
    static bool manuf_id_initialized = false;
    const static uint16_t lastestPostCodeIndex = 1;

    std::vector<std::tuple<uint64_t, std::vector<uint8_t>>> tmpBuffer;
    int tmpBufferIndex = 0;
    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();

    /* Get manufacturer id for JSON file */

    std::ifstream devIdFile(filename);
    if (!manuf_id_initialized)
    {
        if (devIdFile.is_open())
        {
            std::string line;
            while (std::getline(devIdFile, line))
            {
                if (std::string::npos != line.find("manuf_id"))
                {
                    std::smatch results;
                    std::regex manufIdPattern(".*: (\\d+).*(\\s?)");
                    constexpr size_t manufIdSize = 3;
                    if (std::regex_match(line, results, manufIdPattern))
                    {
                        if (results.size() == manufIdSize)
                        {
                            try
                            {
                                manufId.push_back(stoi(results[1]));
                            }
                            catch (const std::invalid_argument&)
                            {
                                sd_journal_print(
                                    LOG_ERR,
                                    "[%s] Convert to integral number failed\n",
                                    __FUNCTION__);
                                return IPMI_CC_UNSPECIFIED_ERROR;
                            }
                            manuf_id_initialized = true;
                        }
                    }
                    else
                    {
                        sd_journal_print(
                            LOG_ERR,
                            "[%s] Fail to match the regular expression\n",
                            __FUNCTION__);
                    }
                }
            }
            if (manufId.size() == 0)
            {
                sd_journal_print(
                    LOG_ERR,
                    "[%s] Fail to Get Manufacturer ID from Json File\n",
                    __FUNCTION__);
                return IPMI_CC_UNSPECIFIED_ERROR;
            }
        }
        else
        {
            sd_journal_print(LOG_ERR, "[%s] Device ID file not found\n",
                             __FUNCTION__);
            return IPMI_CC_UNSPECIFIED_ERROR;
        }
    }

    /* Compare manufacturer id */
    reverse(reqData->manufId, reqData->manufId + sizeof(reqData->manufId));
    for (int i = 0; i < manufId.size();)
    {
        int ret = std::memcmp(&reqData->manufId, &manufId[i],
                              sizeof(reqData->manufId));
        if (ret == 0)
        {
            break;
        }
        if (++i == manufId.size())
        {
            return IPMI_CC_INVALID_FIELD_REQUEST;
        }
    }

    int postFd = -1;
    postFd = open("/dev/aspeed-lpc-snoop0", O_NONBLOCK);
    if (postFd < 0)
    {
        return IPMI_CC_FILE_ERROR;
    }
    close(postFd);

    try
    {
        /* Get the post codes by calling GetPostCodes method */
        auto msg = dbus->new_method_call(postCodeService, postCodeObjPath,
                                         postCodeInterface, "GetPostCodes");
        msg.append(lastestPostCodeIndex);
        auto reply = dbus->call(msg);
        reply.read(tmpBuffer);

        int tmpBufferSize = tmpBuffer.size();
        /* Set command return length to return the last 20 post code*/
        if (tmpBufferSize > retPostCodeLen)
        {
            tmpBufferIndex = tmpBufferSize - retPostCodeLen;
            *dataLen = retPostCodeLen;
        }
        else
        {
            tmpBufferIndex = 0;
            *dataLen = tmpBufferSize;
        }

        /* Get post code data */
        for (int i = 0;
             ((i < retPostCodeLen) && (tmpBufferIndex < tmpBufferSize));
             ++i, ++tmpBufferIndex)
        {
            resData->postCode[i] = std::get<0>(tmpBuffer[tmpBufferIndex]);
        }
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        sd_journal_print(
            LOG_ERR, "IPMI GetPostCode Failed in call method, %s\n", e.what());
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    return IPMI_CC_OK;
}

/**
 *  @brief Function of getting amber LED status.
 *  @brief NetFn: 0x30, Cmd: 0x20
 *
 *  @param[in] None.
 *
 *  @return completion code and 1 byte amber LED status.
 **/
ipmi::RspType<uint8_t> IpmiGetAmberLedStatus()
{
    constexpr auto ledGroupService = "xyz.openbmc_project.LED.GroupManager";
    constexpr auto ledSensorFailPath =
        "/xyz/openbmc_project/led/groups/sensor_fail";
    constexpr auto ledGroupInterface = "xyz.openbmc_project.Led.Group";

    try
    {
        auto dbus = getSdBus();
        auto asserted =
            ipmi::getDbusProperty(*dbus, ledGroupService, ledSensorFailPath,
                                  ledGroupInterface, "Asserted");
        return ipmi::responseSuccess(std::get<bool>(asserted));
    }
    catch (const sdbusplus::exception_t& e)
    {
        sd_journal_print(LOG_ERR, "Failed to get amber led status, %s\n",
                         e.what());
        return ipmi::responseResponseError();
    }
}

/*
    Set SOL pattern func
    NetFn: 0x3E / CMD: 0xB2
*/
ipmi_ret_t ipmiSetSolPattern(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                             ipmi_request_t request, ipmi_response_t response,
                             ipmi_data_len_t data_len, ipmi_context_t context)
{
    SetSolPatternCmdReq* reqData =
        reinterpret_cast<SetSolPatternCmdReq*>(request);

    int32_t reqDataLen = (int32_t)*data_len;
    *data_len = 0;

    /* Data Length
       Pattern Number (1) + Max length (256) */
    if ((reqDataLen == 0) || (reqDataLen > (maxPatternLen + 1)))
    {
        sd_journal_print(LOG_CRIT, "[%s] invalid cmd data length %d\n",
                         __FUNCTION__, reqDataLen);
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    /* Pattern Number */
    uint8_t patternNum = reqData->patternIdx;
    if ((patternNum < 1) || (patternNum > 4))
    {
        sd_journal_print(LOG_CRIT, "[%s] invalid pattern number %d\n",
                         __FUNCTION__, patternNum);
        return IPMI_CC_INVALID_FIELD_REQUEST;
    }

    /* Copy the Pattern String */
    char tmpPattern[maxPatternLen + 1];
    memset(tmpPattern, '\0', maxPatternLen + 1);
    memcpy(tmpPattern, reqData->data, (reqDataLen - 1));
    std::string patternData = tmpPattern;

    /* Set pattern to dbus */
    std::string solPatternObjPath =
        solPatternObjPrefix + std::to_string((patternNum));

    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
    try
    {
        ipmi::setDbusProperty(*dbus, solPatternService, solPatternObjPath,
                              solPatternInterface, "Pattern", patternData);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    return IPMI_CC_OK;
}

/*
    Get SOL pattern func
    NetFn: 0x3E / CMD: 0xB3
*/
ipmi_ret_t ipmiGetSolPattern(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                             ipmi_request_t request, ipmi_response_t response,
                             ipmi_data_len_t data_len, ipmi_context_t context)
{
    GetSolPatternCmdReq* reqData =
        reinterpret_cast<GetSolPatternCmdReq*>(request);
    GetSolPatternCmdRes* resData =
        reinterpret_cast<GetSolPatternCmdRes*>(response);

    int32_t reqDataLen = (int32_t)*data_len;
    *data_len = 0;

    /* Data Length
       Pattern Number (1) */
    if (reqDataLen != 1)
    {
        sd_journal_print(LOG_CRIT, "[%s] invalid cmd data length %d\n",
                         __FUNCTION__, reqDataLen);
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    /* Pattern Number */
    uint8_t patternNum = reqData->patternIdx;
    if ((patternNum < 1) || (patternNum > 4))
    {
        sd_journal_print(LOG_CRIT, "[%s] invalid pattern number %d\n",
                         __FUNCTION__, patternNum);
        return IPMI_CC_INVALID_FIELD_REQUEST;
    }

    /* Get pattern to dbus */
    std::string solPatternObjPath =
        solPatternObjPrefix + std::to_string((patternNum));
    std::string patternData;

    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();
    try
    {
        auto value =
            ipmi::getDbusProperty(*dbus, solPatternService, solPatternObjPath,
                                  solPatternInterface, "Pattern");
        patternData = std::get<std::string>(value);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    /* Invalid pattern length */
    int32_t resDataLen = patternData.size();

    if (resDataLen > maxPatternLen)
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    memcpy(resData->data, patternData.data(), resDataLen);
    *data_len = resDataLen;

    return IPMI_CC_OK;
}

/**
 *   Set BMC Time Sync Mode
 *   NetFn: 0x3E / CMD: 0xB8
 *   @param[in]: Time sync mode
 *               - 0: Manual
 *               - 1: NTP
 **/

const std::vector<std::string> timeSyncMethod = {
    "xyz.openbmc_project.Time.Synchronization.Method.Manual",
    "xyz.openbmc_project.Time.Synchronization.Method.NTP"};

ipmi_ret_t ipmiSetBmcTimeSyncMode(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                  ipmi_request_t request,
                                  ipmi_response_t response,
                                  ipmi_data_len_t data_len,
                                  ipmi_context_t context)
{
    int32_t reqDataLen = (int32_t)*data_len;
    *data_len = 0;

    /* Data Length - Time sync mode (1) */
    if (reqDataLen != sizeof(SetBmcTimeSyncModeReq))
    {
        sd_journal_print(LOG_ERR, "[%s] invalid cmd data length %d\n",
                         __FUNCTION__, reqDataLen);
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    SetBmcTimeSyncModeReq* reqData =
        reinterpret_cast<SetBmcTimeSyncModeReq*>(request);

    /* Time Sync Mode Check */
    if ((reqData->syncMode) >= timeSyncMethod.size())
    {
        sd_journal_print(LOG_ERR, "[%s] invalid bmc time sync mode %d\n",
                         __FUNCTION__, reqData->syncMode);
        return IPMI_CC_INVALID_FIELD_REQUEST;
    }

    std::string modeProperty = timeSyncMethod.at(reqData->syncMode);
    std::shared_ptr<sdbusplus::asio::connection> dbus = getSdBus();

    // Set TimeSyncMethod Property
    try
    {
        ipmi::setDbusProperty(*dbus, settingMgtService, timeSyncMethodPath,
                              timeSyncMethodIntf, "TimeSyncMethod",
                              modeProperty);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        sd_journal_print(LOG_ERR,
                         "[%s] failed to set TimeSyncMethod Property\n",
                         __FUNCTION__);
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    bool isNtp =
        (modeProperty == "xyz.openbmc_project.Time.Synchronization.Method.NTP");

    // Set systemd NTP method
    auto method = dbus->new_method_call(systemdTimeService, systemdTimePath,
                                        systemdTimeInterface, "SetNTP");
    method.append(isNtp, false);

    try
    {
        dbus->call_noreply(method);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        sd_journal_print(LOG_ERR,
                         "[%s] failed to call systemd NTP set method\n",
                         __FUNCTION__);
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    return IPMI_CC_OK;
}

/*
    Get VR FW version func
    NetFn: 0x3C / CMD: 0x51
*/
static const uint8_t vrI2cBusNum = 5;
static const uint8_t vrCmdSwitchPage = 0x0;
static const uint8_t vrGetUserCodePage = 0x2f;
static const uint8_t vrCmdGetUserCode0 = 0x0c;
static const uint8_t vrCmdGetUserCode1 = 0x0d;
static const uint8_t vrFWVersionLength = 4;

ipmi_ret_t ipmiGetVrVersion(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                            ipmi_request_t request, ipmi_response_t response,
                            ipmi_data_len_t data_len, ipmi_context_t context)
{
    GetVrVersionCmdReq* reqData =
        reinterpret_cast<GetVrVersionCmdReq*>(request);
    GetVrVersionCmdRes* resData =
        reinterpret_cast<GetVrVersionCmdRes*>(response);

    int32_t reqDataLen = (int32_t)*data_len;
    *data_len = 0;

    if (reqDataLen != 1)
    {
        sd_journal_print(LOG_ERR, "[%s] invalid cmd data length %d\n",
                         __FUNCTION__, reqDataLen);
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    int fd = -1;
    int res = -1;
    uint8_t slaveaddr;
    std::vector<char> filename;
    filename.assign(32, 0);

    switch (reqData->vrType)
    {
        case CPU0_VCCIN:
            slaveaddr = 0x78;
            break;
        case CPU1_VCCIN:
            slaveaddr = 0x58;
            break;
        case CPU0_DIMM0:
            slaveaddr = 0x7a;
            break;
        case CPU0_DIMM1:
            slaveaddr = 0x6a;
            break;
        case CPU1_DIMM0:
            slaveaddr = 0x5a;
            break;
        case CPU1_DIMM1:
            slaveaddr = 0x4a;
            break;
        case CPU0_VCCIO:
            slaveaddr = 0x3a;
            break;
        case CPU1_VCCIO:
            slaveaddr = 0x2a;
            break;
        default:
            sd_journal_print(LOG_ERR,
                             "IPMI Get VR Version invalid field request, "
                             "received = 0x%x, required = 0x00 - 0x07\n",
                             reqData->vrType);
            return IPMI_CC_PARM_OUT_OF_RANGE;
    }

    fd = open_i2c_dev(vrI2cBusNum, filename.data(), filename.size(), 0);
    if (fd < 0)
    {
        sd_journal_print(LOG_CRIT, "Fail to open I2C device:[%d]\n", __LINE__);
        return IPMI_CC_BUSY;
    }

    std::vector<uint8_t> cmdData;
    std::vector<uint8_t> userCode0;
    std::vector<uint8_t> userCode1;

    // Select page to get User Code 0
    cmdData.assign(2, 0);
    cmdData.at(0) = vrCmdSwitchPage;
    cmdData.at(1) = vrGetUserCodePage;

    res = i2c_master_write(fd, slaveaddr, cmdData.size(), cmdData.data());

    if (res < 0)
    {
        sd_journal_print(LOG_CRIT,
                         "i2c_master_write failed: Get User Code 0\n");
        close_i2c_dev(fd);
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    // Get VR User Code 0
    cmdData.assign(1, vrCmdGetUserCode0);
    userCode0.assign(2, 0x0);

    res = i2c_master_write_read(fd, slaveaddr, cmdData.size(), cmdData.data(),
                                userCode0.size(), userCode0.data());
    if (res < 0)
    {
        sd_journal_print(LOG_CRIT,
                         "i2c_master_write_read failed: Get VR User Code 0\n");
        close_i2c_dev(fd);
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    // Select page to get User Code 1
    cmdData.assign(2, 0);
    cmdData.at(0) = vrCmdSwitchPage;
    cmdData.at(1) = vrGetUserCodePage;

    res = i2c_master_write(fd, slaveaddr, cmdData.size(), cmdData.data());

    if (res < 0)
    {
        sd_journal_print(LOG_CRIT, "i2c_master_write failed:[%d]\n", __LINE__);
        close_i2c_dev(fd);
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    //  Get VR User Code 1
    cmdData.assign(1, vrCmdGetUserCode1);
    userCode1.assign(2, 0x0);

    res = i2c_master_write_read(fd, slaveaddr, cmdData.size(), cmdData.data(),
                                userCode1.size(), userCode1.data());
    if (res < 0)
    {
        sd_journal_print(LOG_CRIT,
                         "i2c_master_write_read failed: Get VR User Code 1\n");
        close_i2c_dev(fd);
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    memcpy(&(resData->verData0), userCode0.data(), userCode0.size());
    memcpy(&(resData->verData1), userCode1.data(), userCode1.size());

    int32_t resDataLen = (int32_t)(userCode0.size() + userCode1.size());

    if (resDataLen != vrFWVersionLength)
    {
        sd_journal_print(LOG_CRIT, "Invalid VR version length\n");
        close_i2c_dev(fd);
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    *data_len = resDataLen;

    close_i2c_dev(fd);

    return IPMI_CC_OK;
}

/*
    OEM implements master write read IPMI command
    request :
    byte 1 : Bus ID
    byte 2 : Slave address (8 bits)
    byte 3 : Read count
    byte 4 - n : WriteData

    response :
    byte 1 : CC code
    byte 2 - n : Response data (Maximum = 50 bytes)
 */

ipmi_ret_t ipmiMasterWriteRead(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                               ipmi_request_t request, ipmi_response_t response,
                               ipmi_data_len_t dataLen, ipmi_context_t context)
{
    auto req = reinterpret_cast<MasterWriteReadReq*>(request);
    auto res = reinterpret_cast<MasterWriteReadRes*>(response);

    int ret = -1;
    int fd = -1;
    // Shift right 1 bit, use 7 bits data as slave address.
    uint8_t oemSlaveAddr = req->slaveAddr >> 1;
    // Write data length were counted from the fourth byte of request command.
    const size_t writeCount = *dataLen - 3;
    std::vector<char> filename;
    filename.assign(32, 0);

    fd = open_i2c_dev(req->busId, filename.data(), filename.size(), 0);
    if (fd < 0)
    {
        sd_journal_print(LOG_CRIT, "Fail to open I2C device:[%d]\n", __LINE__);
        return IPMI_CC_BUSY;
    }

    if (req->readCount > oemMaxIPMIWriteReadSize)
    {
        sd_journal_print(LOG_ERR,
                         "Master write read command: "
                         "Read count exceeds limit:%d bytes\n",
                         oemMaxIPMIWriteReadSize);
        close_i2c_dev(fd);
        return IPMI_CC_PARM_OUT_OF_RANGE;
    }

    if (!req->readCount && !writeCount)
    {
        sd_journal_print(LOG_ERR, "Master write read command: "
                                  "Read & write count are 0");
        close_i2c_dev(fd);
        return IPMI_CC_INVALID_FIELD_REQUEST;
    }

    if (req->readCount == 0)
    {
        ret = i2c_master_write(fd, oemSlaveAddr, writeCount, req->writeData);
    }
    else
    {
        ret =
            i2c_master_write_read(fd, oemSlaveAddr, writeCount, req->writeData,
                                  req->readCount, res->readResp);
    }

    if (ret < 0)
    {
        sd_journal_print(
            LOG_CRIT, "i2c_master_write_read failed: OEM master write read\n");
        close_i2c_dev(fd);
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    close_i2c_dev(fd);

    *dataLen = req->readCount;

    return IPMI_CC_OK;
}

/*
    Enable or disable a service
    NetFn: 0x30 / CMD: 0x0D
    Request:
        Byte 1: Set Service
            [7-1]: reserved
            [0]:
              0h: Disable Web Service
              1h: Enable Web Service
    Response:
        Byte 1: Response Code
*/
ipmi_ret_t ipmiSetService(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                          ipmi_request_t request, ipmi_response_t response,
                          ipmi_data_len_t data_len, ipmi_context_t context)
{
    constexpr auto service = "xyz.openbmc_project.Settings";
    constexpr auto path = "/xyz/openbmc_project/oem/ServiceStatus";
    constexpr auto serviceStatusInterface =
        "xyz.openbmc_project.OEM.ServiceStatus";
    constexpr auto webService = "WebService";

    auto bus = sdbusplus::bus::new_default();

    ServiceSettingReq* reqData = reinterpret_cast<ServiceSettingReq*>(request);

    int32_t reqDataLen = (int32_t)*data_len;
    *data_len = 0;

    if (reqDataLen != 1)
    {
        sd_journal_print(LOG_ERR, "[%s] invalid cmd data length %d\n",
                         __FUNCTION__, reqDataLen);
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    // Set web service status
    try
    {
        auto method =
            bus.new_method_call(service, path, propertyInterface, "Set");
        method.append(
            serviceStatusInterface, webService,
            std::variant<bool>((bool)(reqData->serviceSetting & 0x01)));
        bus.call_noreply(method);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        sd_journal_print(LOG_ERR, "[%s] Error in Set Service: %s", __FUNCTION__,
                         e.what());
        return IPMI_CC_PARM_OUT_OF_RANGE;
    }

    return IPMI_CC_OK;
}

/*
    Retrieve a service's enabled or disabled status
    NetFn: 0x30 / CMD: 0x0E
    Request:
    Response:
        Byte 1: Response Code
        Byte 2:
            [7-1]: reserved
            [0]:
              0h: Disabled Web Service
              1h: Enabled Web Service
*/
ipmi_ret_t ipmiGetService(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                          ipmi_request_t request, ipmi_response_t response,
                          ipmi_data_len_t data_len, ipmi_context_t context)
{
    uint8_t serviceResponse = 0;

    constexpr auto service = "xyz.openbmc_project.Settings";
    constexpr auto path = "/xyz/openbmc_project/oem/ServiceStatus";
    constexpr auto serviceStatusInterface =
        "xyz.openbmc_project.OEM.ServiceStatus";
    constexpr auto webService = "WebService";

    auto bus = sdbusplus::bus::new_default();

    ServiceSettingReq* resData = reinterpret_cast<ServiceSettingReq*>(response);
    std::variant<bool> result;

    try
    {
        // Get web service status
        auto method =
            bus.new_method_call(service, path, propertyInterface, "Get");
        method.append(serviceStatusInterface, webService);
        auto reply = bus.call(method);
        reply.read(result);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        sd_journal_print(LOG_ERR, "Error in Get Service %s", e.what());
        return IPMI_CC_UNSPECIFIED_ERROR;
    }
    auto webServiceStatus = std::get<bool>(result);

    serviceResponse = (uint8_t)(webServiceStatus);

    resData->serviceSetting = serviceResponse;
    *data_len = sizeof(ServiceSettingReq);

    return IPMI_CC_OK;
}

int8_t getCpldFwViaI2c(int bus, int addr, std::vector<uint8_t> *readData)
{
    std::vector<char> filename;
    filename.assign(20, 0);

    int fd = open_i2c_dev(bus, filename.data(), filename.size(), 0);
    if (fd < 0)
    {
        std::cerr << "Fail to open I2C device: " << unsigned(bus) << "\n";
        return -1;
    }

    std::vector<uint8_t> cmdData = {0xc0, 0x0, 0x0, 0x0};
    std::vector<uint8_t> resData;
    uint8_t resSize = 4;
    resData.assign(resSize, 0);

    int ret = i2c_master_write_read(fd, addr, cmdData.size(), cmdData.data(),
                                    resData.size(), resData.data());
    if (ret < 0)
    {
        std::cerr << "Fail to r/w I2C device: " << unsigned(bus)
                  << ", Addr: " << unsigned(addr) << "\n";
        close_i2c_dev(fd);
        return -1;
    }

    *readData = resData;
    close_i2c_dev(fd);

    return 0;
}

int8_t getVrFw(int bus, int addr, std::vector<uint8_t> *readData)
{
    std::vector<char> filename;
    filename.assign(20, 0);

    int fd = open_i2c_dev(bus, filename.data(), filename.size(), 0);
    if (fd < 0)
    {
        std::cerr << "Fail to open I2C device: " << unsigned(bus) << "\n";
        return -1;
    }

    std::vector<uint8_t> writeDmaCmd = {0xc7, 0x3f, 0x0};
    int ret = i2c_master_write(fd, addr, writeDmaCmd.size(), writeDmaCmd.data());

    if (ret < 0)
    {
        sd_journal_print(LOG_CRIT, "i2c_master_write failed: Get User Code 0\n");
        close_i2c_dev(fd);
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    std::vector<uint8_t> readDmaCmd = {0xc5};
    std::vector<uint8_t> resData;
    uint8_t resSize = 4;
    resData.assign(resSize, 0);

    ret = i2c_master_write_read(fd, addr, readDmaCmd.size(), readDmaCmd.data(),
                                resData.size(), resData.data());
    if (ret < 0)
    {
        std::cerr << "Fail to r/w I2C device: " << unsigned(bus)
                  << ", Addr: " << unsigned(addr) << "\n";
        close_i2c_dev(fd);
        return -1;
    }

    *readData = resData;
    close_i2c_dev(fd);

    return 0;
}

/*
    Get FW Version func
    NetFn: 0x3C / CMD: 0x80
*/
ipmi_ret_t ipmiGetFwVersion(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                            ipmi_request_t request, ipmi_response_t response,
                            ipmi_data_len_t data_len, ipmi_context_t context)
{
    GetFwVersionReq* reqData = reinterpret_cast<GetFwVersionReq*>(request);
    GetFwVersionRes* resData = reinterpret_cast<GetFwVersionRes*>(response);

    constexpr auto verService = "xyz.openbmc_project.Software.BMC.Updater";
    constexpr auto bmcObjPath = "/xyz/openbmc_project/software/BMC";
    constexpr auto biosObjPath = "/xyz/openbmc_project/software/BIOS";
    constexpr auto verInterface = "xyz.openbmc_project.Software.Version";

    int32_t reqDataLen = (int32_t) * data_len;
    if (reqDataLen == 0)
    {
        sd_journal_print(LOG_ERR, "[%s] invalid cmd data length %d\n",
                         __FUNCTION__, reqDataLen);
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    if (reqData->component == FW_BMC)
    {
        if (reqDataLen != 1)
        {
            sd_journal_print(LOG_ERR, "[%s] invalid cmd data length %d\n",
                             __FUNCTION__, reqDataLen);
            return IPMI_CC_REQ_DATA_LEN_INVALID;
        }

        std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
        auto method = bus->new_method_call(verService, bmcObjPath,
                                           propertyInterface, "Get");
        method.append(verInterface, "Version");
        std::string verStr;
        try
        {
            auto reply = bus->call(method);
            std::variant<std::string> verProp;
            reply.read(verProp);
            verStr = std::get<std::string>(verProp);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            sd_journal_print(LOG_ERR,
                             "Failed to obtain isPowerCapEnable property, %s\n",
                             e.what());
            return IPMI_CC_UNSPECIFIED_ERROR;
        }

        *data_len = 3;

        // Split the version string
        int curPos = 0;
        int next = 0;
        int verPos = 0;
        while (next != std::string::npos)
        {
            next = verStr.find_first_of(".", curPos);
            if (next != curPos)
            {
                char *ptr;
                std::string version = verStr.substr(curPos, next - curPos);
                resData->fwVersion[verPos] = strtol(version.c_str(), &ptr, 10);
            }
            curPos = next + 1;
            verPos++;
        }
    }
    else if (reqData->component == FW_BIOS)
    {
        std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
        auto propValue = ipmi::getDbusProperty(*bus, verService, biosObjPath,
                                               verInterface, "Version");
        std::string biosVersion = std::get<std::string>(propValue);

        *data_len = biosVersion.length();

        for (std::size_t i = 0; i < biosVersion.length(); i++)
        {
            resData->fwVersion[i] = biosVersion.at(i);
        }
    }
    else if (reqData->component == FW_CPLD)
    {
        if (reqDataLen != 2)
        {
            sd_journal_print(LOG_ERR, "[%s] invalid cmd data length %d\n",
                             __FUNCTION__, reqDataLen);
            return IPMI_CC_REQ_DATA_LEN_INVALID;
        }

        *data_len = 4;
        std::vector<uint8_t> readData;
        uint8_t resSize = 4;
        readData.assign(resSize, 0);

        if (reqData->index == CPLD_MB)
        {

            uint32_t buffer = 0;

            gpiod_ctxless_set_value("gpiochip0", gpioJtagBmcOeN, 0, false, "JTAG_BMC_OE_N" , NULL, NULL);
            gpiod_ctxless_set_value("gpiochip0", gpioBmcJtagSel, 1, false, "BMC_JTAG_SEL", NULL, NULL);

            int ret = getCpldUserCode(LATTICE, &buffer);
            if (ret < 0)
            {
                sd_journal_print(LOG_CRIT, "Failed to get MB CPLD version\n");
                return IPMI_CC_UNSPECIFIED_ERROR;
            }

            gpiod_ctxless_set_value("gpiochip0", gpioJtagBmcOeN, 1, false, "JTAG_BMC_OE_N" , NULL, NULL);
            gpiod_ctxless_set_value("gpiochip0", gpioBmcJtagSel, 0, false, "BMC_JTAG_SEL", NULL, NULL);

            memcpy(&(resData->fwVersion), &buffer, sizeof(uint32_t));
        }
        else if (reqData->index == CPLD_DPB)
        {
            uint8_t bus = 7;
            uint8_t addr = 0x40;

            int8_t ret = getCpldFwViaI2c(bus, addr, &readData);
            if (ret < 0)
            {
                sd_journal_print(LOG_CRIT, "Failed to get PDB CPLD version\n");
                return IPMI_CC_UNSPECIFIED_ERROR;
            }
            copy(readData.begin(), readData.end(), resData->fwVersion);
        }
        else if (reqData->index == CPLD_48V_PDB)
        {
            uint8_t bus = 14;
            uint8_t addr = 0x40;

            int8_t ret = getCpldFwViaI2c(bus, addr, &readData);
            if (ret < 0)
            {
                sd_journal_print(LOG_CRIT, "Failed to get 48V PDB CPLD version\n");
                return IPMI_CC_UNSPECIFIED_ERROR;
            }

            copy(readData.begin(), readData.end(), resData->fwVersion);
        }
        else
        {
            sd_journal_print(LOG_ERR,
                             "Invalid Fw component operation, "
                             "index = 0x%x\n", reqData->index);
            return IPMI_CC_PARM_OUT_OF_RANGE;
        }
    }
    else if (reqData->component == FW_VR)
    {
        if (reqDataLen != 2)
        {
            sd_journal_print(LOG_ERR, "[%s] invalid cmd data length %d\n",
                             __FUNCTION__, reqDataLen);
            return IPMI_CC_REQ_DATA_LEN_INVALID;
        }

        *data_len = 4;
        std::vector<uint8_t> readData;
        uint8_t resSize = 4;
        readData.assign(resSize, 0);

        if (reqData->index == VR_CPU_CORE)
        {
            uint8_t bus = 5;
            uint8_t addr = 0x61;

            int8_t ret = getVrFw(bus, addr, &readData);
            if (ret < 0)
            {
                sd_journal_print(LOG_CRIT, "Failed to get CPU CORE VR version\n");
                return IPMI_CC_UNSPECIFIED_ERROR;
            }

            copy(readData.begin(), readData.end(), resData->fwVersion);
        }
        else if (reqData->index == VR_CPU_SOC)
        {
            uint8_t bus = 5;
            uint8_t addr = 0x63;

            int8_t ret = getVrFw(bus, addr, &readData);
            if (ret < 0)
            {
                sd_journal_print(LOG_CRIT, "Failed to get CPU SOC VR version\n");
                return IPMI_CC_UNSPECIFIED_ERROR;
            }

            copy(readData.begin(), readData.end(), resData->fwVersion);
        }
        else if (reqData->index == VR_MEM_ABCD)
        {
            uint8_t bus = 5;
            uint8_t addr = 0x64;

            int8_t ret = getVrFw(bus, addr, &readData);
            if (ret < 0)
            {
                sd_journal_print(LOG_CRIT, "Failed to get MEM ABCD VR version\n");
                return IPMI_CC_UNSPECIFIED_ERROR;
            }

            copy(readData.begin(), readData.end(), resData->fwVersion);
        }
        else if (reqData->index == VR_MEM_EFGH)
        {
            uint8_t bus = 5;
            uint8_t addr = 0x65;

            int8_t ret = getVrFw(bus, addr, &readData);
            if (ret < 0)
            {
                sd_journal_print(LOG_CRIT, "Failed to get MEM MEM EFGH VR version\n");
                return IPMI_CC_UNSPECIFIED_ERROR;
            }

            copy(readData.begin(), readData.end(), resData->fwVersion);
        }
        else if (reqData->index == VR_48V_PDB)
        {
            ifstream ifs ("/run/openbmc/VR_48V_PDB.tmp", ifstream::in);
            if (!ifs)
            {
                sd_journal_print(LOG_ERR,
                                 "Failed to open file /run/openbmc/VR_48V_PDB.tmp\n");
            }

            string str;
            int i = 0;
            while (ifs >> str)
            {
                readData.at(i) = stoi(str, nullptr, 16);
                i++;
            }

            copy(readData.begin(), readData.end(), resData->fwVersion);
        }
        else
        {
            sd_journal_print(LOG_ERR,
                             "Invalid Fw component operation,"
                             "index = 0x%x\n", reqData->index);
            return IPMI_CC_PARM_OUT_OF_RANGE;
        }
    }
    else
    {
        sd_journal_print(LOG_ERR,
                         "Invalid Fw component operation, "
                         "component = 0x%x\n", reqData->component);
        return IPMI_CC_PARM_OUT_OF_RANGE;
    }

    return IPMI_CC_OK;
}

/*
    Access CPLD JTAG func
    NetFn: 0x3C / CMD: 0x88
*/
ipmi_ret_t ipmiAccessCpldJtag(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                              ipmi_request_t request, ipmi_response_t response,
                              ipmi_data_len_t data_len, ipmi_context_t context)
{
    AccessCpldJtagCmdReq* reqData =
        reinterpret_cast<AccessCpldJtagCmdReq*>(request);
    AccessCpldJtagCmdRes* resData =
        reinterpret_cast<AccessCpldJtagCmdRes*>(response);

    int32_t reqDataLen = (int32_t)*data_len;
    *data_len = 0;

    if (reqDataLen != sizeof(AccessCpldJtagCmdReq))
    {
        sd_journal_print(LOG_ERR, "[%s] invalid cmd data length %d\n",
                         __FUNCTION__, reqDataLen);
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    uint32_t buffer = 0;
    int res = -1;

    /*  JTAG MUX OE#, its default is high.
        H:Disable (Default)
        L:Enable */
    gpiod_ctxless_set_value("gpiochip0", gpioJtagBmcOeN, 0, false,
                            "JTAG_BMC_OE_N", NULL, NULL);

    /*  JTAG MUX select
        0: BMC JTAG To CPU (default)
        1: BMC JTAG To CPLD */
    gpiod_ctxless_set_value("gpiochip0", gpioBmcJtagSel, 1, false,
                            "BMC_JTAG_SEL", NULL, NULL);

    switch (reqData->cpldOp)
    {
        case GET_USERCODE:
        {
            res = getCpldUserCode(LATTICE, &buffer);
            break;
        }
        case GET_IDCODE:
        {
            res = getCpldIdCode(LATTICE, &buffer);
            break;
        }
        default:
            sd_journal_print(LOG_ERR,
                             "Invalid CPLD JTAG operation, "
                             "received = 0x%x\n",
                             reqData->cpldOp);
            return IPMI_CC_PARM_OUT_OF_RANGE;
    }

    gpiod_ctxless_set_value("gpiochip0", gpioJtagBmcOeN, 1, false,
                            "JTAG_BMC_OE_N", NULL, NULL);
    gpiod_ctxless_set_value("gpiochip0", gpioBmcJtagSel, 0, false,
                            "BMC_JTAG_SEL", NULL, NULL);

    if (res < 0)
    {
        sd_journal_print(LOG_CRIT, "[%d] Failed to access CPLD via JTAG\n",
                         reqData->cpldOp);
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    memcpy(&(resData->result), &buffer, sizeof(uint32_t));
    *data_len = sizeof(AccessCpldJtagCmdRes);

    return IPMI_CC_OK;
}
/*
    Get CPU Power Consumption
    NetFn: 0x34 / CMD: 0x01
*/
ipmi_ret_t ipmiGetCpuPower(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                           ipmi_request_t request, ipmi_response_t response,
                           ipmi_data_len_t data_len, ipmi_context_t context)
{
    GetCpuPowerCmdReq* reqData = reinterpret_cast<GetCpuPowerCmdReq*>(request);
    GetCpuPowerCmdRes* resData = reinterpret_cast<GetCpuPowerCmdRes*>(response);

    int32_t reqDataLen = (int32_t)*data_len;
    *data_len = 0;
    if (reqDataLen != sizeof(GetCpuPowerCmdReq))
    {
        sd_journal_print(LOG_ERR, "[%s] invalid cmd data length %d\n",
                         __FUNCTION__, reqDataLen);
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    std::string autoPowerCap = "OFF";
    std::string manualPowerCap = "OFF";
    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    auto propValue =
        ipmi::getDbusProperty(*bus, cpuPwrCtrlService, cpuPwrCtrlPath,
                              operationalStatusIntf, autoPowerCapProperty);
    autoPowerCap = std::get<std::string>(propValue);
    propValue =
        ipmi::getDbusProperty(*bus, cpuPwrCtrlService, cpuPwrCtrlPath,
                              operationalStatusIntf, manualPowerCapProperty);
    manualPowerCap = std::get<std::string>(propValue);

    if ((autoPowerCap == "OFF") && (manualPowerCap == "OFF"))
    {
        if (!isPgoodOn() || !isPostComplete())
        {
            return IPMI_CC_NOT_SUPPORTED_IN_PRESENT_STATE;
        }
    }
    else if ((autoPowerCap == "PAUSE") || (manualPowerCap == "PAUSE"))
    {
        return IPMI_CC_PARAMETER_IS_ILLEGAL;
    }

    int socketID = reqData->socketID;
    if (socketID != cpuSocketID)
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    std::tuple<int, uint32_t> buffer;
    auto getCpuPowerConsumptionDbus =
        bus->new_method_call(cpuPwrCtrlService, cpuPwrCtrlPath,
                             cpuPwrCtrlInterface, getCpuPowerConsumption);
    getCpuPowerConsumptionDbus.append(socketID);
    try
    {
        auto reply = bus->call(getCpuPowerConsumptionDbus);
        reply.read(buffer);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        sd_journal_print(
            LOG_ERR, "[%s] failed to call systemd CPU power get method:%s\n",
            __FUNCTION__, e.what());
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    auto [ret, cpuPowerConsumption] = buffer;
    if (ret)
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }
    memcpy(&(resData->powerConsumption), &cpuPowerConsumption,
           sizeof(uint32_t));
    *data_len = sizeof(GetCpuPowerCmdRes);
    return IPMI_CC_OK;
}
/*
    Get CPU Power Limit
    NetFn: 0x34 / CMD: 0x02
*/
ipmi_ret_t ipmiGetCpuPowerLimit(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                ipmi_request_t request,
                                ipmi_response_t response,
                                ipmi_data_len_t data_len,
                                ipmi_context_t context)
{
    GetCpuPowerLimitCmdReq* reqData =
        reinterpret_cast<GetCpuPowerLimitCmdReq*>(request);
    GetCpuPowerLimitCmdRes* resData =
        reinterpret_cast<GetCpuPowerLimitCmdRes*>(response);

    int32_t reqDataLen = (int32_t)*data_len;
    *data_len = 0;
    if (reqDataLen != sizeof(GetCpuPowerLimitCmdReq))
    {
        sd_journal_print(LOG_ERR, "[%s] invalid cmd data length %d\n",
                         __FUNCTION__, reqDataLen);
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    std::string autoPowerCap = "OFF";
    std::string manualPowerCap = "OFF";
    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    auto propValue =
        ipmi::getDbusProperty(*bus, cpuPwrCtrlService, cpuPwrCtrlPath,
                              operationalStatusIntf, autoPowerCapProperty);
    autoPowerCap = std::get<std::string>(propValue);
    propValue =
        ipmi::getDbusProperty(*bus, cpuPwrCtrlService, cpuPwrCtrlPath,
                              operationalStatusIntf, manualPowerCapProperty);
    manualPowerCap = std::get<std::string>(propValue);

    if ((autoPowerCap == "OFF") && (manualPowerCap == "OFF"))
    {
        if (!isPgoodOn() || !isPostComplete())
        {
            return IPMI_CC_NOT_SUPPORTED_IN_PRESENT_STATE;
        }
    }
    else if ((autoPowerCap == "PAUSE") || (manualPowerCap == "PAUSE"))
    {
        return IPMI_CC_PARAMETER_IS_ILLEGAL;
    }

    int socketID = reqData->socketID;
    if (socketID != cpuSocketID)
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    std::tuple<int, uint32_t> buffer;
    auto getCpuPowerLimitDbus =
        bus->new_method_call(cpuPwrCtrlService, cpuPwrCtrlPath,
                             cpuPwrCtrlInterface, getCpuPowerLimit);
    getCpuPowerLimitDbus.append(socketID);
    try
    {
        auto reply = bus->call(getCpuPowerLimitDbus);
        reply.read(buffer);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        sd_journal_print(
            LOG_ERR, "[%s] failed to call systemd CPU Limit get method:%s\n",
            __FUNCTION__, e.what());
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    auto [ret, cpuPowerLimit] = buffer;
    if (ret)
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }
    memcpy(&(resData->powerLimit), &cpuPowerLimit, sizeof(uint32_t));
    *data_len = sizeof(GetCpuPowerCmdRes);
    return IPMI_CC_OK;
}
/*
    Set System Power Limit
    NetFn: 0x34 / CMD: 0x03
*/
ipmi_ret_t ipmiSetSystemPowerLimit(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                   ipmi_request_t request,
                                   ipmi_response_t response,
                                   ipmi_data_len_t data_len,
                                   ipmi_context_t context)
{
    SetSystemPowerLimitCmdReq* reqData =
        reinterpret_cast<SetSystemPowerLimitCmdReq*>(request);

    int32_t reqDataLen = (int32_t)*data_len;
    *data_len = 0;
    if (reqDataLen != sizeof(SetSystemPowerLimitCmdReq))
    {
        sd_journal_print(LOG_ERR, "[%s] invalid cmd data length %d\n",
                         __FUNCTION__, reqDataLen);
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    std::string autoPowerCap = "OFF";
    std::string manualPowerCap = "OFF";
    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    auto propValue =
        ipmi::getDbusProperty(*bus, cpuPwrCtrlService, cpuPwrCtrlPath,
                              operationalStatusIntf, autoPowerCapProperty);
    autoPowerCap = std::get<std::string>(propValue);
    propValue =
        ipmi::getDbusProperty(*bus, cpuPwrCtrlService, cpuPwrCtrlPath,
                              operationalStatusIntf, manualPowerCapProperty);
    manualPowerCap = std::get<std::string>(propValue);

    if ((autoPowerCap == "OFF") && (manualPowerCap == "OFF"))
    {
        if (!isPgoodOn() || !isPostComplete())
        {
            return IPMI_CC_NOT_SUPPORTED_IN_PRESENT_STATE;
        }
    }
    else if ((autoPowerCap == "PAUSE") || (manualPowerCap == "PAUSE"))
    {
        return IPMI_CC_PARAMETER_IS_ILLEGAL;
    }

    uint32_t systemPowerLimit = reqData->systemPowerLimit;
    int socketID = reqData->socketID;
    if (socketID != cpuSocketID)
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    auto setSystemPowerLimitDbus =
        bus->new_method_call(cpuPwrCtrlService, cpuPwrCtrlPath,
                             cpuPwrCtrlInterface, setSystemPowerLimit);
    setSystemPowerLimitDbus.append(socketID, systemPowerLimit);
    try
    {
        int ret = 0;
        auto reply = bus->call(setSystemPowerLimitDbus);
        reply.read(ret);
        if (ret)
        {
            return IPMI_CC_UNSPECIFIED_ERROR;
        }
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        sd_journal_print(
            LOG_ERR, "[%s] Failed to call systemd CPU Limit get method:%s\n",
            __FUNCTION__, e.what());
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    std::fstream powerCapFile;
    const char* filename = "/var/lib/amd-cpu-power-cap/PowerCapStatus.json";
    nlohmann::json outJson;
    outJson["autoPowerCap"] = "ON";
    outJson["systemPowerLimit"] = systemPowerLimit;
    outJson["manualPowerCap"] = "OFF";
    outJson["cpuPowerLimit"] = 0;

    powerCapFile.open(filename, std::ios::out | std::ios::trunc);
    if (!powerCapFile)
    {
        sd_journal_print(LOG_ERR, "[%s] Power cap state file isn't exist.",
                         __FUNCTION__);
        auto dir = std::filesystem::path("/var/lib/amd-cpu-power-cap");
        std::filesystem::create_directories(dir);

        std::ofstream file(filename);
        file << outJson.dump(4);
        if (file.fail())
        {
            sd_journal_print(
                LOG_ERR,
                "[%s] Failed to write the setting into power cap state file",
                __FUNCTION__);
        }
        file.close();
    }
    else
    {
        powerCapFile << outJson.dump(4);
        if (powerCapFile.fail())
        {
            sd_journal_print(
                LOG_ERR,
                "[%s] Failed to write the setting into power cap state file",
                __FUNCTION__);
        }
        powerCapFile.close();
    }
    if (manualPowerCap == "ON")
    {
        sd_journal_print(LOG_ERR, "[%s] Disable CPU power cap - manual mode",
                         __FUNCTION__);
    }
    return IPMI_CC_OK;
}
/*
    Set Default CPU Power Limit
    NetFn: 0x34 / CMD: 0x04
*/
ipmi_ret_t ipmiSetDefaultCPUPowerLimit(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                       ipmi_request_t request,
                                       ipmi_response_t response,
                                       ipmi_data_len_t data_len,
                                       ipmi_context_t context)
{
    SetDefaultCpuPowerCmdReq* reqData =
        reinterpret_cast<SetDefaultCpuPowerCmdReq*>(request);

    int32_t reqDataLen = (int32_t)*data_len;
    *data_len = 0;
    if (reqDataLen != sizeof(SetDefaultCpuPowerCmdReq))
    {
        sd_journal_print(LOG_ERR, "[%s] invalid cmd data length %d\n",
                         __FUNCTION__, reqDataLen);
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    std::string autoPowerCap = "OFF";
    std::string manualPowerCap = "OFF";
    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    auto propValue =
        ipmi::getDbusProperty(*bus, cpuPwrCtrlService, cpuPwrCtrlPath,
                              operationalStatusIntf, autoPowerCapProperty);
    autoPowerCap = std::get<std::string>(propValue);
    propValue =
        ipmi::getDbusProperty(*bus, cpuPwrCtrlService, cpuPwrCtrlPath,
                              operationalStatusIntf, manualPowerCapProperty);
    manualPowerCap = std::get<std::string>(propValue);

    if ((autoPowerCap == "OFF") && (manualPowerCap == "OFF"))
    {
        if (!isPgoodOn() || !isPostComplete())
        {
            return IPMI_CC_NOT_SUPPORTED_IN_PRESENT_STATE;
        }
    }
    else if ((autoPowerCap == "PAUSE") || (manualPowerCap == "PAUSE"))
    {
        return IPMI_CC_PARAMETER_IS_ILLEGAL;
    }

    int socketID = reqData->socketID;
    if (socketID != cpuSocketID)
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    auto setDefaultCpuPwrDbus =
        bus->new_method_call(cpuPwrCtrlService, cpuPwrCtrlPath,
                             cpuPwrCtrlInterface, setDefaultCpuPowerLimit);
    setDefaultCpuPwrDbus.append(socketID);
    try
    {
        int ret = 0;
        auto reply = bus->call(setDefaultCpuPwrDbus);
        reply.read(ret);
        if (ret)
        {
            return IPMI_CC_UNSPECIFIED_ERROR;
        }
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        sd_journal_print(
            LOG_ERR, "[%s] Failed to call systemd CPU Limit get method:%s\n",
            __FUNCTION__, e.what());
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    std::fstream powerCapFile;
    const char* filename = "/var/lib/amd-cpu-power-cap/PowerCapStatus.json";
    nlohmann::json outJson;
    outJson["autoPowerCap"] = "OFF";
    outJson["systemPowerLimit"] = 0;
    outJson["manualPowerCap"] = "OFF";
    outJson["cpuPowerLimit"] = 0;

    powerCapFile.open(filename, std::ios::out | std::ios::trunc);
    if (!powerCapFile)
    {
        sd_journal_print(LOG_ERR, "[%s] Power cap state file isn't exist.",
                         __FUNCTION__);
        auto dir = std::filesystem::path("/var/lib/amd-cpu-power-cap");
        std::filesystem::create_directories(dir);

        std::ofstream file(filename);
        file << outJson.dump(4);
        if (file.fail())
        {
            sd_journal_print(
                LOG_ERR,
                "[%s] Failed to write the setting into power cap state file",
                __FUNCTION__);
        }
        file.close();
    }
    else
    {
        powerCapFile << outJson.dump(4);
        if (powerCapFile.fail())
        {
            sd_journal_print(
                LOG_ERR,
                "[%s] Failed to write the setting into power cap state file",
                __FUNCTION__);
        }
        powerCapFile.close();
    }
    if (manualPowerCap == "ON")
    {
        sd_journal_print(LOG_ERR, "[%s] Disable CPU power cap - manual mode",
                         __FUNCTION__);
    }
    return IPMI_CC_OK;
}
/*
    Get System Power Limit
    NetFn: 0x34 / CMD: 0x05
*/
ipmi_ret_t ipmiGetSystemPowerLimit(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                   ipmi_request_t request,
                                   ipmi_response_t response,
                                   ipmi_data_len_t data_len,
                                   ipmi_context_t context)
{
    GetSystemPowerLimitCmdRes* resData =
        reinterpret_cast<GetSystemPowerLimitCmdRes*>(response);

    int32_t reqDataLen = (int32_t)*data_len;
    *data_len = 0;

    /* Data Length - 0 */
    if (reqDataLen != 0)
    {
        sd_journal_print(LOG_ERR, "[%s] invalid cmd data length %d\n",
                         __FUNCTION__, reqDataLen);
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    std::string autoPowerCap = "OFF";

    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    auto propValue =
        ipmi::getDbusProperty(*bus, cpuPwrCtrlService, cpuPwrCtrlPath,
                              operationalStatusIntf, autoPowerCapProperty);
    autoPowerCap = std::get<std::string>(propValue);

    if (autoPowerCap == "OFF")
    {
        return IPMI_CC_NOT_SUPPORTED_IN_PRESENT_STATE;
    }
    else if (autoPowerCap == "PAUSE")
    {
        return IPMI_CC_PARAMETER_IS_ILLEGAL;
    }

    uint32_t sysPowerLimit = 0;
    auto sysPowerLimitPropValue =
        ipmi::getDbusProperty(*bus, cpuPwrCtrlService, cpuPwrCtrlPath,
                              parameterIntf, systemPowerLimitProperty);
    sysPowerLimit = std::get<uint32_t>(sysPowerLimitPropValue);

    memcpy(&(resData->syspowerLimit), &sysPowerLimit, sizeof(uint32_t));
    *data_len = sizeof(GetSystemPowerLimitCmdRes);
    return IPMI_CC_OK;
}
/*
    Set CPU Power Limit
    NetFn: 0x34 / CMD: 0x06
*/
ipmi_ret_t ipmiSetCPUPowerLimit(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                ipmi_request_t request,
                                ipmi_response_t response,
                                ipmi_data_len_t data_len,
                                ipmi_context_t context)
{
    SetCpuPowerLimitCmdReq* reqData =
        reinterpret_cast<SetCpuPowerLimitCmdReq*>(request);

    int32_t reqDataLen = (int32_t)*data_len;
    *data_len = 0;
    if (reqDataLen != sizeof(SetCpuPowerLimitCmdReq))
    {
        sd_journal_print(LOG_ERR, "[%s] invalid cmd data length %d\n",
                         __FUNCTION__, reqDataLen);
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    std::string autoPowerCap = "OFF";
    std::string manualPowerCap = "OFF";
    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();
    auto propValue =
        ipmi::getDbusProperty(*bus, cpuPwrCtrlService, cpuPwrCtrlPath,
                              operationalStatusIntf, autoPowerCapProperty);
    autoPowerCap = std::get<std::string>(propValue);
    propValue =
        ipmi::getDbusProperty(*bus, cpuPwrCtrlService, cpuPwrCtrlPath,
                              operationalStatusIntf, manualPowerCapProperty);
    manualPowerCap = std::get<std::string>(propValue);

    if ((autoPowerCap != "OFF") && (manualPowerCap == "OFF"))
    {
        if (!isPgoodOn() || !isPostComplete())
        {
            return IPMI_CC_NOT_SUPPORTED_IN_PRESENT_STATE;
        }
    }
    else if ((autoPowerCap != "OFF") || (manualPowerCap == "PAUSE"))
    {
        return IPMI_CC_PARAMETER_IS_ILLEGAL;
    }

    uint32_t cpuPowerLimit = reqData->cpuPowerLimit;
    int socketID = reqData->socketID;
    if (socketID != cpuSocketID)
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    auto setCpuPowerLimitDbus =
        bus->new_method_call(cpuPwrCtrlService, cpuPwrCtrlPath,
                             cpuPwrCtrlInterface, setCpuPowerLimit);
    setCpuPowerLimitDbus.append(socketID, cpuPowerLimit);
    try
    {
        int ret = 0;
        auto reply = bus->call(setCpuPowerLimitDbus);
        reply.read(ret);
        if (ret)
        {
            return IPMI_CC_UNSPECIFIED_ERROR;
        }
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        sd_journal_print(
            LOG_ERR, "[%s] Failed to call systemd CPU Limit get method:%s\n",
            __FUNCTION__, e.what());
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    std::fstream powerCapFile;
    const char* filename = "/var/lib/amd-cpu-power-cap/PowerCapStatus.json";
    nlohmann::json outJson;
    outJson["autoPowerCap"] = "OFF";
    outJson["systemPowerLimit"] = 0;
    outJson["manualPowerCap"] = "ON";
    outJson["cpuPowerLimit"] = cpuPowerLimit;

    powerCapFile.open(filename, std::ios::out | std::ios::trunc);
    if (!powerCapFile)
    {
        sd_journal_print(LOG_ERR, "[%s] Power cap state file isn't exist.",
                         __FUNCTION__);
        auto dir = std::filesystem::path("/var/lib/amd-cpu-power-cap");
        std::filesystem::create_directories(dir);

        std::ofstream file(filename);
        file << outJson.dump(4);
        if (file.fail())
        {
            sd_journal_print(
                LOG_ERR,
                "[%s] Failed to write the setting into power cap state file",
                __FUNCTION__);
        }
        file.close();
    }
    else
    {
        powerCapFile << outJson.dump(4);
        if (powerCapFile.fail())
        {
            sd_journal_print(
                LOG_ERR,
                "[%s] Failed to write the setting into power cap state file",
                __FUNCTION__);
        }
        powerCapFile.close();
    }
    sd_journal_print(LOG_ERR, "[%s] Enable CPU power cap - manual mode",
                     __FUNCTION__);

    return IPMI_CC_OK;
}
/**
 *   Get BMC Boot from Information
 *   NetFn: 0x3E / CMD: 0xBB
 *   Response:
 *           - 1: Primary
 *           - 2: Backup
 **/
ipmi_ret_t ipmiGetBmcBootFrom(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                              ipmi_request_t request, ipmi_response_t response,
                              ipmi_data_len_t data_len, ipmi_context_t context)
{
    if (*data_len != 0)
    {
        sd_journal_print(LOG_ERR,
                         "IPMI GetBMCBootFrom request data len invalid, "
                         "received: %d, required: 0\n",
                         *data_len);
        *data_len = 0;
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    GetBMC* resData = reinterpret_cast<GetBMC*>(response);

    if (std::filesystem::exists("/run/openbmc/boot_from_backup"))
    {
        resData->readBootAtNum = backupBMC;
    }
    else
    {
        resData->readBootAtNum = primaryBMC;
    }
    *data_len = sizeof(GetBMC);

    return IPMI_CC_OK;
}

inline void setUbootEnv(std::string foo, std::string bar)
{
    std::string cmd = "/sbin/fw_setenv";
    cmd = cmd + " " + foo + " " + bar;
    FILE *fp = popen(cmd.c_str(), "r");
    pclose(fp);
}

/**
*   Get BMC Boot from Information
*   NetFn: 0x3E / CMD: 0xBB
*   Request:
*           - 1: Primary
*           - 2: Backup
**/
ipmi_ret_t ipmiSetBmcBootFrom(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                              ipmi_request_t request, ipmi_response_t response,
                              ipmi_data_len_t data_len, ipmi_context_t context)
{
    if (*data_len != 1)
    {
        sd_journal_print(LOG_ERR,
                         "IPMI GetBMCBootFrom request data len invalid, "
                         "received: %d, required: 0\n",
                         *data_len);
        *data_len = 0;
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }

    auto req = reinterpret_cast<bmcBootFromReq*>(request);

    if (req->bmcBootFrom != primaryBMC &&
        req->bmcBootFrom != backupBMC)
    {
        return IPMI_CC_PARM_OUT_OF_RANGE;
    }

    if (std::filesystem::exists("/run/openbmc/boot_from_backup"))
    {
        if (req->bmcBootFrom == primaryBMC)
        {
            setUbootEnv("switchBMC", "true");
        }
        else
        {
            setUbootEnv("switchBMC", "");
        }
    }
    else
    {
        if (req->bmcBootFrom == backupBMC)
        {
            setUbootEnv("switchBMC", "true");
        }
        else
        {
            setUbootEnv("switchBMC", "");
        }
    }

    *data_len = 0;

    return IPMI_CC_OK;
}

uint8_t restoreSensorThreshold()
{
    std::filesystem::remove_all(sensorThresholdPath);
    std::string sysUnit = "xyz.openbmc_project.EntityManager.service";
    auto bus = sdbusplus::bus::new_default();
    try
    {
        auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH,
                                          SYSTEMD_INTERFACE, "RestartUnit");
        method.append(sysUnit, "replace");
        bus.call_noreply(method);
    }
    catch (std::exception& e)
    {
        return IPMI_CC_RESPONSE_ERROR;
    }
    return 0;
}

uint8_t restoreFanMode()
{
    std::filesystem::remove_all(manualModeFilePath);

    const auto modeService = "xyz.openbmc_project.State.FanCtrl";
    const auto modeRoot = "/xyz/openbmc_project/settings/fanctrl";
    const auto modeIntf = "xyz.openbmc_project.Control.Mode";
    const auto propIntf = "org.freedesktop.DBus.Properties";

    // Bus for system control.
    std::shared_ptr<sdbusplus::asio::connection> bus = getSdBus();

    // Get all zones object path.
    DbusSubTree zonesPath = getSubTree(*bus, modeRoot, 1, modeIntf);
    if (zonesPath.empty() == true)
    {
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    // Set manual property on dbus to false.
    for (auto& path : zonesPath)
    {
        std::variant<bool> value = false;
        auto msg = bus->new_method_call(modeService, path.first.c_str(),
                                        propIntf, "Set");
        msg.append(modeIntf, "Manual", value);

        try
        {
            bus->call_noreply(msg);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            return IPMI_CC_RESPONSE_ERROR;
        }
    }

    return 0;
}

uint8_t restorePowerCycleInterval()
{
    std::string defaultConfigPath = "/run/initramfs/ro";
    std::string configPath =
        "/etc/default/obmc/phosphor-reboot-host/reboot.conf";

    // read default setting of restore power cycle from read only files
    defaultConfigPath = defaultConfigPath + configPath;
    std::ifstream fin(defaultConfigPath);
    if (fin.is_open() == false)
    {
        sd_journal_print(LOG_ERR, "[%s] failed to open file %s\n", __FUNCTION__,
                         defaultConfigPath);
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    std::string defaultPowerCycleIntervalStr;
    fin >> defaultPowerCycleIntervalStr;
    fin.close();

    // write default setting of restore power cycle to reboot config
    std::ofstream fout;
    fout.open(configPath, std::ios::out | std::ios::trunc);
    if (fout.is_open() == false)
    {
        sd_journal_print(LOG_ERR, "[%s] failed to open file %s\n", __FUNCTION__,
                         configPath);
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    fout << defaultPowerCycleIntervalStr << "\n";
    if (fout.fail() == true)
    {
        fout.close();
        sd_journal_print(LOG_ERR, "[%s] failed to write file %s\n",
                         __FUNCTION__, configPath);
        return IPMI_CC_UNSPECIFIED_ERROR;
    }

    fout.close();

    return 0;
}

/**
 *   Get BMC Boot from Information
 *   NetFn: 0x30 / CMD: 0x19
 *   @param[in] restore item
 *   - 00h = restore all item
 *   - 01h = restore BMC sensor threshold
 *   - 02h = restore fan mode
 *   - 03h = restore power interval
 *
 *   @return Byte 1: Completion Code.
 **/
ipmi_ret_t IpmiRestoreToDefualt(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                                ipmi_request_t request,
                                ipmi_response_t response,
                                ipmi_data_len_t data_len,
                                ipmi_context_t context)
{
    if (*data_len != 1)
    {
        sd_journal_print(LOG_ERR,
                         "IPMI RestoreToDefualt request data len invalid, "
                         "received: %d, required: 0\n",
                         *data_len);
        *data_len = 0;
        return IPMI_CC_REQ_DATA_LEN_INVALID;
    }
    *data_len = 0;

    auto req = reinterpret_cast<RestoreToDefaultReq*>(request);

    if (req->restoreItem == allItemRestore)
    {
        bool isFncFail = false;
        int ret = restoreFanMode();
        if (ret != 0)
        {
            std::cerr << __FUNCTION__ << ": fail to restore fan mode."
                      << "\n";
            isFncFail = true;
        }

        ret = restoreSensorThreshold();
        if (ret != 0)
        {
            std::cerr << __FUNCTION__ << ": fail to restore sensor threshold."
                      << "\n";
            isFncFail = true;
        }

        ret = restorePowerCycleInterval();
        if (ret != 0)
        {
            std::cerr << __FUNCTION__
                      << ": fail to restore power cycle interval."
                      << "\n";
            isFncFail = true;
        }

        if (isFncFail)
        {
            return IPMI_CC_UNSPECIFIED_ERROR;
        }
    }
    else if (req->restoreItem == restoreItemSensorThreshold)
    {
        int ret = restoreSensorThreshold();
        if (ret != 0)
        {
            std::cerr << __FUNCTION__ << ": fail to restore sensor threshold."
                      << "\n";
            return ret;
        }
    }
    else if (req->restoreItem == restoreItemFanMode)
    {
        int ret = restoreFanMode();
        if (ret != 0)
        {
            std::cerr << __FUNCTION__ << ": fail to restore fan mode."
                      << "\n";
            return ret;
        }
    }
    else if (req->restoreItem == restoreItemPowerInterval)
    {
        int ret = restorePowerCycleInterval();
        if (ret != 0)
        {
            std::cerr << __FUNCTION__
                      << ": fail to restore power cycle interval."
                      << "\n";
            return ret;
        }
    }
    else
    {
        return IPMI_CC_INVALID_FIELD_REQUEST;
    }
    return IPMI_CC_OK;
}

ipmi_ret_t ipmiOBMCNotSupport(ipmi_netfn_t netfn, ipmi_cmd_t cmd,
                              ipmi_request_t request, ipmi_response_t response,
                              ipmi_data_len_t data_len, ipmi_context_t context)
{
    return IPMI_CC_NOT_SUPPORTED_IN_PRESENT_STATE;
}

static void register_oem_functions(void)
{

    // <Set Service>
    ipmi_register_callback(netFnSv310aOEM2, CMD_SET_SERVICE, NULL,
                           ipmiSetService, PRIVILEGE_USER);
    // <Get Service>
    ipmi_register_callback(netFnSv310aOEM2, CMD_GET_SERVICE, NULL,
                           ipmiGetService, PRIVILEGE_USER);
    // <Set GPIO>
    ipmi_register_callback(netFnSv310aOEM2, CMD_SET_GPIO, NULL, IpmiSetGpio,
                           PRIVILEGE_USER);
    // <Get GPIO>
    ipmi_register_callback(netFnSv310aOEM2, CMD_GET_GPIO, NULL, IpmiGetGpio,
                           PRIVILEGE_USER);
    // <Restore to Default>
    ipmi_register_callback(netFnSv310aOEM2, CMD_RESTORE_TO_DEFAULT, NULL,
                           IpmiRestoreToDefualt, PRIVILEGE_USER);

    // <Set Fan PWM>
    ipmi::registerHandler(ipmi::prioOemBase, netFnSv310aOEM2, CMD_SET_FAN_PWM,
                          ipmi::Privilege::Admin, IpmiSetPwm);

    // <Set FSC Mode>
    ipmi::registerHandler(ipmi::prioOemBase, netFnSv310aOEM2, CMD_SET_FSC_MODE,
                          ipmi::Privilege::Admin, IpmiSetFscMode);

    // <Set Leaky Bucket Threshold>
    ipmi_register_callback(netFnSv310aOEM3, CMD_SET_LBA_THRESHOLD, NULL,
                           ipmiSetLBAThreshold, PRIVILEGE_USER);

    // <Get Leaky Bucket Threshold>
    ipmi_register_callback(netFnSv310aOEM3, CMD_GET_LBA_THRESHOLD, NULL,
                           ipmiGetLBAThreshold, PRIVILEGE_USER);

    // <Get Total Correctable ECC Counter Value>
    ipmi_register_callback(netFnSv310aOEM3, CMD_GET_LBA_TOTAL_COUNTER, NULL,
                           ipmiGetLBATotalCounter, PRIVILEGE_USER);

    // <Get Relative Correctable ECC Counter Value>
    ipmi_register_callback(netFnSv310aOEM3, CMD_GET_LBA_RELATIVE_COUNTER, NULL,
                           ipmiGetLBARelativeCounter, PRIVILEGE_USER);

    // <Random Power On Status>
    ipmi_register_callback(netFnSv310aOEM1, CMD_RANDOM_POWERON_STATUS, NULL,
                           IpmiRandomPowerOnStatus, PRIVILEGE_USER);

    // <Random Power On Switch>
    ipmi_register_callback(netFnSv310aOEM1, CMD_RANDOM_POWERON_SWITCH, NULL,
                           IpmiRandomPowerOnSwitch, PRIVILEGE_USER);

    // <Get Post Code>
    ipmi_register_callback(netFnSv310aOEM2, CMD_GET_POST_CODE, NULL,
                           IpmiGetPostCode, PRIVILEGE_USER);

    // <Get Post End Status>
    ipmi_register_callback(netFnSv310aOEM2, CMD_GET_POST_END_STATUS, NULL,
                           IpmiGetPostEndStatus, PRIVILEGE_USER);

    // <Get Amber LED Status>
    ipmi::registerHandler(ipmi::prioOemBase, netFnSv310aOEM2,
                          CMD_GET_AMBER_LED_STATUS, ipmi::Privilege::User,
                          IpmiGetAmberLedStatus);

    // <Set SOL Pattern>
    ipmi_register_callback(netFnSv310aOEM3, CMD_SET_SOL_PATTERN, NULL,
                           ipmiSetSolPattern, PRIVILEGE_USER);

    // <Get SOL Pattern>
    ipmi_register_callback(netFnSv310aOEM3, CMD_GET_SOL_PATTERN, NULL,
                           ipmiGetSolPattern, PRIVILEGE_USER);

    // <Set BMC Time Sync Mode>
    ipmi_register_callback(netFnSv310aOEM3, CMD_SET_BMC_TIMESYNC_MODE, NULL,
                           ipmiSetBmcTimeSyncMode, PRIVILEGE_USER);

    // <Get VR Version>
    ipmi_register_callback(netFnSv310aOEM4, CMD_GET_VR_VERSION, NULL,
                           ipmiGetVrVersion, PRIVILEGE_USER);

    // <Master Write Read>
    ipmi_register_callback(netFnSv310aOEM3, CMD_MASTER_WRITE_READ, NULL,
                           ipmiMasterWriteRead, PRIVILEGE_USER);

    // <Get BMC Boot from Information>
    ipmi_register_callback(netFnSv310aOEM3, CMD_GET_BMC_BOOT_FROM, NULL,
                           ipmiGetBmcBootFrom, PRIVILEGE_USER);

    // <Set BMC Boot from Information>
    ipmi_register_callback(netFnSv310aOEM3, CMD_SET_BMC_BOOT_FROM,
                           NULL, ipmiSetBmcBootFrom, PRIVILEGE_USER);

    // <Get FW version>
    ipmi_register_callback(netFnSv310aOEM4, CMD_GET_FW_VERSION,
                           NULL, ipmiGetFwVersion, PRIVILEGE_USER);

    // <Access CPLD JTAG>
    ipmi_register_callback(netFnSv310aOEM4, CMD_ACCESS_CPLD_JTAG, NULL,
                           ipmiAccessCpldJtag, PRIVILEGE_USER);

    // <Get CPU Power Consumption>
    ipmi_register_callback(netFnSv310aOEM5, CMD_GET_CPU_POWER, NULL,
                           ipmiGetCpuPower, PRIVILEGE_USER);

    // <Get CPU Power Limit>
    ipmi_register_callback(netFnSv310aOEM5, CMD_GET_CPU_POWER_LIMIT, NULL,
                           ipmiGetCpuPowerLimit, PRIVILEGE_USER);

    // <Set System Power Limit>
    ipmi_register_callback(netFnSv310aOEM5, CMD_SET_SYSTEM_POWER_LIMIT, NULL,
                           ipmiSetSystemPowerLimit, PRIVILEGE_USER);

    // <Set Default CPU Power Limit>
    ipmi_register_callback(netFnSv310aOEM5, CMD_SET_DEFAULT_CPU_POWER_LIMIT,
                           NULL, ipmiSetDefaultCPUPowerLimit, PRIVILEGE_USER);

    // <Get System Power Limit>
    ipmi_register_callback(netFnSv310aOEM5, CMD_GET_SYSTEM_POWER_LIMIT, NULL,
                           ipmiGetSystemPowerLimit, PRIVILEGE_USER);

    // <Set CPU Power Limit>
    ipmi_register_callback(netFnSv310aOEM5, CMD_SET_CPU_POWER_LIMIT, NULL,
                           ipmiSetCPUPowerLimit, PRIVILEGE_USER);

    // <APML Write Package Power Limit>
    ipmi_register_callback(netFnSv310aOEM6, CMD_APML_WRITE_PACKAGE_POWER_LIMIT,
                           NULL, ipmiOBMCNotSupport, PRIVILEGE_USER);

    // <APML Read Package Power Consumption>
    ipmi_register_callback(netFnSv310aOEM6,
                           CMD_APML_READ_PACKAGE_POWER_CONSUMPTION, NULL,
                           ipmiOBMCNotSupport, PRIVILEGE_USER);
}
