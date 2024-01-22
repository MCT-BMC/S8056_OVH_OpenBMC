#include "libobmcdbus.hpp"

#include <systemd/sd-journal.h>

static auto bus = sdbusplus::bus::new_system();

int ipmiSelAdd(std::string message, std::string object_path,
               std::vector<uint8_t> event_data, bool is_assert)
{
    auto method = bus.new_method_call(
                              IPMI_SEL_SERVICE, IPMI_SEL_PATH,
                              IPMI_SEL_INTERFACE, "IpmiSelAdd");
    method.append(message, object_path, event_data, is_assert, selBMCGenID);

    try
    {
        bus.call_noreply(method);
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        sd_journal_print(LOG_ERR, "Error to Add Sel : %s, errorcode: %s\n",
                         message, e.what());

        return -1;
    }

    return 0;
}

bool isPgoodOn()
{
    bool pgoodOn = false;
    int pgoodValue = 0;

    auto msg = bus.new_method_call("org.openbmc.control.Power",
                                   "/org/openbmc/control/power0",
                                   "org.freedesktop.DBus.Properties", "Get");
    msg.append("org.openbmc.control.Power", "pgood");
    try
    {
        auto reply = bus.call(msg);
        std::variant<int> pgoodProp;
        reply.read(pgoodProp);
        pgoodValue = std::get<int>(pgoodProp);

        if (pgoodValue == 1)
        {
            pgoodOn = true;
        }
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        sd_journal_print(LOG_ERR, "Failed to obtain pgood value, %s\n", e.what());
    }

    return pgoodOn;
}

bool isPostComplete()
{
    bool postComplete = false;
    int postValue = 0;

    auto msg = bus.new_method_call("org.openbmc.control.Power",
                                   "/org/openbmc/control/power0",
                                   "org.freedesktop.DBus.Properties", "Get");
    msg.append("org.openbmc.control.PostComplete", "postcomplete");
    try
    {
        auto reply = bus.call(msg);
        std::variant<int> postProp;
        reply.read(postProp);
        postValue = std::get<int>(postProp);

        if (postValue == 0)
        {
            postComplete = true;
        }
    }
    catch (const sdbusplus::exception::SdBusError& e)
    {
        sd_journal_print(LOG_ERR, "Failed to obtain postcomplete value, %s\n", e.what());
    }

    return postComplete;
}

