#pragma once

#include <openbmc/libobmcdbus.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/server.hpp>

#include <functional>
#include <iostream>
#include <map>
#include <string>

constexpr auto discreteServ = "xyz.openbmc_project.Discrete.Sensor";
constexpr auto discretePathPrefix = "/xyz/openbmc_project/sensors/discrete/";
constexpr auto discreteIntf = "xyz.openbmc_project.Discrete.Sensor";
constexpr auto valueIntf = "xyz.openbmc_project.Sensor.Value";
constexpr auto maxAddSelRetryTime = 3;

std::map<
    std::string,
    std::map<std::string, std::shared_ptr<sdbusplus::asio::dbus_interface>>>
    interfaces;

int addSelWithRetry(std::string message, std::string objPath,
                    std::vector<uint8_t> eventData, bool is_assert,
                    uint8_t retryTime)
{
    if (retryTime >= maxAddSelRetryTime)
    {
        return -1;
    }
    else
    {
        if (ipmiSelAdd(message, objPath, eventData, is_assert) != 0)
        {
            return addSelWithRetry(message, objPath, eventData, is_assert,
                                   (retryTime + 1));
        }
    }

    return 0;
}

int setEndofPOST(int32_t event, uint32_t setValue)
{
    std::string objPath = discretePathPrefix + std::string("End_of_POST");
    uint8_t retryTime = 0;
    uint32_t gpioLo = 0;

    std::vector<uint8_t> eventData(3, 0xFF);
    std::string message;

    message = "End_of_POST";

    auto ifaceFind = interfaces.find(objPath);
    auto valueIntfFind = ifaceFind->second.find(valueIntf);
    if (valueIntfFind != ifaceFind->second.end())
    {
        auto iface = valueIntfFind->second;
        iface->set_property("Value", setValue);
    }
    else
    {
        std::cerr << "Cannot find interface: " << valueIntf << "\n";
    }

    eventData[0] = static_cast<uint8_t>(event);
    if (setValue == gpioLo)
    {
        if (addSelWithRetry(message, objPath, eventData, true, retryTime) != 0)
        {
            std::cerr << "Fail to add End_of_POST SEL \n";
        }
    }
    return 0;
}

// For dbus method registration
std::map<std::string, int32_t> funcMap = {{"setEndofPOST", 0}};
// no 4 exist
std::vector<std::function<int(int32_t, uint32_t)>> fnVec = {setEndofPOST};

int32_t getFunctionMap(std::string funcName)
{
    int32_t idx = 0;

    idx = funcMap[funcName];

    return idx;
}
