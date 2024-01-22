#pragma once

#include <iostream>
#include <chrono>
#include <ctime>
#include <fstream>
#include <string>
#include <gpiod.h>
#include <fcntl.h>
#include <sdbusplus/bus.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace ipmi
{
static constexpr const char* busName = "xyz.openbmc_project.Logging.IPMI";
static constexpr const char* path = "/xyz/openbmc_project/Logging/IPMI";
static constexpr const char* interface = "xyz.openbmc_project.Logging.IPMI";
static constexpr const char* request = "IpmiSelAdd";
} // namespace ipmi

auto bus = sdbusplus::bus::new_default_system();

static void logSelEvent(std::string enrty, std::string path ,
                        uint8_t eventData0, uint8_t eventData1, uint8_t eventData2)
{
    std::vector<uint8_t> eventData(3, 0xFF);
    eventData[0] = eventData0;
    eventData[1] = eventData1;
    eventData[2] = eventData2;
    uint16_t genid = 0x20;

    sdbusplus::message::message writeSEL = bus.new_method_call(
        ipmi::busName, ipmi::path,ipmi::interface, ipmi::request);
    writeSEL.append(enrty, path, eventData, true, (uint16_t)genid);
    try
    {
        bus.call(writeSEL);
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to log the button event:" << e.what() << "\n";
    }
}