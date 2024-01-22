#pragma once

#include <stdint.h>
#include <host-ipmid/ipmid-api.h>

#define IANA_TYAN 0x0019fd

enum ipmi_net_fns_tyan_oem_cmds
{
    IPMI_CMD_SetgpioStatus = 0x03,
    IPMI_CMD_GetgpioStatus = 0x04,
    IPMI_CMD_MasterWriteRead = 0x09,
    IPMI_CMD_SetManufactureData = 0x0D,
    IPMI_CMD_GetManufactureData = 0x0E,
    IPMI_CMD_ConfigEccLeakyBucket = 0x1A,
    IPMI_CMD_GetEccCount = 0x1B,
    IPMI_CMD_SetFanControl = 0x40,
    IPMI_CMD_GetFanControl = 0x41,
    IPMI_CMD_SetFanPwmDuty = 0x44,
    IPMI_CMD_GetFanPwmDuty = 0x45,
    IPMI_CMD_ClearCmos = 0x65,
    IPMI_CMD_GetFirmwareString = 0x80,
};
