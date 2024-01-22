#include "mct-acpi-handler.hpp"

static constexpr bool DEBUG = false;

inline static sdbusplus::bus::match::match 
    setAcpiHandler(std::shared_ptr<sdbusplus::asio::connection>& systemBus)
{
    auto acpiEvntHandler =
        [&](sdbusplus::message::message& message) {
            boost::container::flat_map<std::string, std::variant<std::string>> values;
            std::string objectName;
            if (message.is_method_error())
            {
                std::cerr << "callback method error\n";
                return;
            }

            if (DEBUG)
            {
                std::cout << message.get_path() << " is changed" << std::endl;
            }

            message.read(objectName, values);
            auto findState = values.find("CurrentPowerState");
            if (findState != values.end())
            {
                bool on = boost::ends_with(
                    std::get<std::string>(findState->second), "On");

                std::vector<uint8_t> eventData(3, 0xFF);
                eventData[0] = (on) ? 0:5;
                       
                sdbusplus::message::message writeSEL = systemBus->new_method_call(
                            IPMI_SERVICE, IPMI_PATH, IPMI_INTERFACE, "IpmiSelAdd");
                writeSEL.append("ACPI state SEL Entry",
                                std::string(ACPI_SENSOR_PATH), eventData, true,  (uint16_t)0x20);
                    
                try
                {
                    systemBus->call(writeSEL);
                }
                    catch (sdbusplus::exception_t& e)
                {
                    std::cerr << "Call IpmiSelAdd failed" << std::endl;
                }
               
            }

        };

    sdbusplus::bus::match::match acpiStateMatcher(
        static_cast<sdbusplus::bus::bus&>(*systemBus),
        sdbusplus::bus::match::rules::type::signal() +
        sdbusplus::bus::match::rules::interface(std::string(PROPERTIES_INTERFACE)) +
        sdbusplus::bus::match::rules::path(std::string(CHASSIS_STATE_PATH)) +
        sdbusplus::bus::match::rules::argN(0,std::string(CHASSIS_STATE_INTERFACE)),
        std::move(acpiEvntHandler));
    
    return acpiStateMatcher;
}

int main()
{
    boost::asio::io_service io;
    auto systemBus = std::make_shared<sdbusplus::asio::connection>(io);
    
    sdbusplus::bus::match::match acpiHandler = setAcpiHandler(systemBus);

    io.run();

    return 0;
}