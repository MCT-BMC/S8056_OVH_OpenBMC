#pragma once

#include <iostream>
#include <sdbusplus/bus.hpp>
#include <boost/algorithm/string/predicate.hpp>

constexpr auto DBUS_PROPERTY_INTERFACE = "org.freedesktop.DBus.Properties";

namespace chassis
{
static constexpr const char* busName = "xyz.openbmc_project.State.Chassis";
static constexpr const char* path = "/xyz/openbmc_project/state/chassis0";
static constexpr const char* interface = "xyz.openbmc_project.State.Chassis";
static constexpr const char* prop = "CurrentPowerState";
} // namespace chassis

namespace power
{
static constexpr const char* busName = "xyz.openbmc_project.State.Host";
static constexpr const char* path = "/xyz/openbmc_project/state/host0";
static constexpr const char* interface = "xyz.openbmc_project.State.Host";
static constexpr const char* request = "RequestedHostTransition";
} // namespace power

namespace ipmi
{
static constexpr const char* busName = "xyz.openbmc_project.Logging.IPMI";
static constexpr const char* path = "/xyz/openbmc_project/Logging/IPMI";
static constexpr const char* interface = "xyz.openbmc_project.Logging.IPMI";
static constexpr const char* request = "IpmiSelAdd";
} // namespace ipmi

auto bus = sdbusplus::bus::new_default_system();

bool isPowerOn()
{
    auto bus = sdbusplus::bus::new_default();
    auto method = bus.new_method_call(chassis::busName, chassis::path, DBUS_PROPERTY_INTERFACE,"Get");
    method.append(chassis::interface, chassis::prop);
    std::variant<std::string> result;
    try
    {
        auto reply = bus.call(method);
        reply.read(result);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        return false;
    }

    bool on = boost::ends_with(std::get<std::string>(result), "On");

    if(on)
    {
        return true;
    }

    return false;
}

int initiateStateTransition(std::string power)
{
    std::variant<std::string> state("xyz.openbmc_project.State.Host.Transition." + power);
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

int setBmcService(std::string service,std::string status, std::string setting)
{
    constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
    constexpr auto SYSTEMD_OBJ_PATH = "/org/freedesktop/systemd1";
    constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

    auto bus = sdbusplus::bus::new_default();

    // start/stop servcie
    if(status != "Null"){
        auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH, SYSTEMD_INTERFACE, status.c_str());
        method.append(service, "replace");

        try
        {
            auto reply = bus.call(method);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            std::cerr << "Error in start/stop servcie. Error" << e.what() << std::endl;
            return -1;
        }
    }

    // enable/disable service
    if(setting != "Null"){
        auto method = bus.new_method_call(SYSTEMD_SERVICE, SYSTEMD_OBJ_PATH, SYSTEMD_INTERFACE,setting.c_str());

        std::array<std::string, 1> appendMessage = {service};

        if(setting == "EnableUnitFiles")
        {
            method.append(appendMessage, false, true);
        }
        else
        {
            method.append(appendMessage, false);
        }

        try
        {
            auto reply = bus.call(method);
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            std::cerr << "Error in enable/disable service. Error" << e.what() << std::endl;
            return -1;
        }
    }

     return 0;
}