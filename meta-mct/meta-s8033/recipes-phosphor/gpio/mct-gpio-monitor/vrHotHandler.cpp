#include <iostream>
#include <variant>
#include <vector>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <boost/algorithm/string.hpp>

#define PROPERTY_INTERFACE "org.freedesktop.DBus.Properties"

namespace sel
{
    static constexpr const char* busName = "xyz.openbmc_project.Logging.IPMI";
    static constexpr const char* path = "/xyz/openbmc_project/Logging/IPMI";
    static constexpr const char* interface = "xyz.openbmc_project.Logging.IPMI";
} // namespace sel

namespace power
{
    static constexpr const char* busName = "xyz.openbmc_project.State.Chassis";
    static constexpr const char* path = "/xyz/openbmc_project/state/chassis0";
    static constexpr const char* interface = "xyz.openbmc_project.State.Chassis";
    static constexpr const char* prop = "CurrentPowerState";
} // namespace power

static constexpr const char* PRCOHOT_SERVICE = "xyz.openbmc_project.eventSensor";
static constexpr const char* PRCOHOT_PATH = "/xyz/openbmc_project/sensors/processor/CPU_State";
static constexpr const char* PRCOHOT_INTERFACE = "xyz.openbmc_project.Sensor.Value";

static constexpr char const* generalSensorPath = "/xyz/openbmc_project/sensors/processor/";

static constexpr size_t selEvtDataMaxSize = 3;

#ifdef ENABLE_PROCHOT_ASSET

namespace prochot
{
    static constexpr const char* busName = "xyz.openbmc_project.mct.prochot";
    static constexpr const char* path = "/xyz/openbmc_project/mct/prochot";
    static constexpr const char* interface = "xyz.openbmc_project.mct.prochot";
} // namespace prochot


static void sendDbusMethod(const std::string& service, const std::string& path,
                          const std::string& interface, const std::string& dbusMethod)
{
    auto bus = sdbusplus::bus::new_default();
    auto method = bus.new_method_call(service.c_str(), path.c_str(), interface.c_str(), dbusMethod.c_str());
    bus.call_noreply(method);
}
#endif

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        std::cerr << "Unexpected parameters" << std::endl;
        return 0;
    }

    auto bus = sdbusplus::bus::new_default();

    std::string type(argv[1]);
    std::string sensor(argv[2]);
    std::string event(argv[3]);

    uint16_t genId = 0x20;
    bool assert= (type=="assert") ? 1:0;
    std::vector<uint8_t> eventData(selEvtDataMaxSize, 0xFF);
    std::string senorPath = std::string(generalSensorPath) + sensor;

    std::string ipmiSELAddMessage = sensor + " SEL Entry";

#ifdef ENABLE_VR_POWER_FILTER
    auto method = bus.new_method_call(power::busName, power::path, PROPERTY_INTERFACE, "Get");
    method.append(power::interface, power::prop);
    std::variant<std::string> result;
    try
    {
        auto reply = bus.call(method);
        reply.read(result);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        return 0;
    }

    bool on = boost::ends_with(std::get<std::string>(result), "On");

    if(!on)
    {
        return 0;
    }
#endif

    if (event == "CPU_VRHOT" || event == "SOC_VRHOT" || event == "MEM_ABCD_VRHOT" || event == "MEM_EFGH_VRHOT")
    {
#ifdef ENABLE_PROCHOT_ASSET
        if(assert)
        {
            sendDbusMethod(prochot::busName, prochot::path, prochot::interface, "increaseProchotAsset");
        }
        else
        {
            sendDbusMethod(prochot::busName, prochot::path, prochot::interface, "decreaseProchotAsset");
        }
#endif
        eventData[0] = 0x0A;
    }
    else
    {
        std::cerr << "Unexpected parameters" << std::endl;
        return 0;
    }

    sdbusplus::message::message addSEL = bus.new_method_call(
                     sel::busName, sel::path, sel::interface, "IpmiSelAdd");
    addSEL.append(ipmiSELAddMessage, senorPath, eventData, assert, genId);

    try
    {
        bus.call(addSEL);
    }
    catch (const std::exception& e)
    {
        std::cerr << "ERROR=" << e.what() << "\n";
        return 0;
    }

    return 0;
}
