#pragma once

#include <stdint.h>
#include <vector>
#include <xyz/openbmc_project/State/Host/server.hpp>

#define PROPERTY_INTERFACE "org.freedesktop.DBus.Properties"
#define POWER_STATE_OBJ "org.openbmc.control.Power"
#define POWER_STATE_PATH "/org/openbmc/control/power0"
#define POWER_STATE_INTERFACE "org.openbmc.control.Power"

#define HOST_STATE_OBJ "xyz.openbmc_project.State.Host"
#define HOST_STATE_PATH "/xyz/openbmc_project/state/host0"
#define HOST_STATE_INTERFACE "xyz.openbmc_project.State.Host"

#define IPMB_BRIDGE_OBJ "xyz.openbmc_project.Ipmi.Channel.Ipmb"
#define IPMB_BRIDGE_PATH "/xyz/openbmc_project/Ipmi/Channel/Ipmb"
#define IPMB_BRIDGE_INTERFACE "org.openbmc.Ipmb"

#define BIOS_SPI "1e630000.spi"
#define BIOS_DRIVER_PATH "/sys/bus/platform/drivers/aspeed-smc/"

constexpr int MAX_RETRY_RECOVERY_MODE = 3;

// phosphor-dbus-interfaces
namespace State = sdbusplus::xyz::openbmc_project::State::server;

enum mtd_mount_state : uint8_t
{
    unbind = 0,
    bind
};

enum pgood_state : uint8_t
{
    off = 0,
    on
};

class BiosUpdateManager{

public:
    int8_t biosUpdatePwrStateCheck(uint8_t& state);
    int8_t biosUpdatePwrCtl(uint8_t state);
    int8_t biosUpdatePrepare();
    int8_t biosUpdate(char* image_str);
    int8_t biosUpdateFinished(char* image_str);
private:
    int8_t systemPowerStateCheck();
    int8_t performStateTransition(State::Host::Transition transition);
    int8_t setMeToRecoveryMode();
    int8_t resetMeToBoot();
    int8_t setBiosMtdDevice(uint8_t state);
    int8_t checkMeToRecoveryMode();
    bool isRecoveryMode;
};
