#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/vtable.hpp>
#include <sdbusplus/server/interface.hpp>


#define CATERR_OBJECTPATH_BASE "/xyz/openbmc_project/control/processor"
#define CATERR_SERVICE "xyz.openbmc_project.Settings"
#define CATERR_INTERFACE "xyz.openbmc_project.Control.Processor"
#define PROPERTY_INTERFACE "org.freedesktop.DBus.Properties"


/**
 * @brief Main
 */
int main(int argc, char *argv[])
{

    std::string state="xyz.openbmc_project.Control.Processor.State.CATERR"; 

    auto bus = sdbusplus::bus::new_default();
    auto method = bus.new_method_call(CATERR_SERVICE, CATERR_OBJECTPATH_BASE,
                                      PROPERTY_INTERFACE, "Set");
    method.append(CATERR_INTERFACE,"ProcessorStatus", sdbusplus::message::variant<std::string>(state));
    bus.call_noreply(method);
    return 0;

}
