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

static constexpr char const* generalSensorPath = "/xyz/openbmc_project/sensors/board/";

static constexpr size_t selEvtDataMaxSize = 3;

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


    if (event == "Absent")
    {
        eventData[0] = 0x00;
    }
    else if (event == "Present")
    {
        eventData[0] = 0x01;
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
