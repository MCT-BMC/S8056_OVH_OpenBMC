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
#include <cpld-update-utils.hpp>

constexpr auto DBUS_PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";

namespace power
{
static constexpr const char* busName = "xyz.openbmc_project.State.Chassis";
static constexpr const char* path = "/xyz/openbmc_project/state/chassis0";
static constexpr const char* interface = "xyz.openbmc_project.State.Chassis";
static constexpr const char* request = "RequestedHostTransition";
static constexpr const char* current = "CurrentPowerState";
} // namespace power

auto bus = sdbusplus::bus::new_default_system();

void setOutput(std::string lineName,int setValue)
{
    int value;
    struct gpiod_line *line = nullptr;;

    line = gpiod_line_find(lineName.c_str());
    if (gpiod_line_request_output(line,"set-output",setValue) < 0)
    {
        std::cerr << "Error request line" << std::endl;;
    }
    gpiod_line_release(line);
}

int initiateStateTransition(std::string power)
{
    std::variant<std::string> state("xyz.openbmc_project.State.Chassis.Transition." + power);
    try
    {
        auto method = bus.new_method_call(power::busName, power::path,
                                          DBUS_PROPERTY_INTERFACE, "Set");
        method.append(power::interface, power::request , state);

        bus.call_noreply(method);
    }
    catch (const std::exception&  e)
    {
        std::cerr << "Error in " << __func__ << std::endl;
        return -1;
    }
     return 0;
}

bool checkHostPower()
{
    std::variant<std::string> state;

    auto method = bus.new_method_call(power::busName, power::path,
                                      DBUS_PROPERTY_INTERFACE, "Get");
    method.append(power::interface, power::current);
    try
    {
        auto reply = bus.call(method);
        reply.read(state);
    }
    catch (const std::exception&  e)
    {
        std::cerr << "Error in " << __func__ << std::endl;
        return false;
    }

    return boost::ends_with(std::get<std::string>(state), "Running");
}

void logUpdateSel(unsigned int userCode)
{
    std::string selectSensor;
    switch(userCode)
    {
        case 3:
            //Current using cpld device: S8053 DC-SCM CPLD
            selectSensor = "SCM_CPLD_UPDATE";
            break;
        case 4:
             //Current using cpld device: S8053 MB CPLD
            selectSensor = "MB_CPLD_UPDATE";
            break;
        default:
            return;
    }

    CpldUpdateUtil::logSelEvent(selectSensor + " SEL Entry",
        "/xyz/openbmc_project/sensors/versionchange/" + selectSensor,
        0x07,0x00,0xFF);
}