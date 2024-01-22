#pragma once

#include <iostream>
#include <vector>
#include <fstream>
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

constexpr auto ROOT_PATH = "/";
constexpr auto ROOT_INTERFACE = "org.freedesktop.DBus";

constexpr auto STATUS_PATH = "/xyz/openbmc_project/oem/SensorStatus";
constexpr auto STATUS_INTERFACE = "xyz.openbmc_project.OEM.SensorStatus";
constexpr auto PROP_INTERFACE = "org.freedesktop.DBus.Properties";
constexpr auto METHOD_SET = "Set";

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

/** @brief Read the value from specific path.
 *
 *  @param[in] path - Specific path for reading.
 */
int readFileValue(std::string path)
{
    std::fstream registerFile(path, std::ios_base::in);
    int value;
    registerFile >> value;
    return value;
}

/** @brief Write the value for specific path.
 *
 *  @param[in] path - Specific path for writing.
 *  @param[in] value - Value for writing.
 */
void writeFileValue(std::string path, int value)
{
    std::fstream registerFile(path, std::ios_base::out);

    registerFile << value;
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

/**
 * @brief Send signal to DBUS.
 *
 * @param[in] bus - DBUS Bus Object.
 * @param[in] signal - Send signal.
 * @param[in] parameter - Parameter for send signal.
 */
template <typename T>
void sendDbusSignal(std::shared_ptr<sdbusplus::asio::connection>& bus, std::string signal, T parameter)
{
    try
    {
        auto msg = bus->new_signal(ROOT_PATH, ROOT_INTERFACE, signal.c_str());
        msg.append(parameter);
        msg.signal_send();
    }
    catch (SdBusError& e)
    {
        log<level::ERR>("Failed to send signal to DBUS", entry("ERROR=%s", e.what()));
    }
}

/**
 * @brief Set sensor status.
 *
 * @param[in] bus - DBUS Bus Object.
 * @param[in] signal - Send propertie.
 * @param[in] control - DIMM sensor status.
 */
void setSensorStatus(std::shared_ptr<sdbusplus::asio::connection>& bus, std::string propertie, uint32_t control)
{
    try
    {
        auto service = getService(bus, STATUS_INTERFACE, STATUS_PATH);
        auto method =  bus->new_method_call(
            service.c_str(), STATUS_PATH, PROP_INTERFACE, METHOD_SET);
        method.append(STATUS_INTERFACE, propertie.c_str(), std::variant<uint32_t>(control));
        bus->call(method);
    }
    catch (SdBusError& e)
    {
        log<level::ERR>("Failed to change sensor status", entry("ERROR=%s", e.what()));
    }
}