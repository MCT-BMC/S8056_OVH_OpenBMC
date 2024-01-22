#include <iostream>
#include <variant>
#include <boost/asio/io_context.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <gpiod.hpp>

boost::asio::io_context io;

std::shared_ptr<sdbusplus::asio::connection> bus;
std::shared_ptr<sdbusplus::asio::dbus_interface> iface;

static std::string prochotAssetName = "ASSERT_PROCHOT";
static gpiod::line prochotAssetLine;
int prochotAssetValue;

int prochotStatus = 0;

namespace prochot
{
    static constexpr const char* busName = "xyz.openbmc_project.mct.prochot";
    static constexpr const char* path = "/xyz/openbmc_project/mct/prochot";
    static constexpr const char* interface = "xyz.openbmc_project.mct.prochot";
    static constexpr const char* prochotAssetProp = "prochotAsset";
    static constexpr const char* increaseProchotAssetMethod = "increaseProchotAsset";
    static constexpr const char* decreaseProchotAssetMethod = "decreaseProchotAsset";
} // namespace prochot

static constexpr bool DEBUG = false;


void setProchotStatus(int enable)
{
    if(enable > 0)
    {
        prochotAssetLine.set_value(1);
    }
    else
    {
        prochotAssetLine.set_value(0);
    }

}

int increaseProchotAsset(void)
{
    prochotStatus++;
    iface->set_property(prochot::prochotAssetProp, prochotStatus);
    setProchotStatus(prochotStatus);
    return prochotStatus;
}

int decreaseProchotAsset(void)
{
    if(prochotStatus > 0){
        prochotStatus--;
        iface->set_property(prochot::prochotAssetProp, prochotStatus);
    }
    setProchotStatus(prochotStatus);
    return prochotStatus;
}


void dbusServiceInitialize()
{
    bus->request_name(prochot::busName);
    sdbusplus::asio::object_server objServer(bus);
    iface=objServer.add_interface(prochot::path,prochot::interface);
    iface->register_method(prochot::increaseProchotAssetMethod, increaseProchotAsset);
    iface->register_method(prochot::decreaseProchotAssetMethod, decreaseProchotAsset);
    iface->register_property(prochot::prochotAssetProp, prochotStatus,
                             sdbusplus::asio::PropertyPermission::readWrite);
    iface->initialize();
}

bool gpioInit(const std::string& name,gpiod::line& gpioLine)
{
    gpioLine = gpiod::find_line(name);
    if (!gpioLine)
    {
        std::cerr << "Failed to find the " << name << " line" << std::endl;
        return false;
    }

    try
    {
        gpioLine.request({"prochot-manager", gpiod::line_request::DIRECTION_OUTPUT, 0});
    }
    catch (std::exception&)
    {
        std::cerr << "Failed to request input for " << name << std::endl;
        return false;
    }

    gpioLine.set_value(0);

    return true;
}

int main(void)
{
    bus = std::make_shared<sdbusplus::asio::connection>(io);

    gpioInit(prochotAssetName, prochotAssetLine);
    dbusServiceInitialize();
    
    io.run();
    return 0;
}