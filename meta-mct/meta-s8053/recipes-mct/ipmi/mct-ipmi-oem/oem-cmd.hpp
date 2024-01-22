#pragma once

#include <stdint.h>
#include <host-ipmid/ipmid-api.h>

enum ipmi_net_fns_oem
{
    NETFUN_TWITTER_OEM = 0x30,
    NETFUN_TWITTER_OEM_38 = 0x38,
};

enum ipmi_net_fns_oem_cmds
{
    IPMI_CMD_FloorDuty = 0x07,
    IPMI_CMD_SetService = 0x0D,
    IPMI_CMD_GetService = 0x0E,
    IPMI_CMD_BmcFlashDevice = 0x0F,
    IPMI_CMD_GetPostCode = 0x10,
    IPMI_CMD_GetCpuPower = 0x11,
    IPMI_CMD_GetCpuPowerLimit = 0x12,
    IPMI_CMD_SetCpuPowerLimit = 0x13,
    IPMI_CMD_SetDefaultCpuPowerLimit = 0x14,
    IPMI_CMD_BiosRecovery = 0x16,
    IPMI_CMD_BiosFlashDevice = 0x17,
    IPMI_CMD_RamdomDelayACRestorePowerON = 0x18,
    IPMI_CMD_GetAmberLedStatus = 0x19,
    IPMI_CMD_GetHostEccCount = 0x1B,
    IPMI_CMD_GetSysFirmwareVersion= 0x1C,
    IPMI_CMD_SetSysPowerLimit = 0x20,
    IPMI_CMD_GetSysPowerLimit = 0x21,
    IPMI_CMD_EnableCpuPowerLimit = 0x22,
    IPMI_CMD_ConfigAmdMbist = 0x23,
    IPMI_CMD_GetCurrentNodeId = 0x24,
    IPMI_CMD_SetSpecialMode = 0x25,
    IPMI_CMD_GetSpecialMode = 0x26,
    IPMI_CMD_SetOtpStrap = 0x27,
    IPMI_CMD_GetOtpStrap = 0x28,
    IPMI_CMD_ConfigBmcP2a = 0x29,
    IPMI_CMD_SET_SOL_PATTERN = 0xB2,
    IPMI_CMD_GET_SOL_PATTERN = 0xB3,
    IPMI_CMD_BMC_RESTORE_DEFAULT = 0xB4,
};

enum ipmi_net_fns_oem_38_cmds
{
    IPMI_CMD_DisableHsc = 0x20,
    IPMI_CMD_HscACStart = 0x21,
    IPMI_CMD_HscConfigACDelay = 0x22,
};
