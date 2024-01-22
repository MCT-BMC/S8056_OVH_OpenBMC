#include "Sensor.hpp"

extern std::map<
    std::string,
    std::map<std::string, std::shared_ptr<sdbusplus::asio::dbus_interface>>>
    interfaces;

Sensor::Sensor(std::string object_path,
               std::shared_ptr<sdbusplus::asio::dbus_interface> interface,
               std::string sensor_name) :
    object_path(object_path),
    interface(interface), sensor_name(sensor_name), pre_value(0), value(0)
{}

void Sensor::addSel(std::string message, std::string object_path,
                    std::vector<uint8_t> event_data, bool is_assert)
{
    try
    {
        auto bus = sdbusplus::bus::new_system();
        auto getMessage = bus.new_method_call(IPMI_SEL_SERVICE, IPMI_SEL_PATH,
                                              IPMI_SEL_INTERFACE, "IpmiSelAdd");

        getMessage.append(message, object_path, event_data, is_assert,
                          selBMCGenID);

        bus.call_noreply(getMessage);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        std::cerr << "Error to Add Sel : " << message
                  << ",error code : " << e.what() << "\n";
    }
}
