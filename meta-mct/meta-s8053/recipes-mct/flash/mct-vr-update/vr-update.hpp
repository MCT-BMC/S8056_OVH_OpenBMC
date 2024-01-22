#pragma once

#include <iostream>
#include <chrono>
#include <ctime>
#include <fstream>
#include <string>
#include <fcntl.h>
#include <sdbusplus/bus.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <vr-update-manager.hpp>
#include <misc-utils.hpp>
#include <vr-update-utils.hpp>

typedef struct {
    int program;                    /* enable/disable program  */
    int debug;                      /* enable/disable debug flag */
    int crc;                        /* enable/disable getting current VR firmware CRC value */
    int progress;                   /* progress update, use the version id */
    int select;                     /* Select update VR device */
    char selectDevice[128];         /* Select update VR device name*/
    char version_id[256];           /* version id */
}vr_t;

struct vrDeviceInfo
{
    uint8_t bus;
    uint8_t pmbusAddr;
    uint32_t devcieId;
    uint8_t selDeviceType;
};

// pmbus address and devcie ID are determined by external resistors
std::map<std::string, vrDeviceInfo> supportedDevice =
{
  // {name,  {bus, addr(7Bits), devcie ID, device type for version chage SEL}}
    {"P0_VDDCR_SOC", {11, 0x61, 0x49D29B00, 0xC2} },
    {"P0_VDD11", {11, 0x62, 0x49D29B00, 0xC4} },
    {"P0_VDDIO", {11, 0x63, 0x49D29C00, 0xC6} }
};


void logVrUpdateSel(uint8_t selDeviceType)
{
    std::string selectSensor = "VR_FW_UPDATE";

    logSelEvent("VR firmware update SEL Entry",
        "/xyz/openbmc_project/sensors/versionchange/" + selectSensor,
        0x07,0x00,selDeviceType);
}