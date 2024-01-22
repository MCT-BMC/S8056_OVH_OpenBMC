#include <ipmid/api.hpp>
#include <ipmid/types.hpp>
#include <fstream>
#include <systemd/sd-journal.h>

static constexpr auto powerCycleIntervalConfigPath =
    "/etc/default/obmc/phosphor-reboot-host/reboot.conf";

static void register_chassis_functions() __attribute__((constructor));

ipmi::RspType<> ipmiSetPowerCycleInterval(uint8_t interval)
{
    std::ofstream fout;
    fout.open(powerCycleIntervalConfigPath, std::ios::out | std::ios::trunc);
    if (fout.is_open() == false)
    {
        sd_journal_print(LOG_ERR, "[%s] failed to open file %s\n",
                         __FUNCTION__, powerCycleIntervalConfigPath);
        return ipmi::responseDestinationUnavailable();
    }

    fout << "REBOOT_DELAY=" << static_cast<uint32_t>(interval) << "\n";
    if (fout.fail() == true)
    {
        fout.close();
        sd_journal_print(LOG_ERR, "[%s] failed to write file %s\n",
                         __FUNCTION__, powerCycleIntervalConfigPath);
        return ipmi::responseDestinationUnavailable();
    }

    fout.close();

    return ipmi::responseSuccess();
}

static void register_chassis_functions()
{
    // <Set Power Cycle Interval>
    ipmi::registerHandler(ipmi::prioOemBase, ipmi::netFnChassis,
                          ipmi::chassis::cmdSetPowerCycleInterval,
                          ipmi::Privilege::Admin, ipmiSetPowerCycleInterval);
}
