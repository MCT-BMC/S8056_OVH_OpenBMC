#include "mct-power-handler.hpp"

static constexpr bool DEBUG = false;
bool firstAccess = true;

void setSensorStatus(std::string propertie, uint32_t control)
{
    auto systemBus = sdbusplus::bus::new_default();
    try
    {
        auto method = systemBus.new_method_call(sensorStatus::busName,
                      sensorStatus::path,PROPERTIES_INTERFACE,"Set");
        method.append(sensorStatus::interface, propertie.c_str(), std::variant<uint32_t>(control));
        systemBus.call(method);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << "Failed to change sensor status. ERROR=" << e.what();
    }
}

bool isPostEnd()
{
    auto bus = sdbusplus::bus::new_default();
    auto method = bus.new_method_call(postComplete::busName, postComplete::path, PROPERTIES_INTERFACE, "Get");
    method.append(postComplete::interface, postComplete::request);
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

    bool enable = boost::ends_with(std::get<std::string>(result), "Standby");

    if(enable)
    {
        return true;
    }

    return false;
}

void checkDimmStatus()
{
    for(uint8_t i=0; i<10; i++)
    {
        if constexpr (DEBUG)
        {
            std::cerr << "Check BMC dimm sensor status loop..." << std::endl;
        }
        if(!isPostEnd())
        {
            std::cerr << "Disable BMC dimm sensor switching feature." << std::endl;
            setSensorStatus("DIMMSensorStatus",0x00);
            break;
        }

        if(i >= 5)
        {
            std::cerr << "Enable BMC dimm sensor switching feature." << std::endl;
            setSensorStatus("DIMMSensorStatus",0x01);
            break;
        }
        sleep(2);
    }

    return;
}

void dimmStatusSetting(bool on)
{
    std::cerr << "Check BMC dimm sensor status..." << std::endl;

    if(!on)
    {
        std::cerr << "Disable BMC dimm sensor switching feature." << std::endl;
        setSensorStatus("DIMMSensorStatus",0x00);
        return;
    }

    std::thread checkDimmStatusThread(checkDimmStatus);
    checkDimmStatusThread.detach();
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

void defaultPowerState(std::shared_ptr<sdbusplus::asio::connection>& systemBus)
{
    if (!firstAccess)
    {
        return;
    }
    firstAccess = false;
    auto method = systemBus->new_method_call(chassis::busName, chassis::path,PROPERTIES_INTERFACE,"Get");
    method.append(chassis::interface, chassis::request);
    std::variant<std::string> result;
    try
    {
        auto reply = systemBus->call(method);
        reply.read(result);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << "Error in power status get. Error:" << e.what() << std::endl;
        sleep(5);
        defaultPowerState(systemBus);
        return;
    }

    bool on = boost::ends_with(std::get<std::string>(result), "On");

    if(on)
    {
        setBmcService("obmc-chassis-poweron.target","StartUnit","Null");
    }
    else
    {
        setBmcService("obmc-chassis-poweroff.target","StartUnit","Null");
    }

    dimmStatusSetting(on);
}

inline static sdbusplus::bus::match::match
    setPowerHandler(std::shared_ptr<sdbusplus::asio::connection>& systemBus)
{
    auto powerMatcherCallback = [&](sdbusplus::message::message &msg)
    {
            bool value;
            msg.read(value);

            if(value)
            {
                //set power on handler

                setBmcService("obmc-chassis-poweron.target","StartUnit","Null");
                setBmcService("obmc-host-poweron.target","StartUnit","Null");
            }
            else
            {
                //set power off handler
                setBmcService("obmc-chassis-poweroff.target","StartUnit","Null");
                setBmcService("obmc-host-poweroff.target","StartUnit","Null");
            }

            if (firstAccess)
            {
                defaultPowerState(systemBus);
            }
    };
    sdbusplus::bus::match::match powerMatcher(
        static_cast<sdbusplus::bus::bus &>(*systemBus),
        sdbusplus::bus::match::rules::type::signal() +
        sdbusplus::bus::match::rules::member(std::string("PowerTrigger")),
        std::move(powerMatcherCallback));
    return powerMatcher;
}

int main()
{
    boost::asio::io_service io;
    auto systemBus = std::make_shared<sdbusplus::asio::connection>(io);

    sdbusplus::bus::match::match powerHandler = setPowerHandler(systemBus);
    defaultPowerState(systemBus);

    io.run();

    return 0;
}