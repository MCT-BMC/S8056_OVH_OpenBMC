#include <iostream>
#include <variant>
#include <vector>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <gpiod.h>
#include <openbmc/libmisc.h>

#define PROPERTY_INTERFACE "org.freedesktop.DBus.Properties"

static constexpr char const* ipmiSELService =    "xyz.openbmc_project.Logging.IPMI";
static constexpr char const* ipmiSELPath = "/xyz/openbmc_project/Logging/IPMI";
static constexpr char const* ipmiSELAddInterface = "xyz.openbmc_project.Logging.IPMI";

static constexpr char const* powerService = "xyz.openbmc_project.State.Host";
static constexpr char const* powerPath = "/xyz/openbmc_project/state/host0";
static constexpr char const* powerInterface = "xyz.openbmc_project.State.Host";

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

    if (event == "prochot")
    {
        eventData[0] = 0x0A;

#ifdef PROCHOT_ENABLE_POWER_FILTER
        struct gpiod_line *gpioPostStart = nullptr;;
        gpioPostStart = gpiod_line_find("POST_START");

        if (gpiod_line_request_input(gpioPostStart,"set-intput") < 0)
        {
            std::cerr << "Error request line\n";
            return 0;
        }

        if(gpiod_line_get_value(gpioPostStart) != 0)
        {
            std::cerr << "Filter out thermtrip event (POST_START)\n";
            return 0;
        }
        gpiod_line_release(gpioPostStart);

        uint32_t registerBuffer = 0;
        if (read_register(0x1e780080,&registerBuffer) < 0)
        {
            std::cerr<<"Failed to read register \n";
            return 0;
        }

        if((registerBuffer & 0x00020000) == 0)
        {
            std::cerr << "Filter out thermtrip event (PS_PWROK)\n";
            return 0;
        }
#endif

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

    }
    else if (event == "thermtrip")
    {
        eventData[0] = 0x01;

#ifdef THERMTRIP_ENABLE_POWER_FILTER
        struct gpiod_line *gpioPostStart = nullptr;;
        gpioPostStart = gpiod_line_find("POST_START");

        if (gpiod_line_request_input(gpioPostStart,"set-intput") < 0)
        {
            std::cerr << "Error request line\n";
            return 0;
        }

        if(gpiod_line_get_value(gpioPostStart) != 0)
        {
            std::cerr << "Filter out thermtrip event (POST_START)\n";
            return 0;
        }
        gpiod_line_release(gpioPostStart);

        uint32_t registerBuffer = 0;
        if (read_register(0x1e780080,&registerBuffer) < 0)
        {
            std::cerr<<"Failed to read register \n";
            return 0;
        }

        if((registerBuffer & 0x00020000) == 0)
        {
            std::cerr << "Filter out thermtrip event (PS_PWROK)\n";
            return 0;
        }
#endif

    }
    else
    {
        std::cerr << "Unexpected parameters" << std::endl;
        return 0;
    }

    sdbusplus::message::message addSEL = bus.new_method_call(
                     ipmiSELService, ipmiSELPath, ipmiSELAddInterface, "IpmiSelAdd");
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
