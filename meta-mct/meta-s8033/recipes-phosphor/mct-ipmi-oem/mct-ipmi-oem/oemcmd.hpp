#include <stdint.h>
#include <host-ipmid/ipmid-api.h>

enum ipmi_net_fns_oem
{
    NETFUN_TWITTER_OEM = 0x30,
};

#define IANA_TYAN 0x0019fd
enum ipmi_net_fns_oem_cmds
{
    IPMI_CMD_FanPwmDuty = 0x05,
    IPMI_CMD_ManufactureMode = 0x06,
    IPMI_CMD_FloorDuty = 0x07,
    IPMI_CMD_SetService = 0x0D,
    IPMI_CMD_SetManufactureData = 0x0D,
    IPMI_CMD_GetService = 0x0E,
    IPMI_CMD_GetManufactureData = 0x0E,
    IPMI_CMD_BmcFlashDevice = 0x0F,
    IPMI_CMD_GetFirmwareString = 0x10,
    IPMI_CMD_GetPostCode = 0x10,
    IPMI_CMD_GetCpuPower = 0x11,
    IPMI_CMD_GetCpuPowerLimit = 0x12,
    IPMI_CMD_SetCpuPowerLimit = 0x13,
    IPMI_CMD_SetDefaultCpuPowerLimit = 0x14,
    IPMI_CMD_DisableHsc = 0x15,
    IPMI_CMD_BiosRecovery = 0x16,
    IPMI_CMD_BiosFlashDevice = 0x17,
    IPMI_CMD_RamdomDelayACRestorePowerON = 0x18,
    IPMI_CMD_GetAmberLedStatus = 0x19,
    IPMI_CMD_ConfigEccLeakyBucket = 0x1A,
    IPMI_CMD_GetEccCount = 0x1B,
    IPMI_CMD_GetHostEccCount = 0x1B,
    IPMI_CMD_GetSysFirmwareVersion= 0x1C,
    IPMI_CMD_SetSysPowerLimit = 0x20,
    IPMI_CMD_GetSysPowerLimit = 0x21,
    IPMI_CMD_EnableCpuPowerLimit = 0x22,
    IPMI_CMD_ConfigAmdMbist = 0x23,
    IPMI_CMD_ClearCmos = 0x3A,
    IPMI_CMD_gpioStatus = 0x41,
    IPMI_CMD_SET_SOL_PATTERN = 0xB2,
    IPMI_CMD_GET_SOL_PATTERN = 0xB3,
};