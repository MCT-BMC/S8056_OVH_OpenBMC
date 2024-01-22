#pragma once

#include <iostream>
#include <vector>
#include <filesystem>
#include <regex>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sdbusplus/bus.hpp>
#include <phosphor-logging/elog-errors.hpp>
#include <phosphor-logging/log.hpp>

namespace fs = std::filesystem;
using namespace phosphor::logging;
using sdbusplus::exception::SdBusError;

std::shared_ptr<sdbusplus::asio::connection> bus;
std::shared_ptr<sdbusplus::asio::dbus_interface> powerInterface;

using DbusProperty = std::string;
using Value = std::variant<bool, uint8_t, int16_t, uint16_t, int32_t, uint32_t,
                           int64_t, uint64_t, double, std::string>;
using PropertyMap = std::map<DbusProperty, Value>;

constexpr auto IPMI_SEL_Service =    "xyz.openbmc_project.Logging.IPMI";
constexpr auto IPMI_SEL_Path = "/xyz/openbmc_project/Logging/IPMI";
constexpr auto IPMI_SEL_Add_Interface = "xyz.openbmc_project.Logging.IPMI";
constexpr auto HOST_STATE_PATH ="/xyz/openbmc_project/state/host0";
constexpr auto HOST_INTERFACE = "xyz.openbmc_project.State.Host";
constexpr auto PROP_INTERFACE = "org.freedesktop.DBus.Properties";
constexpr auto METHOD_SET = "Set";
constexpr auto METHOD_GET_ALL = "GetAll";


/**
 * @brief Find path accroding to match string.
 *
 * @param[in] dirPath - Directory path.
 * @param[in] matchString - String for match.
 * @param[in] foundPaths - Found match path.
 * @param[in] symlinkDepth - Symlink for directory depth.
 * @return On success returns true.
 */
bool findFiles(const fs::path dirPath, const std::string& matchString,
               std::vector<fs::path>& foundPaths, unsigned int symlinkDepth)
{
    if (!fs::exists(dirPath))
        return false;

    std::regex search(matchString);
    std::smatch match;
    for (auto& p : fs::recursive_directory_iterator(dirPath))
    {
        std::string path = p.path().string();
        if (!is_directory(p))
        {
            if (std::regex_search(path, match, search))
                foundPaths.emplace_back(p.path());
        }
        else if (is_symlink(p) && symlinkDepth)
        {
            findFiles(p.path(), matchString, foundPaths, symlinkDepth - 1);
        }
    }
    return true;
}

/**
 * @brief Get current timestamp in milliseconds.
 * 
 * @param[in] Null.
 * @return current timestamp in milliseconds.
 */
double getCurrentTimeWithMs()
{
    time_t s;
    long ms;
    struct timespec spec;

    clock_gettime(CLOCK_REALTIME, &spec);



    s  = spec.tv_sec;
    ms = round(spec.tv_nsec / 1.0e6); // Convert nanoseconds to milliseconds
    if (ms > 999) {
        s++;
        ms = 0;
    }

    double  result = (intmax_t)s + ((double)ms/1000);

    return result;
}

/**
 * @brief Get the DBUS Service name for the input dbus path.

 * @param[in] bus - DBUS Bus Object.
 * @param[in] intf - DBUS Interface.
 * @param[in] path - DBUS Object Path.
 */
std::string getService(std::shared_ptr<sdbusplus::asio::connection>& bus, const std::string& intf,
                       const std::string& path)
{
    auto mapperCall =
        bus->new_method_call("xyz.openbmc_project.ObjectMapper",
                            "/xyz/openbmc_project/object_mapper",
                            "xyz.openbmc_project.ObjectMapper", "GetObject");

    mapperCall.append(path);
    mapperCall.append(std::vector<std::string>({intf}));

    auto mapperResponseMsg = bus->call(mapperCall);

    if (mapperResponseMsg.is_method_error())
    {
        throw std::runtime_error("ERROR in mapper call");
    }

    std::map<std::string, std::vector<std::string>> mapperResponse;
    mapperResponseMsg.read(mapperResponse);

    if (mapperResponse.begin() == mapperResponse.end())
    {
        throw std::runtime_error("ERROR in reading the mapper response");
    }

    return mapperResponse.begin()->first;
}

/** @brief Gets all the properties associated with the given object
 *         and the interface.
 *  @param[in] bus - DBUS Bus Object.
 *  @param[in] service - Dbus service name.
 *  @param[in] objPath - Dbus object path.
 *  @param[in] interface - Dbus interface.
 *  @return On success returns the map of name value pair.
 */
PropertyMap getAllDbusProperties(std::shared_ptr<sdbusplus::asio::connection>& bus,
                                 const std::string& service,
                                 const std::string& objPath,
                                 const std::string& interface,
                                 std::chrono::microseconds timeout)
{
    PropertyMap properties;

    auto method = bus->new_method_call(service.c_str(), objPath.c_str(),
                                      PROP_INTERFACE, METHOD_GET_ALL);

    method.append(interface);

    auto reply = bus->call(method, timeout.count());

    if (reply.is_method_error())
    {
        log<level::ERR>("Failed to get all properties",
                        entry("PATH=%s", objPath.c_str()),
                        entry("INTERFACE=%s", interface.c_str()));
    }

    reply.read(properties);
    return properties;
}

/**
 * @brief Generate SEL event.
 *
 * @param[in] bus - DBUS Bus Object.
 * @param[in] SensorPath - Dbus service name of sensor.
 * @param[in] eventData - SEL event data 1 to 3.
 */
void generateSELEvent(std::shared_ptr<sdbusplus::asio::connection>& bus,char const* SensorPath,std::vector<uint8_t> eventData, bool assert)
{
    uint16_t generateID = 0x20;

    sdbusplus::message::message writeSEL = bus->new_method_call(
                            IPMI_SEL_Service, IPMI_SEL_Path, IPMI_SEL_Add_Interface, "IpmiSelAdd");
    writeSEL.append("SEL Entry",  std::string(SensorPath), eventData, assert,generateID);
    try
    {
        bus->call(writeSEL);
    }
    catch (sdbusplus::exception_t& e)
    {
        std::cerr << "call IpmiSelAdd failed\n";
    }
}

/**
 * @brief Control host power state.
 *
 * @param[in] bus - DBUS Bus Object.
 * @param[in] control - Power state.
 */
void hostPowerControl(std::shared_ptr<sdbusplus::asio::connection>& bus, std::string control)
{
    try
    {
        auto service = getService(bus, HOST_INTERFACE, HOST_STATE_PATH);
        
        auto method =  bus->new_method_call(
            service.c_str(), HOST_STATE_PATH, PROP_INTERFACE, METHOD_SET);
        method.append(HOST_INTERFACE, "RequestedHostTransition", std::variant<std::string>(control));

        bus->call(method);
    }
    catch (SdBusError& e)
    {
        log<level::ERR>("Failed to change power state", entry("ERROR=%s", e.what()));
    }
}