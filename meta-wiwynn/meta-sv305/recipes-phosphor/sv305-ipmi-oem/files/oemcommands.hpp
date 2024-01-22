/*
// Copyright (c) 2019 Wiwynn Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#pragma once

#include <ipmid/api.hpp>

#include <map>
#include <string>

static constexpr ipmi_netfn_t netFnSv305OEM1 = 0x2E;
static constexpr ipmi_netfn_t netFnSv305OEM2 = 0x30;
static constexpr ipmi_netfn_t netFnSv305OEM3 = 0x3E;
static constexpr ipmi_netfn_t netFnSv305OEM4 = 0x3C;
static constexpr ipmi_netfn_t netFnSv305OEM5 = 0x34;
static constexpr ipmi_netfn_t netFnSv305OEM6 = 0x36;

static constexpr int gpioJtagBmcOeN = 2;
static constexpr int gpioBmcJtagSel = 148;

static const size_t maxPatternLen = 256;
static const size_t retPostCodeLen = 20;
static const uint8_t oemMaxIPMIWriteReadSize = 50;

enum ipmi_sv305_oem1_cmds : uint8_t
{
    CMD_RANDOM_POWERON_STATUS = 0x15,
    CMD_RANDOM_POWERON_SWITCH = 0x16,
};

enum ipmi_sv305_oem2_cmds : uint8_t
{
    CMD_SET_SERVICE = 0x0D,
    CMD_GET_SERVICE = 0x0E,
    CMD_GET_POST_CODE = 0x10,
    CMD_SET_FAN_PWM = 0x11,
    CMD_SET_FSC_MODE = 0x12,
    CMD_GET_SYSTEM_LED_STATUS = 0x16,
    CMD_SET_GPIO = 0x17,
    CMD_GET_GPIO = 0x18,
    CMD_RESTORE_TO_DEFAULT = 0x19,
    CMD_GET_POST_END_STATUS = 0xAA,
};

enum ipmi_sv305_oem3_cmds : uint8_t
{
    CMD_MASTER_WRITE_READ = 0x01,
    CMD_SET_SOL_PATTERN = 0xB2,
    CMD_GET_SOL_PATTERN = 0xB3,
    CMD_SET_LBA_THRESHOLD = 0xB4,
    CMD_GET_LBA_THRESHOLD = 0xB5,
    CMD_GET_LBA_TOTAL_COUNTER = 0xB6,
    CMD_GET_LBA_RELATIVE_COUNTER = 0xB7,
    CMD_SET_BMC_TIMESYNC_MODE = 0xB8,
    CMD_GET_BMC_BOOT_FROM = 0xBB,
};

enum ipmi_sv305_oem4_cmds : uint8_t
{
    CMD_GET_VR_VERSION = 0x51,
    CMD_ACCESS_CPLD_JTAG = 0x88,
};

enum ipmi_sv305_oem5_cmds : uint8_t
{
    CMD_GET_CPU_POWER = 0x01,
    CMD_GET_CPU_POWER_LIMIT = 0x02,
    CMD_SET_SYSTEM_POWER_LIMIT = 0x03,
    CMD_SET_DEFAULT_CPU_POWER_LIMIT = 0x04,
    CMD_GET_SYSTEM_POWER_LIMIT = 0x05,
};

enum ipmi_sv305_oem6_cmds : uint8_t
{
    CMD_APML_READ_PACKAGE_POWER_CONSUMPTION = 0x15,
    CMD_APML_WRITE_PACKAGE_POWER_LIMIT = 0x16,
};

enum
{
    FSC_MODE_MANUAL = 0x00,
    FSC_MODE_AUTO = 0x01,
};

enum vr_type : uint8_t
{
    CPU0_VCCIN = 0x00,
    CPU1_VCCIN,
    CPU0_DIMM0,
    CPU0_DIMM1,
    CPU1_DIMM0,
    CPU1_DIMM1,
    CPU0_VCCIO,
    CPU1_VCCIO,
};

enum restore_item : uint8_t
{
    allItemRestore = 0x0,
    restoreItemSensorThreshold = 0x01,
    restoreItemFanMode = 0x2,
    restoreItemPowerInterval = 0x3,
};

enum cpld_operation : uint8_t
{
    GET_IDCODE = 0x02,
    GET_USERCODE = 0x03,
};

enum bmc_from : uint8_t
{
    primaryBMC = 0x1,
    backupBMC = 0x2,
};

struct SetGpioReq
{
    uint8_t gpioNum;
    uint8_t direction;
    uint8_t value;
};

struct RestoreToDefaultReq
{
    uint8_t restoreItem;
};

struct GetGpioReq
{
    uint8_t gpioNum;
};

struct GetGpioRes
{
    uint8_t direction;
    uint8_t value;
};

// Maintain the request data pwmIndex (which pwm we are going to write).
std::map<uint8_t, std::string> pwmFileTable = {{0x00, "pwm1"}, {0x01, "pwm2"},
                                               {0x02, "pwm3"}, {0x03, "pwm4"},
                                               {0x04, "pwm5"}, {0x05, "pwm6"}};
// Maintain which zone pwm files belong to.
std::map<std::string, std::string> zoneTable = {
    {"pwm1", "zone1"}, {"pwm2", "zone1"}, {"pwm3", "zone2"},
    {"pwm4", "zone2"}, {"pwm5", "zone2"}, {"pwm6", "zone2"}};

constexpr auto parentPwmDir = "/sys/devices/platform/ahb/ahb:apb/"
                              "1e786000.pwm-tacho-controller/hwmon/";
constexpr auto manualModeFilePath = "/tmp/fanCtrlManual/";
constexpr auto sensorThresholdPath = "/var/configuration/system.json";

constexpr auto SYSTEMD_SERVICE = "org.freedesktop.systemd1";
constexpr auto SYSTEMD_OBJ_PATH = "/org/freedesktop/systemd1";
constexpr auto SYSTEMD_INTERFACE = "org.freedesktop.systemd1.Manager";

struct SetPwmRequest
{
    uint8_t pwmIndex;   // 00h - 05h (pwm1 - pwm6), 06h: for all of fan.
    uint8_t pwmPercent; // 01h - 64h (1 - 100)
};

struct SetFscModeRequest
{
    uint8_t mode; // 00h Manual, 01h Auto
};
struct MasterWriteReadReq
{
    uint8_t busId;
    uint8_t slaveAddr;
    uint8_t readCount;
    uint8_t writeData[oemMaxIPMIWriteReadSize];
};

struct MasterWriteReadRes
{
    uint8_t readResp[oemMaxIPMIWriteReadSize];
};

static constexpr uint8_t maxLBAThresholdNum = 3;
static constexpr auto leckyBucketDimmConfigPath = "/etc/leaky-bucket-dimm.json";

struct SetLBAThresholdRequest
{
    uint8_t thresholdNumber;
    uint16_t thresholdValue;
} __attribute__((packed));

struct GetLBAThresholdRequest
{
    uint8_t thresholdNumber;
};

struct GetLBAThresholdResponse
{
    uint16_t thresholdValue;
};

struct GetLBATotalCounterRequest
{
    uint8_t dimmIndex;
};

struct GetLBATotalCounterResponse
{
    uint32_t totalCounter;
};

struct GetLBARelativeCounterRequest
{
    uint8_t dimmIndex;
};

struct GetLBARelativeCounterResonse
{
    uint32_t relativeCounter;
};

static constexpr auto settingMgtService = "xyz.openbmc_project.Settings";
static constexpr auto powerRestorePolicyPath =
    "/xyz/openbmc_project/control/host0/power_restore_policy";
static constexpr auto powerRestorePolicyIntf =
    "xyz.openbmc_project.Control.Power.RestorePolicy";

struct GetRandomPowerOnStatusResonse
{
    uint8_t delayMin;
};

struct SetRandomPowerOnSwitchRequest
{
    uint8_t delayMin;
};

enum HostStatus : uint8_t
{
    BiosPostEnd = 0x00,
    DuringBiosPost = 0x01,
    PowerOff = 0xE0,
};

enum GpioStatus : int
{
    gpioHi = 1,
    gpioLo = 0,
};

typedef struct
{
    uint8_t readBootAtNum;  // 1 -> BMC1, 2 -> BMC2
}__attribute__((packed)) GetBMC;

typedef struct
{
    uint8_t postCode[retPostCodeLen];
}__attribute__((packed)) GetPostCodeRes;

typedef struct
{
    uint8_t manufId[3];
}__attribute__((packed)) GetPostCodeReq;

typedef struct
{
    uint8_t postEndStatus;
} GetPostEndStatusRes;

typedef struct
{
    uint8_t sysLedStatus;
}__attribute__((packed)) GetSystemLedStatusRes;

typedef struct
{
    uint8_t patternIdx;
    char data[maxPatternLen];
}__attribute__((packed)) SetSolPatternCmdReq;

typedef struct
{
    uint8_t patternIdx;
}__attribute__((packed)) GetSolPatternCmdReq;

typedef struct
{
    char data[maxPatternLen];
}__attribute__((packed)) GetSolPatternCmdRes;

typedef struct
{
    uint8_t syncMode;
}__attribute__((packed)) SetBmcTimeSyncModeReq;

typedef struct
{
    uint8_t vrType;
}__attribute__((packed)) GetVrVersionCmdReq;

typedef struct
{
    uint16_t verData0;
    uint16_t verData1;
}__attribute__((packed)) GetVrVersionCmdRes;

typedef struct
{
    uint8_t cpldOp;
}__attribute__((packed)) AccessCpldJtagCmdReq;

typedef struct
{
    uint32_t result;
}__attribute__((packed)) AccessCpldJtagCmdRes;

typedef struct
{
    uint8_t serviceSetting;
}__attribute__((packed)) ServiceSettingReq;

typedef struct
{
    int8_t socketID;
}__attribute__((packed)) GetCpuPowerCmdReq;

typedef struct
{
    uint32_t powerConsumption;
}__attribute__((packed)) GetCpuPowerCmdRes;

typedef struct
{
    int8_t socketID;
}__attribute__((packed)) GetCpuPowerLimitCmdReq;

typedef struct
{
    uint32_t powerLimit;
}__attribute__((packed)) GetCpuPowerLimitCmdRes;

typedef struct
{
    int8_t socketID;
    uint32_t systemPowerLimit;
}__attribute__((packed)) SetSystemPowerLimitCmdReq;

typedef struct
{
    int8_t socketID;
}__attribute__((packed)) SetDefaultCpuPowerCmdReq;

typedef struct
{
    uint32_t syspowerLimit;
}__attribute__((packed)) GetSystemPowerLimitCmdRes;

