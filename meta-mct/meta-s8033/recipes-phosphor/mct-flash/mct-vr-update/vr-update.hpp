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
    int progress;                   /* progress update, use the version id */
    int select;                     /* Select update VR device */
    char selectDevice[128];         /* Select update VR device name*/
    char version_id[256];           /* version id */
}vr_t;

struct vrDeviceInfo
{
    uint8_t bus;
    uint8_t pmbusAddr;
    uint8_t devcieId;
    uint8_t selDeviceType;
};

// pmbus address and devcie ID are determined by external resistors
std::map<std::string, vrDeviceInfo> supportedDevice =
{
  // {name,  {bus, addr(7Bits), devcie ID, device type for version chage SEL}}
    {"P0_VDDIO_MEM_ABCD", {4, 0x63, 0x53, 0xC6} },
    {"P0_VDDIO_MEM_EFGH", {4, 0x64, 0x53, 0xC8} },
    {"P0_VDDCR_CPU", {4, 0x60, 0x48, 0xC0} },
    {"P0_VDDCR_SOC", {4, 0x61, 0x59, 0xC2} }
};


void logVrUpdateSel(uint8_t selDeviceType)
{
    std::string selectSensor = "VR_FW_UPDATE";

    logSelEvent("VR firmware update SEL Entry",
        "/xyz/openbmc_project/sensors/versionchange/" + selectSensor,
        0x07,0x00,selDeviceType);
}